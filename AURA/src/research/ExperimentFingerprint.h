#ifndef AURA_RESEARCH_EXPERIMENTFINGERPRINT_H
#define AURA_RESEARCH_EXPERIMENTFINGERPRINT_H

#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "research/Experiment.h"

#include <memory>
#include <string>

namespace aura {
namespace research {

// Deterministic experiment fingerprint (Master 20.2 / reproducibility).
//
// Phase 4 research plane. A fingerprint is a SHA-256 digest over the stable
// identity-relevant fields of an experiment (question, versions, candidate
// definition, validation protocol, seed policy, budget) but excludes transient
// fields such as timestamps and results. Two experiments with the same fingerprint
// are the same experiment, so re-running one is detectable and idempotent.
// Pure: no clock, no state, no I/O.
class ExperimentFingerprint {
public:
    static std::string canonical(const Experiment& e) {
        std::string s;
        s += "campaign=" + e.campaign_id + "\n";
        s += "parent=" + e.parent_version + "\n";
        s += "hypothesis=" + e.hypothesis_id.value() + "\n";
        s += "failure=" + e.failure_id.value() + "\n";
        s += "question=" + e.question + "\n";
        s += "dataset=" + e.dataset_version + "\n";
        s += "feature=" + e.feature_version + "\n";
        s += "parameter_space=" + e.parameter_space + "\n";
        s += "generator=" + e.candidate_generator_version + "\n";
        s += "evaluator=" + e.evaluator_version + "\n";
        s += "metric_registry=" + e.metric_registry_version + "\n";
        s += "environment=" + e.environment_version + "\n";
        s += "control=" + e.control_version + "\n";
        s += "candidate=" + e.candidate_definition + "\n";
        s += "seed_policy=" + e.random_seed_policy + "\n";
        s += "protocol=" + e.validation_protocol + "\n";
        s += "budget=" + std::to_string(e.budget) + "\n";
        return s;
    }

    // Returns an empty digest only if the hasher is unavailable (never silently
    // substitutes a different algorithm).
    static foundation::HashDigest of(const Experiment& e) {
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return {};
        return hasher->hash(canonical(e));
    }

    static std::string of_hex(const Experiment& e) { return of(e).to_hex(); }
};

}  // namespace research
}  // namespace aura

#endif  // AURA_RESEARCH_EXPERIMENTFINGERPRINT_H
