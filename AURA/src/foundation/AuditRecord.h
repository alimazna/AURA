#ifndef AURA_FOUNDATION_AUDITRECORD_H
#define AURA_FOUNDATION_AUDITRECORD_H

#include "foundation/AuditAction.h"
#include "foundation/AuditOutcome.h"
#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <string>
#include <utility>

namespace aura {
namespace foundation {

// Immutable, append-only audit record (V3-27).
//
// AUD-0003 / Phase 0 audit foundation. Audit is append-only and historical
// records cannot be rewritten merely to make the current system look correct.
// An AuditRecord is therefore immutable and fully attributable: it carries its
// own identity, the action and outcome, the actor/component, a timestamp and an
// optional reference to the subject entity. It exposes no mutator, so a record
// cannot be edited after creation — only superseded by appending a new record.
class AuditRecord {
public:
    AuditRecord() = default;

    AuditRecord(EntityId audit_id, AuditAction action, AuditOutcome outcome, Timestamp timestamp,
                std::string actor, std::string component, EntityId subject, std::string detail)
        : audit_id_(std::move(audit_id)),
          action_(action),
          outcome_(outcome),
          timestamp_(timestamp),
          actor_(std::move(actor)),
          component_(std::move(component)),
          subject_(std::move(subject)),
          detail_(std::move(detail)) {}

    const EntityId& audit_id() const noexcept { return audit_id_; }
    AuditAction action() const noexcept { return action_; }
    AuditOutcome outcome() const noexcept { return outcome_; }
    Timestamp timestamp() const noexcept { return timestamp_; }
    const std::string& actor() const noexcept { return actor_; }
    const std::string& component() const noexcept { return component_; }
    const EntityId& subject() const noexcept { return subject_; }
    const std::string& detail() const noexcept { return detail_; }

    // A record is attributable only when it has its own identity, a known action
    // and a named actor. Outcome may legitimately be UNKNOWN.
    bool attributable() const noexcept {
        return audit_id_.valid() && action_ != AuditAction::UNKNOWN && !actor_.empty();
    }

private:
    EntityId audit_id_{};
    AuditAction action_{AuditAction::UNKNOWN};
    AuditOutcome outcome_{AuditOutcome::UNKNOWN};
    Timestamp timestamp_{};
    std::string actor_{};
    std::string component_{};
    EntityId subject_{};
    std::string detail_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_AUDITRECORD_H
