#ifndef AURA_GOVERNANCE_AUDITLEDGER_H
#define AURA_GOVERNANCE_AUDITLEDGER_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace governance {

// Audit event category (Master 34.1 Audit Ledger / V3-31).
enum class AuditCategory {
    DECISION = 0,
    PROMOTION,
    ROLLBACK,
    RESEARCH_ACTION,
    GOVERNANCE_SCREEN,
    INCIDENT,
    CONFIGURATION,
    OBSERVATION,
};

// An immutable audit record.
struct AuditRecord {
    foundation::EntityId audit_id{};
    AuditCategory category{AuditCategory::OBSERVATION};
    std::string actor{};
    std::string action{};
    std::string subject_ref{};
    std::string detail{};
    foundation::Timestamp recorded_at{};

    bool valid() const noexcept { return audit_id.valid() && !actor.empty() && !action.empty(); }
};

// Append-only audit ledger (Master 34.1 / V3-31 / V3-36 auditability).
//
// Phase 7 governance. Records every material action immutably. There is no API to
// edit or delete: history is append-only. Writes are idempotent on audit identity.
// Recording is deterministic and never depends on wall-clock; the caller supplies
// the timestamp, so records are reproducible. This layer never mutates runtime.
class AuditLedger {
public:
    AuditLedger() = default;

    bool append(AuditRecord record) {
        if (!record.valid()) return false;
        if (by_id_.find(record.audit_id) != by_id_.end()) {
            return false;  // duplicate identity rejected; no overwrite
        }
        order_.push_back(record.audit_id);
        by_id_.emplace(record.audit_id, std::move(record));
        return true;
    }

    bool contains(const foundation::EntityId& id) const {
        return by_id_.find(id) != by_id_.end();
    }

    std::size_t size() const noexcept { return by_id_.size(); }

    std::vector<AuditRecord> ordered() const {
        std::vector<AuditRecord> out;
        out.reserve(order_.size());
        for (const foundation::EntityId& id : order_) out.push_back(by_id_.at(id));
        return out;
    }

    std::size_t count_category(AuditCategory category) const {
        std::size_t n = 0;
        for (const auto& e : by_id_) {
            if (e.second.category == category) ++n;
        }
        return n;
    }

    // There is deliberately no remove()/clear()/update() method: the ledger is
    // append-only by construction (Master 39 #06/#21).
    static bool supports_mutation() noexcept { return false; }

private:
    std::vector<foundation::EntityId> order_{};
    std::map<foundation::EntityId, AuditRecord> by_id_{};
};

}  // namespace governance
}  // namespace aura

#endif  // AURA_GOVERNANCE_AUDITLEDGER_H
