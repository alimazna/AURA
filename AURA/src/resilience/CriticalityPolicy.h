#ifndef AURA_RESILIENCE_CRITICALITYPOLICY_H
#define AURA_RESILIENCE_CRITICALITYPOLICY_H

#include "resilience/CapabilityDescriptor.h"

#include <cstddef>
#include <map>
#include <string_view>

namespace aura {
namespace resilience {

// Critical-vs-non-critical capability policy.
//
// RS-0008 / Phase 0.5 resilience foundation. Classifies capabilities so the
// isolation/degradation managers can decide which failures may be tolerated. The
// classification is explicit and closed: there is no silent default. A capability
// that is not classified is UNCLASSIFIED and must not be assumed critical or
// non-critical; callers must resolve it explicitly. This is a value contract
// only: it applies no policy and performs no I/O.
enum class Criticality : std::uint8_t {
    UNCLASSIFIED = 0,
    NON_CRITICAL,
    CRITICAL,
};

constexpr std::string_view to_string(Criticality criticality) noexcept {
    switch (criticality) {
        case Criticality::UNCLASSIFIED: return "UNCLASSIFIED";
        case Criticality::NON_CRITICAL: return "NON_CRITICAL";
        case Criticality::CRITICAL:     return "CRITICAL";
    }
    return "UNCLASSIFIED";
}

// An explicit classification is required before a criticality decision is
// trustworthy; UNCLASSIFIED is never a silent default.
constexpr bool is_classified(Criticality criticality) noexcept {
    return criticality != Criticality::UNCLASSIFIED;
}

// Immutable, deterministic set of explicit capability classifications.
class CriticalityPolicy {
public:
    using classification_map = std::map<CapabilityId, Criticality>;

    CriticalityPolicy() = default;

    explicit CriticalityPolicy(classification_map classifications)
        : classifications_(std::move(classifications)) {}

    // Returns the explicit classification, or UNCLASSIFIED when the capability
    // has no explicit classification. Never returns a silent default.
    Criticality classify(const CapabilityId& capability) const {
        const auto it = classifications_.find(capability);
        return it == classifications_.end() ? Criticality::UNCLASSIFIED : it->second;
    }

    bool is_critical(const CapabilityId& capability) const {
        return classify(capability) == Criticality::CRITICAL;
    }

    bool contains(const CapabilityId& capability) const {
        return classifications_.find(capability) != classifications_.end();
    }

    std::size_t size() const noexcept { return classifications_.size(); }
    bool empty() const noexcept { return classifications_.empty(); }
    const classification_map& classifications() const noexcept { return classifications_; }

private:
    classification_map classifications_{};
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_CRITICALITYPOLICY_H
