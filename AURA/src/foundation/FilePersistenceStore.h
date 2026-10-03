#ifndef AURA_FOUNDATION_FILEPERSISTENCESTORE_H
#define AURA_FOUNDATION_FILEPERSISTENCESTORE_H

#include "foundation/EntityId.h"
#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/IPersistenceStore.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace aura {
namespace foundation {

// File-backed, append-only, crash-safe persistence store (V3-22).
//
// PERSIST-0001. This is the concrete backing for IPersistenceStore (PER-0003)
// that the application was missing. It satisfies the existing contract without
// inventing a new one:
//
//   - Append-only: records are appended as newline-delimited text lines and
//     history is never rewritten.
//   - Idempotent: re-appending an identical record identity + payload digest is a
//     no-op OK; a conflicting payload for the same identity is CONFLICT.
//   - Corruption-detecting: the whole file has a deterministic SHA-256 checksum;
//     load() verifies it and a mismatch is reported as CORRUPT, never silently
//     accepted. Malformed lines/headers are UNAVAILABLE, not OK.
//   - Crash-safe: flush() persists the file atomically (temp file + fsync +
//     rename) so a crash mid-write cannot leave a torn file that could be mistaken
//     for complete state.
//   - Deterministic: identical append sequences produce byte-identical files.
//
// Line grammar (tab-separated, fields carry no tabs/newlines by construction):
//   AURA-PERSIST\t<format_version>\n
//   <record_id>\t<schema_version>\t<written_at_ns>\t<payload_digest_hex>\t<payload>\n
//   ...
//   #checksum\t<sha256_hex_of_preceding_bytes>\n
//
// The IPersistenceStore contract stores a payload digest; this concrete store
// additionally retains the payload text so a caller can restore the exact state
// it persisted (a digest alone cannot be replayed). A payload containing a tab or
// newline is refused rather than corrupted.
class FilePersistenceStore : public IPersistenceStore {
public:
    static constexpr std::string_view kFormatVersion = "1.0.0";
    static constexpr std::string_view kHeader = "AURA-PERSIST";
    static constexpr std::string_view kChecksumMarker = "#checksum";

    explicit FilePersistenceStore(std::string path) : path_(std::move(path)) {}

    const std::string& path() const noexcept { return path_; }

    PersistenceStatus append(const PersistenceRecordMetadata& metadata,
                             const HashDigest& payload_digest) override {
        return store(metadata, payload_digest.to_hex(), std::string{});
    }

    // Appends a record together with its payload text. The digest is computed from
    // the payload deterministically; the payload is retained for restoration.
    PersistenceStatus append_record(const PersistenceRecordMetadata& metadata,
                                    const std::string& payload) {
        if (payload.find('\t') != std::string::npos || payload.find('\n') != std::string::npos) {
            return PersistenceStatus::FAILED;
        }
        const std::unique_ptr<IHasher> hasher = create_hasher(HashAlgorithm::SHA256);
        if (hasher == nullptr) return PersistenceStatus::UNAVAILABLE;
        const std::string digest = hasher->hash(payload).to_hex();
        if (digest.empty()) return PersistenceStatus::FAILED;
        return store(metadata, digest, payload);
    }

    PersistenceStatus contains(const EntityId& record_id) const override {
        return records_.find(record_id.value()) != records_.end() ? PersistenceStatus::OK
                                                                  : PersistenceStatus::NOT_FOUND;
    }

    PersistenceStatus flush() override { return save(); }

    // Number of records currently held (loaded or appended).
    std::size_t size() const noexcept { return records_.size(); }

    // The recorded payload digest for a record identity, or "" when absent.
    std::string digest_of(const EntityId& record_id) const {
        const auto it = records_.find(record_id.value());
        return it == records_.end() ? std::string{} : it->second.digest;
    }

    // The recorded payload text for a record identity, or "" when absent/empty.
    std::string payload_of(const EntityId& record_id) const {
        const auto it = records_.find(record_id.value());
        return it == records_.end() ? std::string{} : it->second.payload;
    }

    // Read-only view of one stored record, in append order.
    struct View {
        std::string record_id;
        std::string schema_version;
        std::string payload;
    };

    // All records in deterministic append order.
    std::vector<View> ordered() const {
        std::vector<View> out;
        out.reserve(order_.size());
        for (const std::string& id : order_) {
            const Record& record = records_.at(id);
            out.push_back(View{record.record_id, record.schema_version, record.payload});
        }
        return out;
    }

    // Loads and verifies an existing file created by this store.
    //
    // NOT_FOUND when the file does not exist (a first run, which is not an
    // error). OK when the file is present, parses, and its checksum matches.
    // CORRUPT when the checksum does not match (a torn or tampered file).
    // UNAVAILABLE when the file is present but structurally unreadable/malformed.
    PersistenceStatus load() {
        std::ifstream input(path_, std::ios::binary);
        if (!input) return PersistenceStatus::NOT_FOUND;

        std::string content((std::istreambuf_iterator<char>(input)),
                            std::istreambuf_iterator<char>());
        if (content.empty()) return PersistenceStatus::UNAVAILABLE;

        const std::size_t marker = content.rfind(std::string(kChecksumMarker));
        if (marker == std::string::npos) return PersistenceStatus::CORRUPT;
        const std::size_t nl = content.find('\n', marker);
        const std::string recorded_checksum =
            trim(content.substr(marker + kChecksumMarker.size(), nl == std::string::npos
                                                                      ? std::string::npos
                                                                      : nl - marker - kChecksumMarker.size()));
        if (recorded_checksum.empty()) return PersistenceStatus::CORRUPT;

        const std::string body = content.substr(0, marker);
        const std::unique_ptr<IHasher> hasher = create_hasher(HashAlgorithm::SHA256);
        if (hasher == nullptr) return PersistenceStatus::UNAVAILABLE;
        if (hasher->hash(body).to_hex() != recorded_checksum) return PersistenceStatus::CORRUPT;

        std::istringstream stream(body);
        std::string line;
        if (!std::getline(stream, line)) return PersistenceStatus::UNAVAILABLE;
        std::string header = line;
        if (!header.empty() && header.back() == '\r') header.pop_back();
        std::vector<std::string> header_fields;
        split_into(header, header_fields);
        if (header_fields.size() != 2 || header_fields[0] != kHeader) {
            return PersistenceStatus::UNAVAILABLE;
        }
        if (header_fields[1] != kFormatVersion) return PersistenceStatus::UNAVAILABLE;

        std::map<std::string, Record> loaded;
        std::vector<std::string> loaded_order;
        while (std::getline(stream, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            std::vector<std::string> fields;
            split_into(line, fields);
            if (fields.size() != 5) return PersistenceStatus::UNAVAILABLE;
            Record record;
            record.record_id = fields[0];
            record.schema_version = fields[1];
            if (!parse_i64(fields[2], record.written_at_ns)) return PersistenceStatus::UNAVAILABLE;
            record.digest = fields[3];
            record.payload = fields[4];
            if (loaded.find(record.record_id) != loaded.end()) {
                return PersistenceStatus::UNAVAILABLE;  // duplicate identity in file
            }
            loaded_order.push_back(record.record_id);
            loaded.emplace(record.record_id, std::move(record));
        }

        records_ = std::move(loaded);
        order_ = std::move(loaded_order);
        return PersistenceStatus::OK;
    }

    // Writes the whole store atomically. Returns OK on success, UNAVAILABLE when
    // the temp/final file cannot be written, FAILED on rename failure.
    PersistenceStatus save() {
        const std::string body = render_body();
        const std::unique_ptr<IHasher> hasher = create_hasher(HashAlgorithm::SHA256);
        if (hasher == nullptr) return PersistenceStatus::UNAVAILABLE;
        const std::string full =
            body + std::string(kChecksumMarker) + "\t" + hasher->hash(body).to_hex() + "\n";

        const std::string tmp = path_ + ".tmp";
        {
            std::FILE* fp = std::fopen(tmp.c_str(), "wb");
            if (fp == nullptr) return PersistenceStatus::UNAVAILABLE;
            const std::size_t written = std::fwrite(full.data(), 1, full.size(), fp);
            std::fflush(fp);
#if defined(_WIN32)
            _commit(_fileno(fp));
#else
            ::fsync(::fileno(fp));
#endif
            std::fclose(fp);
            if (written != full.size()) {
                std::remove(tmp.c_str());
                return PersistenceStatus::UNAVAILABLE;
            }
        }
#if defined(_WIN32)
        // std::rename does not overwrite an existing destination on Windows, so
        // clear the target first; the atomic swap is then a same-directory rename.
        if (std::rename(tmp.c_str(), path_.c_str()) != 0) {
            std::remove(path_.c_str());
            if (std::rename(tmp.c_str(), path_.c_str()) != 0) {
                std::remove(tmp.c_str());
                return PersistenceStatus::FAILED;
            }
        }
#else
        if (std::rename(tmp.c_str(), path_.c_str()) != 0) {
            std::remove(tmp.c_str());
            return PersistenceStatus::FAILED;
        }
#endif
        return PersistenceStatus::OK;
    }

    // Renders exactly the bytes save() would write (excluding the checksum line),
    // so a caller can compute/verify the artifact digest deterministically.
    std::string render_body() const {
        std::string body;
        body += std::string(kHeader) + "\t" + std::string(kFormatVersion) + "\n";
        for (const std::string& id : order_) {
            const Record& record = records_.at(id);
            body += record.record_id + "\t" + record.schema_version + "\t" +
                    std::to_string(record.written_at_ns) + "\t" + record.digest + "\t" +
                    record.payload + "\n";
        }
        return body;
    }

private:
    struct Record {
        std::string record_id;
        std::string schema_version;
        std::int64_t written_at_ns{0};
        std::string digest;
        std::string payload;
    };

    PersistenceStatus store(const PersistenceRecordMetadata& metadata, std::string digest,
                            std::string payload) {
        if (!metadata.auditable()) return PersistenceStatus::FAILED;
        const std::string id = metadata.record_id().value();
        const auto it = records_.find(id);
        if (it != records_.end()) {
            return it->second.digest == digest ? PersistenceStatus::OK
                                               : PersistenceStatus::CONFLICT;
        }
        Record record;
        record.record_id = id;
        record.schema_version = metadata.schema_version().to_string();
        record.written_at_ns = metadata.written_at().nanoseconds();
        record.digest = std::move(digest);
        record.payload = std::move(payload);
        order_.push_back(id);
        records_.emplace(id, std::move(record));
        return PersistenceStatus::OK;
    }

    static std::string trim(std::string_view text) {
        std::size_t begin = 0;
        std::size_t end = text.size();
        while (begin < end && (text[begin] == ' ' || text[begin] == '\t' || text[begin] == '\r')) ++begin;
        while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r')) --end;
        return std::string(text.substr(begin, end - begin));
    }

    static void split_into(const std::string& text, std::vector<std::string>& out) {
        std::size_t start = 0;
        for (;;) {
            const std::size_t tab = text.find('\t', start);
            if (tab == std::string::npos) {
                out.push_back(text.substr(start));
                return;
            }
            out.push_back(text.substr(start, tab - start));
            start = tab + 1;
        }
    }

    static bool parse_i64(const std::string& text, std::int64_t& out) {
        if (text.empty()) return false;
        bool negative = false;
        std::size_t i = 0;
        if (text[0] == '-') {
            negative = true;
            i = 1;
        }
        std::int64_t value = 0;
        for (; i < text.size(); ++i) {
            if (text[i] < '0' || text[i] > '9') return false;
            value = value * 10 + (text[i] - '0');
        }
        out = negative ? -value : value;
        return true;
    }

    std::string path_;
    std::map<std::string, Record> records_{};
    std::vector<std::string> order_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_FILEPERSISTENCESTORE_H
