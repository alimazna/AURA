#ifndef AURA_RESILIENCE_PAUSERESUMEMANAGER_H
#define AURA_RESILIENCE_PAUSERESUMEMANAGER_H

#include "foundation/ServiceState.h"
#include "foundation/SystemMode.h"
#include "resilience/CapabilityId.h"
#include "resilience/CapabilityRegistry.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace resilience {

// Tracks which capabilities are deliberately paused and why.
//
// RS-0017 / Phase 0.5 resilience foundation. Pause/resume are bounded by
// authority: they are only meaningful while the system mode still permits
// decision production (foundation::allows_decision_production), and every pause
// carries a non-empty reason so the exact capability and cause are attributable
// (V3-15). Pausing records intent only; it changes no health, disables no
// capability and performs no I/O. Paused is distinct from failed: resume is not
// recovery and a paused capability is not unhealthy.
class PauseResumeManager {
public:
    using reason_map = std::map<CapabilityId, std::string>;

    explicit PauseResumeManager(const CapabilityRegistry& registry) : registry_(registry) {}

    // Requests a pause. Succeeds only when the capability is registered, the
    // reason is non-empty, the mode permits decision production, and the
    // capability is not already paused. Grants no authority.
    bool pause(const CapabilityId& capability, std::string reason, foundation::SystemMode mode) {
        if (!registry_.contains(capability)) return false;
        if (reason.empty()) return false;
        if (!foundation::allows_decision_production(mode)) return false;
        return paused_.emplace(capability, std::move(reason)).second;
    }

    // Requests a resume. Succeeds only when the capability is currently paused,
    // the reason is non-empty, and the mode permits decision production.
    bool resume(const CapabilityId& capability, const std::string& reason, foundation::SystemMode mode) {
        if (reason.empty()) return false;
        if (!foundation::allows_decision_production(mode)) return false;
        const auto it = paused_.find(capability);
        if (it == paused_.end()) return false;
        paused_.erase(it);
        return true;
    }

    bool is_paused(const CapabilityId& capability) const {
        return paused_.find(capability) != paused_.end();
    }

    // The reason a capability is paused, or nullptr when it is not paused.
    const std::string* reason_for(const CapabilityId& capability) const {
        const auto it = paused_.find(capability);
        return it == paused_.end() ? nullptr : &it->second;
    }

    std::size_t paused_count() const noexcept { return paused_.size(); }

    // What is paused and why, in deterministic capability order, so a reporting
    // layer can present it without re-deriving intent.
    std::vector<std::pair<CapabilityId, std::string>> what_paused() const {
        return std::vector<std::pair<CapabilityId, std::string>>(paused_.begin(), paused_.end());
    }

private:
    const CapabilityRegistry& registry_;
    reason_map paused_{};
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_PAUSERESUMEMANAGER_H
