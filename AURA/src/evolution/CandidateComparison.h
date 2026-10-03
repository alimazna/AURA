#ifndef AURA_EVOLUTION_CANDIDATECOMPARISON_H
#define AURA_EVOLUTION_CANDIDATECOMPARISON_H

#include "evolution/Candidate.h"

#include <cstdint>
#include <string_view>

namespace aura {
namespace evolution {

// Comparison verdict (Master 11.2 / 17).
enum class ComparisonVerdict : std::uint8_t {
    INSUFFICIENT_EVIDENCE = 0,
    CONTROL_BETTER,      // candidate regressed
    CANDIDATE_BETTER,    // candidate improved on the primary metric
    EQUIVALENT,          // within tolerance
    VETOED,              // a hard safety constraint failed
};

constexpr std::string_view to_string(ComparisonVerdict v) noexcept {
    switch (v) {
        case ComparisonVerdict::INSUFFICIENT_EVIDENCE: return "INSUFFICIENT_EVIDENCE";
        case ComparisonVerdict::CONTROL_BETTER:        return "CONTROL_BETTER";
        case ComparisonVerdict::CANDIDATE_BETTER:      return "CANDIDATE_BETTER";
        case ComparisonVerdict::EQUIVALENT:            return "EQUIVALENT";
        case ComparisonVerdict::VETOED:                return "VETOED";
    }
    return "INSUFFICIENT_EVIDENCE";
}

// Matched comparison inputs. Fields mirror the "matched" conditions (11.2):
// data, environment, time window, evaluator, transaction assumptions, seed policy.
struct ComparisonInput {
    double control_primary{0.0};
    double candidate_primary{0.0};
    double improvement_tolerance{0.0};
    // Hard safety constraints are vetoes, not optimizers (Master 17).
    bool safety_constraints_satisfied{true};
    // Matched conditions: if any differ, the comparison is not controlled.
    bool data_matched{true};
    bool environment_matched{true};
    bool window_matched{true};
    bool evaluator_matched{true};
    bool seed_policy_matched{true};
    bool enough_trials{true};
};

// Candidate comparison (Master 11.2/17 "constraints are vetoes, metrics are optimizers").
//
// Phase 5 evolution. Compares a control (current approved version) against a
// candidate under matched conditions. A safety-constraint failure vetoes the
// candidate regardless of metric gain. A non-matched comparison or too few trials
// yields INSUFFICIENT_EVIDENCE rather than a favorable verdict. Pure/deterministic.
class CandidateComparison {
public:
    static ComparisonVerdict compare(const ComparisonInput& in) noexcept {
        if (!in.safety_constraints_satisfied) return ComparisonVerdict::VETOED;
        if (!in.data_matched || !in.environment_matched || !in.window_matched ||
            !in.evaluator_matched || !in.seed_policy_matched || !in.enough_trials) {
            return ComparisonVerdict::INSUFFICIENT_EVIDENCE;
        }
        const double delta = in.candidate_primary - in.control_primary;
        if (delta > in.improvement_tolerance) return ComparisonVerdict::CANDIDATE_BETTER;
        if (delta < -in.improvement_tolerance) return ComparisonVerdict::CONTROL_BETTER;
        return ComparisonVerdict::EQUIVALENT;
    }

    // A candidate is never auto-promoted: even a CANDIDATE_BETTER verdict only
    // permits proceeding to review, not promotion (Master 31 promotion gate).
    static bool permits_review(ComparisonVerdict v) noexcept {
        return v == ComparisonVerdict::CANDIDATE_BETTER || v == ComparisonVerdict::EQUIVALENT;
    }
};

}  // namespace evolution
}  // namespace aura

#endif  // AURA_EVOLUTION_CANDIDATECOMPARISON_H
