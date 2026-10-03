#ifndef AURA_REALWORLD_VALIDATIONREGISTRY_H
#define AURA_REALWORLD_VALIDATIONREGISTRY_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace realworld {

// Outcome of a controlled validation run.
enum class RunOutcome : std::uint8_t {
    SCHEDULED = 0,
    IN_PROGRESS,
    COMPLETED_SIMULATED,   // shadow/simulation only
    INCONCLUSIVE,
    ABORTED,
    ROLLED_BACK,
};

constexpr std::string_view to_string(RunOutcome o) noexcept {
    switch (o) {
        case RunOutcome::SCHEDULED:          return "SCHEDULED";
        case RunOutcome::IN_PROGRESS:        return "IN_PROGRESS";
        case RunOutcome::COMPLETED_SIMULATED: return "COMPLETED_SIMULATED";
        case RunOutcome::INCONCLUSIVE:       return "INCONCLUSIVE";
        case RunOutcome::ABORTED:            return "ABORTED";
        case RunOutcome::ROLLED_BACK:        return "ROLLED_BACK";
    }
    return "SCHEDULED";
}

// A controlled validation run record. `broker_profile` names the measured broker,
// never an assumed equivalence. `evidence` is a reference, not a claim.
struct ValidationRun {
    foundation::EntityId run_id{};
    std::string campaign_id{};
    std::string broker_profile{};
    std::string environment_version{};
    std::string shadow_evidence_ref{};
    RunOutcome outcome{RunOutcome::SCHEDULED};
    std::string notes{};
    foundation::Timestamp started_at{};

    bool valid() const noexcept { return run_id.valid() && !campaign_id.empty(); }
};

// Append-only controlled-validation registry (Phase 11).
//
// Records controlled real-world validation runs immutably for audit. It does not
// execute trades, does not enable live trading, and does not upgrade a run to a
// profitability/safety claim. Deterministic; no I/O, no clock.
class ValidationRegistry {
public:
    ValidationRegistry() = default;

    bool record(ValidationRun run) {
        if (!run.valid()) return false;
        if (by_id_.find(run.run_id) != by_id_.end()) return false;
        order_.push_back(run.run_id);
        by_id_.emplace(run.run_id, std::move(run));
        return true;
    }

    bool contains(const foundation::EntityId& id) const { return by_id_.find(id) != by_id_.end(); }

    const ValidationRun* find(const foundation::EntityId& id) const {
        const auto it = by_id_.find(id);
        return it == by_id_.end() ? nullptr : &it->second;
    }

    std::size_t size() const noexcept { return by_id_.size(); }

    std::vector<ValidationRun> ordered() const {
        std::vector<ValidationRun> out;
        out.reserve(order_.size());
        for (const auto& id : order_) out.push_back(by_id_.at(id));
        return out;
    }

private:
    std::vector<foundation::EntityId> order_{};
    std::map<foundation::EntityId, ValidationRun> by_id_{};
};

}  // namespace realworld
}  // namespace aura

#endif  // AURA_REALWORLD_VALIDATIONREGISTRY_H
