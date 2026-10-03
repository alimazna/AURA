#ifndef AURA_RESEARCH_EXPERIMENTLEDGER_H
#define AURA_RESEARCH_EXPERIMENTLEDGER_H

#include "foundation/EntityId.h"
#include "foundation/HashDigest.h"
#include "research/Experiment.h"
#include "research/ExperimentFingerprint.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace research {

// Append-only experiment ledger (Master 20 / 15.3).
//
// Phase 4 research plane. Records are appended and never mutated, reordered or
// erased: failed experiments are first-class objects (INVARIANT 08). Writes are
// idempotent on experiment identity and detect duplicate experiments by fingerprint
// (surface a DUPLICATE outcome rather than silently re-running). The ledger never
// alters runtime behaviour; it is a research record only. Deterministic; no clock,
// no threads, no I/O.
class ExperimentLedger {
public:
    ExperimentLedger() = default;

    // Records an experiment. Returns false for an invalid experiment or a
    // conflicting record under the same identity. An identical re-record is an
    // idempotent success.
    bool record(Experiment experiment) {
        if (!experiment.valid()) return false;
        const auto it = by_id_.find(experiment.experiment_id);
        if (it != by_id_.end()) {
            return same_content(it->second, experiment);
        }
        // Duplicate detection by fingerprint: a different identity that fingerprints
        // identically is the same experiment; mark the incoming one DUPLICATE and
        // preserve the first as the canonical record.
        const std::string fp = ExperimentFingerprint::of_hex(experiment);
        const auto dup = by_fingerprint_.find(fp);
        if (dup != by_fingerprint_.end() && !fp.empty()) {
            return false;  // caller learns it is a duplicate; original is preserved
        }
        if (!fp.empty()) by_fingerprint_.emplace(fp, experiment.experiment_id);
        order_.push_back(experiment.experiment_id);
        by_id_.emplace(experiment.experiment_id, std::move(experiment));
        return true;
    }

    bool contains(const foundation::EntityId& id) const {
        return by_id_.find(id) != by_id_.end();
    }

    const Experiment* find(const foundation::EntityId& id) const {
        const auto it = by_id_.find(id);
        return it == by_id_.end() ? nullptr : &it->second;
    }

    // True when an experiment with this fingerprint is already recorded.
    bool contains_fingerprint(const Experiment& e) const {
        const std::string fp = ExperimentFingerprint::of_hex(e);
        return !fp.empty() && by_fingerprint_.find(fp) != by_fingerprint_.end();
    }

    std::size_t size() const noexcept { return by_id_.size(); }

    std::vector<Experiment> ordered() const {
        std::vector<Experiment> out;
        out.reserve(order_.size());
        for (const foundation::EntityId& id : order_) out.push_back(by_id_.at(id));
        return out;
    }

private:
    static bool same_content(const Experiment& a, const Experiment& b) {
        return a.experiment_id == b.experiment_id && a.campaign_id == b.campaign_id &&
               a.question == b.question && a.candidate_definition == b.candidate_definition &&
               a.decision == b.decision && a.result == b.result &&
               a.dataset_version == b.dataset_version && a.evaluator_version == b.evaluator_version;
    }

    std::vector<foundation::EntityId> order_{};
    std::map<foundation::EntityId, Experiment> by_id_{};
    std::map<std::string, foundation::EntityId> by_fingerprint_{};
};

}  // namespace research
}  // namespace aura

#endif  // AURA_RESEARCH_EXPERIMENTLEDGER_H
