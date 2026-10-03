#ifndef AURA_RESEARCH_RESEARCHSANDBOX_H
#define AURA_RESEARCH_RESEARCHSANDBOX_H

#include "foundation/EntityId.h"
#include "research/Experiment.h"
#include "research/ExperimentLedger.h"
#include "research/ResearchPlanner.h"

#include <string>

namespace aura {
namespace research {

// Admission decision for a research artifact.
struct SandboxDecision {
    bool admitted{false};
    std::string reason{};
};

// Research sandbox boundary (Master 14/INVARIANT 01,03; V3-34 "research/validation
// firewalls"; V3-18 forbidden behaviour "direct production mutation").
//
// Phase 4 research plane. The sandbox is the boundary between research and the
// deterministic runtime. It admits an experiment only when the experiment is
// already recorded, references a hypothesis (INVARIANT 03 "hypothesis before
// change"), and carries a fixed validation plan. Critically, it can never mutate
// runtime behaviour: `may_mutate_runtime()` is always false and there is no API
// here that writes to the runtime plane.
class ResearchSandbox {
public:
    static bool may_mutate_runtime() noexcept { return false; }

    static SandboxDecision admit(const Experiment& experiment, const ExperimentLedger& ledger,
                                 const ValidationPlan& plan) {
        SandboxDecision d;
        if (!experiment.valid()) {
            d.reason = "invalid experiment";
            return d;
        }
        if (!experiment.hypothesis_id.valid()) {
            d.reason = "no hypothesis referenced (hypothesis-before-change required)";
            return d;
        }
        if (!ledger.contains(experiment.experiment_id)) {
            d.reason = "experiment not recorded in ledger";
            return d;
        }
        if (plan.empty() || plan.fixed_at.nanoseconds() == 0) {
            d.reason = "validation plan not fixed before evaluation";
            return d;
        }
        d.admitted = true;
        d.reason = "admitted to sandbox (runtime immutable)";
        return d;
    }
};

}  // namespace research
}  // namespace aura

#endif  // AURA_RESEARCH_RESEARCHSANDBOX_H
