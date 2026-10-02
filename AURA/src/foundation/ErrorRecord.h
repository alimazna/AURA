#ifndef AURA_FOUNDATION_ERRORRECORD_H
#define AURA_FOUNDATION_ERRORRECORD_H

#include "foundation/EntityId.h"
#include "foundation/ErrorCode.h"
#include "foundation/ErrorSeverity.h"
#include "foundation/RecoveryAction.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"

#include <string>
#include <utility>

namespace aura {
namespace foundation {

// Immutable structured error record (V3-27, section 113).
//
// FND-0016 / Phase 0 immutable foundation. Mirrors the exact eight-field error
// shape named by the Master V3:
//
//     error_id, component, severity, timestamp, state, message, context,
//     recovery_action
//
// Errors are structured records, not plain strings (V3-27). The record is a value
// contract only: it performs no logging, alerting, recovery, retry, evaluation,
// serialization, persistence, networking or trading. `severity` and
// `recovery_action` are carried as independent fields; this type neither derives
// one from the other nor evaluates a policy.
class ErrorRecord {
public:
    ErrorRecord() = default;

    ErrorRecord(EntityId error_id, std::string component, ErrorSeverity severity,
                Timestamp timestamp, ServiceState state, std::string message,
                std::string context, RecoveryAction recovery_action)
        : error_id_(std::move(error_id)),
          component_(std::move(component)),
          severity_(severity),
          timestamp_(timestamp),
          state_(state),
          message_(std::move(message)),
          context_(std::move(context)),
          recovery_action_(recovery_action) {}

    const EntityId& error_id() const noexcept { return error_id_; }
    const std::string& component() const noexcept { return component_; }
    ErrorSeverity severity() const noexcept { return severity_; }
    Timestamp timestamp() const noexcept { return timestamp_; }
    ServiceState state() const noexcept { return state_; }
    const std::string& message() const noexcept { return message_; }
    const std::string& context() const noexcept { return context_; }
    RecoveryAction recovery_action() const noexcept { return recovery_action_; }

    // A record is attributable only when it carries its own error identity and
    // names the component that produced it. Severity, state and recovery_action
    // may legitimately hold any value of their frozen enumerations; in
    // particular RecoveryAction::UNKNOWN is representable and must not be treated
    // as safe (V3-05, V3-13).
    bool valid() const noexcept { return error_id_.valid() && !component_.empty(); }

private:
    EntityId error_id_{};
    std::string component_{};
    ErrorSeverity severity_{ErrorSeverity::INFO};
    Timestamp timestamp_{};
    ServiceState state_{ServiceState::STARTING};
    std::string message_{};
    std::string context_{};
    RecoveryAction recovery_action_{RecoveryAction::UNKNOWN};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_ERRORRECORD_H
