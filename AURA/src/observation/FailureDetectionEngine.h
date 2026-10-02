#ifndef AURA_OBSERVATION_FAILUREDETECTIONENGINE_H
#define AURA_OBSERVATION_FAILUREDETECTIONENGINE_H

#include "foundation/ErrorCode.h"
#include "foundation/ErrorRecord.h"
#include "foundation/ErrorSeverity.h"
#include "foundation/RecoveryAction.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "observation/OutcomeEngine.h"
#include "observation/PredictionLedger.h"
#include "runtime/AdapterManager.h"

#include <cstddef>
#include <string>
#include <vector>

namespace aura {
namespace observation {

// Classification of a failure detected from observed behaviour.
//
// OB-0003 / Phase 2 observation foundation. Descriptive classification only; it
// does not itself assign severity or recovery action (those live on the structured
// error record).
enum class FailureCategory : std::uint8_t {
    UNKNOWN = 0,
    DATA_FAILURE,
    OUTCOME_FAILURE,
    PREDICTION_FAILURE,
    HEALTH_FAILURE,
};

constexpr std::string_view to_string(FailureCategory category) noexcept {
    switch (category) {
        case FailureCategory::UNKNOWN:            return "UNKNOWN";
        case FailureCategory::DATA_FAILURE:       return "DATA_FAILURE";
        case FailureCategory::OUTCOME_FAILURE:    return "OUTCOME_FAILURE";
        case FailureCategory::PREDICTION_FAILURE: return "PREDICTION_FAILURE";
        case FailureCategory::HEALTH_FAILURE:     return "HEALTH_FAILURE";
    }
    return "UNKNOWN";
}

// Structured failure detection.
//
// OB-0003 / Phase 2 observation foundation. Detects failures from observed data
// and resolves them into structured foundation::ErrorRecord objects (V3-27);
// failures are never plain strings. Each detector maps its condition to an
// explicit ErrorCode, ErrorSeverity and RecoveryAction so the record is
// attributable and auditable. The engine performs no logging, alerting, recovery
// or I/O; it produces records only. Severity and recovery_action are carried as
// independent fields and are not derived from one another by this engine beyond
// the explicit mapping shown.
class FailureDetectionEngine {
public:
    FailureDetectionEngine() = default;

    // Detects failures from adapter stream health. A non-ONLINE, non-STARTING
    // stream is a data failure; a stream that has never produced data is not
    // treated as healthy.
    std::vector<foundation::ErrorRecord> detect_adapter_failures(const runtime::AdapterManager& adapters,
                                                                 foundation::Timestamp observed_at) const {
        std::vector<foundation::ErrorRecord> records;
        for (const auto& entry : adapters.streams()) {
            const runtime::StreamStatus& stream = entry.second;
            // A healthy stream is fine; a STARTING stream has not yet been
            // assessed and is not itself a detected failure.
            if (stream.healthy() || stream.state == foundation::ServiceState::STARTING) continue;
            foundation::ErrorSeverity severity = foundation::ErrorSeverity::WARNING;
            foundation::RecoveryAction action = foundation::RecoveryAction::RETRY;
            if (stream.state == foundation::ServiceState::OFFLINE ||
                stream.state == foundation::ServiceState::ERROR) {
                severity = foundation::ErrorSeverity::ERROR;
                action = foundation::RecoveryAction::RETRY;
            } else if (stream.state == foundation::ServiceState::BLOCKED) {
                severity = foundation::ErrorSeverity::CRITICAL;
                action = foundation::RecoveryAction::SAFE_MODE;
            }
            records.push_back(make_record(
                FailureCategory::DATA_FAILURE, foundation::ErrorCode::DATA_ERROR, severity,
                observed_at, stream.state,
                std::string("stream ") + std::string(runtime::to_string(stream.timeframe)) + ": " +
                    (stream.last_error.empty() ? "not online" : stream.last_error),
                std::string("timeframe=") + std::string(runtime::to_string(stream.timeframe)), action));
        }
        return records;
    }

    // Detects an outcome failure: a prediction that resolved without a
    // determinable outcome (UNKNOWN) is a failure rather than a silent success.
    std::vector<foundation::ErrorRecord> detect_outcome_failures(
        const Prediction& prediction, const Outcome& outcome, foundation::Timestamp observed_at) const {
        std::vector<foundation::ErrorRecord> records;
        if (!prediction.valid || !outcome.valid) return records;
        if (outcome.state == OutcomeState::UNKNOWN) {
            records.push_back(make_record(
                FailureCategory::OUTCOME_FAILURE, foundation::ErrorCode::MODEL_ERROR,
                foundation::ErrorSeverity::WARNING, observed_at, foundation::ServiceState::DEGRADED,
                "prediction resolved with unknown outcome",
                std::string("prediction=") + prediction.prediction_id.value(),
                foundation::RecoveryAction::NONE));
        }
        return records;
    }

    // Builds a structured record for an explicit condition. Kept public so
    // callers can raise a structured failure without a plain string.
    static foundation::ErrorRecord make_record(FailureCategory category, foundation::ErrorCode code,
                                               foundation::ErrorSeverity severity,
                                               foundation::Timestamp timestamp,
                                               foundation::ServiceState state, std::string message,
                                               std::string context,
                                               foundation::RecoveryAction action) {
        const std::string component = std::string("failure.") + std::string(to_string(category)) + "." +
                                      std::string(foundation::to_string(code));
        const foundation::EntityId id(std::string("err|") + component + "|" + message + "|" +
                                      std::to_string(timestamp.nanoseconds()));
        return foundation::ErrorRecord(id, component, severity, timestamp, state, std::move(message),
                                       std::move(context), action);
    }
};

}  // namespace observation
}  // namespace aura

#endif  // AURA_OBSERVATION_FAILUREDETECTIONENGINE_H
