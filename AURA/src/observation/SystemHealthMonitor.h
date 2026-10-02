#ifndef AURA_OBSERVATION_SYSTEMHEALTHMONITOR_H
#define AURA_OBSERVATION_SYSTEMHEALTHMONITOR_H

#include "foundation/ErrorRecord.h"
#include "foundation/ServiceState.h"
#include "foundation/Timestamp.h"
#include "observation/FailureDetectionEngine.h"
#include "runtime/AdapterManager.h"

#include <cstddef>
#include <vector>

namespace aura {
namespace observation {

// Deterministic aggregate system-health snapshot.
struct HealthReport {
    foundation::ServiceState aggregate{foundation::ServiceState::STARTING};
    bool healthy{false};
    std::size_t online{0};
    std::size_t total{0};
    std::vector<foundation::ErrorRecord> failures{};
    bool valid{false};
};

// Aggregates subsystem health deterministically.
//
// OB-0004 / Phase 2 observation foundation. Observes the runtime adapter streams
// and the structured failures reported by the failure-detection engine (OB-0003).
// The aggregate state is the worst observed state (a single OFFLINE/BLOCKED/ERROR
// stream cannot be hidden by healthy peers) and the report is healthy only when
// every observed stream is ONLINE and no failure record is present. An empty or
// unassessed system is reported as STARTING and not healthy: the monitor never
// fabricates a healthy state. Deterministic and pure; no I/O, no clock.
class SystemHealthMonitor {
public:
    SystemHealthMonitor() = default;

    HealthReport observe(const runtime::AdapterManager& adapters,
                         foundation::Timestamp observed_at) const {
        HealthReport report;
        report.total = adapters.stream_count();

        for (const auto& entry : adapters.streams()) {
            const runtime::StreamStatus& stream = entry.second;
            if (stream.healthy()) ++report.online;
            report.aggregate = worst(report.aggregate, stream.state);
        }
        report.failures = failure_engine_.detect_adapter_failures(adapters, observed_at);
        if (!report.failures.empty()) report.aggregate = worst(report.aggregate, foundation::ServiceState::DEGRADED);

        report.healthy = report.total > 0 && report.online == report.total && report.failures.empty();
        if (report.healthy) report.aggregate = foundation::ServiceState::ONLINE;
        report.valid = true;
        return report;
    }

    // Deterministic severity ordering over ServiceState (higher is worse).
    static int rank(foundation::ServiceState state) noexcept {
        switch (state) {
            case foundation::ServiceState::ONLINE:     return 0;
            case foundation::ServiceState::STARTING:   return 1;
            case foundation::ServiceState::PAUSED:     return 2;
            case foundation::ServiceState::RECOVERING: return 3;
            case foundation::ServiceState::DEGRADED:   return 4;
            case foundation::ServiceState::OFFLINE:    return 5;
            case foundation::ServiceState::ERROR:      return 6;
            case foundation::ServiceState::BLOCKED:    return 7;
        }
        return 1;
    }

    static foundation::ServiceState worst(foundation::ServiceState a,
                                          foundation::ServiceState b) noexcept {
        return rank(a) >= rank(b) ? a : b;
    }

private:
    FailureDetectionEngine failure_engine_{};
};

}  // namespace observation
}  // namespace aura

#endif  // AURA_OBSERVATION_SYSTEMHEALTHMONITOR_H
