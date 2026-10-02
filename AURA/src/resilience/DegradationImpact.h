#ifndef AURA_RESILIENCE_DEGRADATIONIMPACT_H
#define AURA_RESILIENCE_DEGRADATIONIMPACT_H

#include "resilience/CapabilityId.h"

#include <cstddef>
#include <vector>

namespace aura {
namespace resilience {

// Immutable description of the impact of a capability degradation.
//
// RS-0007 / Phase 0.5 resilience foundation. States explicitly which
// capabilities are affected by a failure and which remain available. The two
// sets are kept distinct and are never inferred from one another: a capability
// is "available" only when it is explicitly listed as still-available. This is a
// descriptive value contract only; it applies nothing, disables nothing and
// performs no I/O. Impact is expressed as capability identity, not as UI-only
// behaviour (V3-15).
class DegradationImpact {
public:
    using capability_list = std::vector<CapabilityId>;

    DegradationImpact() = default;

    DegradationImpact(CapabilityId failed, capability_list affected, capability_list available)
        : failed_(failed), affected_(std::move(affected)), available_(std::move(available)) {}

    // The capability that failed, if known. May be invalid for an aggregate
    // impact that is not attributable to a single capability.
    const CapabilityId& failed() const noexcept { return failed_; }

    const capability_list& affected() const noexcept { return affected_; }
    const capability_list& available() const noexcept { return available_; }

    bool empty() const noexcept { return affected_.empty() && available_.empty(); }

    bool affects(const CapabilityId& id) const {
        for (const CapabilityId& c : affected_) {
            if (c == id) return true;
        }
        return false;
    }

    bool leaves_available(const CapabilityId& id) const {
        for (const CapabilityId& c : available_) {
            if (c == id) return true;
        }
        return false;
    }

    friend bool operator==(const DegradationImpact& a, const DegradationImpact& b) noexcept {
        return a.failed_ == b.failed_ && a.affected_ == b.affected_ && a.available_ == b.available_;
    }
    friend bool operator!=(const DegradationImpact& a, const DegradationImpact& b) noexcept {
        return !(a == b);
    }

private:
    CapabilityId failed_{};
    capability_list affected_{};
    capability_list available_{};
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_DEGRADATIONIMPACT_H
