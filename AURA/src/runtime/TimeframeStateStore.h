#ifndef AURA_RUNTIME_TIMEFRAMESTATESTORE_H
#define AURA_RUNTIME_TIMEFRAMESTATESTORE_H

#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/IPersistenceStore.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "runtime/AdapterManager.h"
#include "runtime/BarFinalizer.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace aura {
namespace runtime {

// Last-processed-bar progress for one timeframe.
//
// RT-0005 / Phase 1 deterministic runtime. Holds the deterministic identity of
// the last processed closed bar plus its sequence, per timeframe. Progress is
// monotonic: it may only advance to a strictly later closed bar, so a restart or
// a delayed message can never move a timeframe backwards or repaint history.
struct TimeframeProgress {
    Timeframe timeframe{Timeframe::UNKNOWN};
    BarIdentity last_processed{};
    foundation::Timestamp last_close{};
    std::uint64_t sequence{0};
};

// Deterministic text codec for a progress record. Round-trips exactly.
inline std::string encode_progress(const TimeframeProgress& progress) {
    return progress.last_processed.symbol() + "|" + std::string(to_string(progress.timeframe)) + "|" +
           std::to_string(progress.last_close.nanoseconds()) + "|" +
           std::to_string(progress.sequence);
}

inline bool decode_progress(std::string_view text, TimeframeProgress& out) {
    const std::size_t p1 = text.find('|');
    if (p1 == std::string_view::npos) return false;
    const std::size_t p2 = text.find('|', p1 + 1);
    if (p2 == std::string_view::npos) return false;
    const std::size_t p3 = text.find('|', p2 + 1);
    if (p3 == std::string_view::npos) return false;

    TimeframeProgress progress;
    progress.last_processed =
        BarIdentity(std::string(text.substr(0, p1)), timeframe_from_string(text.substr(p1 + 1, p2 - p1 - 1)),
                    foundation::Timestamp::from_nanoseconds(0));
    progress.timeframe = timeframe_from_string(text.substr(p1 + 1, p2 - p1 - 1));
    if (progress.timeframe == Timeframe::UNKNOWN) return false;

    foundation::Timestamp::rep close_ns = 0;
    for (const char c : text.substr(p2 + 1, p3 - p2 - 1)) {
        if (c < '0' || c > '9') return false;
        close_ns = close_ns * 10 + (c - '0');
    }
    std::uint64_t sequence = 0;
    for (const char c : text.substr(p3 + 1)) {
        if (c < '0' || c > '9') return false;
        sequence = sequence * 10 + static_cast<std::uint64_t>(c - '0');
    }
    progress.last_close = foundation::Timestamp::from_nanoseconds(close_ns);
    progress.sequence = sequence;
    progress.last_processed = BarIdentity(std::string(text.substr(0, p1)), progress.timeframe,
                                          progress.last_close);
    if (!progress.last_processed.valid()) return false;
    out = std::move(progress);
    return true;
}

// Per-timeframe last-processed-bar store.
//
// RT-0005 / Phase 1 deterministic runtime. Persists the last processed closed bar
// for each timeframe so restart resumes without reprocessing or repainting.
// Advances are monotonic per timeframe and independent across timeframes. Persist
// uses the append-oriented IPersistenceStore (PER-0003) with idempotent record
// identities; restore merges decoded records monotonically and never regresses
// state. Deterministic throughout; no clock, no threads.
class TimeframeStateStore {
public:
    using progress_map = std::map<Timeframe, TimeframeProgress>;

    TimeframeStateStore() = default;

    // Advances a timeframe to a finalized bar. Returns false, with no change, when
    // the bar would not strictly advance that timeframe's close time.
    bool advance(const FinalizedBar& finalized) {
        const Timeframe tf = finalized.bar.timeframe;
        if (tf == Timeframe::UNKNOWN) return false;
        if (!finalized.identity.valid()) return false;

        auto it = progress_.find(tf);
        if (it != progress_.end() && finalized.bar.close_time <= it->second.last_close) return false;

        TimeframeProgress progress;
        progress.timeframe = tf;
        progress.last_processed = finalized.identity;
        progress.last_close = finalized.bar.close_time;
        progress.sequence = it == progress_.end() ? 1 : it->second.sequence + 1;
        progress_[tf] = std::move(progress);
        return true;
    }

    bool has(Timeframe timeframe) const { return progress_.find(timeframe) != progress_.end(); }

    const TimeframeProgress* last_processed(Timeframe timeframe) const {
        const auto it = progress_.find(timeframe);
        return it == progress_.end() ? nullptr : &it->second;
    }

    foundation::Timestamp last_close(Timeframe timeframe) const {
        const TimeframeProgress* p = last_processed(timeframe);
        return p == nullptr ? foundation::Timestamp{} : p->last_close;
    }

    std::size_t size() const noexcept { return progress_.size(); }
    const progress_map& progress() const noexcept { return progress_; }

    // Persists every timeframe's progress. Idempotent: a re-persist of unchanged
    // progress appends nothing new. Returns the worst status observed.
    foundation::PersistenceStatus persist(foundation::IPersistenceStore& store,
                                          const foundation::SchemaVersion& schema,
                                          foundation::Timestamp written_at) const {
        foundation::PersistenceStatus worst = foundation::PersistenceStatus::OK;
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return foundation::PersistenceStatus::UNAVAILABLE;

        for (const auto& entry : progress_) {
            const std::string payload = encode_progress(entry.second);
            const foundation::EntityId record_id = record_id_for(entry.second);
            const foundation::HashDigest digest = hasher->hash(payload);
            if (digest.empty()) return foundation::PersistenceStatus::FAILED;
            const foundation::PersistenceStatus status = store.append(
                foundation::PersistenceRecordMetadata(record_id, schema, written_at), digest);
            if (!foundation::is_success(status) && status != foundation::PersistenceStatus::CONFLICT) {
                worst = status;
            }
        }
        return worst;
    }

    // Merges a decoded record monotonically: a record that would regress a
    // timeframe is ignored. Returns true when state advanced.
    bool restore(const TimeframeProgress& progress) {
        if (progress.timeframe == Timeframe::UNKNOWN || !progress.last_processed.valid()) return false;
        auto it = progress_.find(progress.timeframe);
        if (it != progress_.end() && progress.last_close <= it->second.last_close) return false;
        progress_[progress.timeframe] = progress;
        return true;
    }

    // Deterministic, idempotent record identity for a progress entry.
    static foundation::EntityId record_id_for(const TimeframeProgress& progress) {
        return foundation::EntityId(std::string("tfstate|") + std::string(to_string(progress.timeframe)) +
                                    "|" + progress.last_processed.symbol() + "|" +
                                    std::to_string(progress.last_close.nanoseconds()));
    }

private:
    progress_map progress_{};
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_TIMEFRAMESTATESTORE_H
