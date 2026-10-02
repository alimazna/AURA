#ifndef AURA_RUNTIME_SHADOWLEDGER_H
#define AURA_RUNTIME_SHADOWLEDGER_H

#include "foundation/EntityId.h"
#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/IPersistenceStore.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "runtime/PositionSimulator.h"
#include "runtime/ShadowExecutionEngine.h"
#include "runtime/SignalEngine.h"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace aura {
namespace runtime {

// Kinds of record held in the shadow ledger.
enum class LedgerEntryType : std::uint8_t {
    UNKNOWN = 0,
    SIGNAL,
    SCORE,
    CONFIDENCE,
    RISK_PROPOSAL,
    SIMULATED_FILL,
    POSITION,
    RECONCILIATION,
    OUTCOME,
};

constexpr std::string_view to_string(LedgerEntryType type) noexcept {
    switch (type) {
        case LedgerEntryType::UNKNOWN:         return "UNKNOWN";
        case LedgerEntryType::SIGNAL:          return "SIGNAL";
        case LedgerEntryType::SCORE:           return "SCORE";
        case LedgerEntryType::CONFIDENCE:      return "CONFIDENCE";
        case LedgerEntryType::RISK_PROPOSAL:   return "RISK_PROPOSAL";
        case LedgerEntryType::SIMULATED_FILL:  return "SIMULATED_FILL";
        case LedgerEntryType::POSITION:        return "POSITION";
        case LedgerEntryType::RECONCILIATION:  return "RECONCILIATION";
        case LedgerEntryType::OUTCOME:         return "OUTCOME";
    }
    return "UNKNOWN";
}

// One immutable, append-only ledger record.
struct LedgerEntry {
    foundation::EntityId entry_id{};
    LedgerEntryType type{LedgerEntryType::UNKNOWN};
    foundation::HashDigest decision_id{};
    foundation::Timestamp recorded_at{};
    std::string payload{};
};

// Append-only shadow ledger (V3-22: the persistent ledger is the historical
// source of truth).
//
// RT-0019 / Phase 1 deterministic runtime. Records are appended and never
// mutated, reordered or erased. Writes are idempotent on entry identity:
// re-appending an identical record is a no-op success, while a conflicting
// payload for the same identity is rejected. Entry identity is a deterministic
// function of type, decision id and payload, so the same record always yields
// the same identity. Persistence delegates to the append-oriented
// IPersistenceStore (PER-0003). Deterministic; no clock, no threads, no I/O
// beyond the injected store.
class ShadowLedger {
public:
    ShadowLedger() = default;

    // Appends a pre-built entry. Idempotent on entry_id: an identical duplicate
    // returns true without a second copy; a conflicting payload for the same
    // entry_id returns false with no change. An entry without a valid identity or
    // a known type is rejected.
    bool append(LedgerEntry entry) {
        if (!entry.entry_id.valid() || entry.type == LedgerEntryType::UNKNOWN) return false;
        const auto it = entries_.find(entry.entry_id);
        if (it != entries_.end()) {
            return it->second.payload == entry.payload && it->second.decision_id == entry.decision_id;
        }
        order_.push_back(entry.entry_id);
        entries_.emplace(entry.entry_id, std::move(entry));
        return true;
    }

    // Builds and appends a deterministic entry from its fields.
    bool record(LedgerEntryType type, const foundation::HashDigest& decision_id,
                foundation::Timestamp recorded_at, std::string payload) {
        LedgerEntry entry;
        entry.type = type;
        entry.decision_id = decision_id;
        entry.recorded_at = recorded_at;
        entry.payload = std::move(payload);
        entry.entry_id = make_entry_id(type, decision_id, entry.payload);
        if (!entry.entry_id.valid()) return false;
        return append(std::move(entry));
    }

    bool record_signal(const Signal& signal, foundation::Timestamp recorded_at) {
        if (!signal.valid) return false;
        const std::string payload = std::string(to_string(signal.family)) + "|" +
                                    std::string(to_string(signal.direction)) + "|" +
                                    signal.closed_bar.canonical_string();
        return record(LedgerEntryType::SIGNAL, signal.decision_id, recorded_at, payload);
    }

    bool record_fill(const SimulatedFill& fill, foundation::Timestamp recorded_at) {
        if (!fill.valid) return false;
        const std::string payload = std::string(to_string(fill.direction)) + "|" +
                                    std::to_string(fill.filled_size) + "|" +
                                    std::to_string(fill.fill_price);
        return record(LedgerEntryType::SIMULATED_FILL, fill.decision_id, recorded_at, payload);
    }

    bool record_position(const SimulatedPosition& position, foundation::Timestamp recorded_at) {
        if (!position.valid) return false;
        const std::string payload = std::string(to_string(position.state)) + "|" +
                                    std::to_string(position.size) + "|" +
                                    std::to_string(position.closed_size) + "|" +
                                    std::to_string(position.realised_pnl) + "|" +
                                    std::to_string(position.costs);
        return record(LedgerEntryType::POSITION, position.decision_id, recorded_at, payload);
    }

    bool contains(const foundation::EntityId& entry_id) const {
        return entries_.find(entry_id) != entries_.end();
    }

    const LedgerEntry* find(const foundation::EntityId& entry_id) const {
        const auto it = entries_.find(entry_id);
        return it == entries_.end() ? nullptr : &it->second;
    }

    std::size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }

    // Entries in append order (deterministic, not sorted).
    std::vector<LedgerEntry> ordered() const {
        std::vector<LedgerEntry> out;
        out.reserve(order_.size());
        for (const foundation::EntityId& id : order_) out.push_back(entries_.at(id));
        return out;
    }

    // Persists every entry to the store. Idempotent: re-persisting unchanged
    // entries appends nothing new. Returns the worst status observed.
    foundation::PersistenceStatus persist(foundation::IPersistenceStore& store,
                                          const foundation::SchemaVersion& schema) const {
        foundation::PersistenceStatus worst = foundation::PersistenceStatus::OK;
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return foundation::PersistenceStatus::UNAVAILABLE;
        for (const foundation::EntityId& id : order_) {
            const LedgerEntry& entry = entries_.at(id);
            const foundation::HashDigest digest = hasher->hash(entry.payload);
            if (digest.empty()) return foundation::PersistenceStatus::FAILED;
            const foundation::PersistenceStatus status = store.append(
                foundation::PersistenceRecordMetadata(entry.entry_id, schema, entry.recorded_at), digest);
            if (!foundation::is_success(status) && status != foundation::PersistenceStatus::CONFLICT) {
                worst = status;
            }
        }
        return worst;
    }

    // Deterministic entry identity: Hash(type | decision id | payload).
    static foundation::EntityId make_entry_id(LedgerEntryType type,
                                              const foundation::HashDigest& decision_id,
                                              const std::string& payload) {
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return {};
        const std::string material = std::string(to_string(type)) + "|" + decision_id.to_hex() + "|" + payload;
        const foundation::HashDigest digest = hasher->hash(material);
        if (digest.empty()) return {};
        return foundation::EntityId(std::string("ledger|") + digest.to_hex());
    }

private:
    std::map<foundation::EntityId, LedgerEntry> entries_{};
    std::vector<foundation::EntityId> order_{};
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_SHADOWLEDGER_H
