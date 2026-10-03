#ifndef AURA_EVOLUTION_CANDIDATEREGISTRY_H
#define AURA_EVOLUTION_CANDIDATE_REGISTRY_H

#include "foundation/EntityId.h"
#include "evolution/Candidate.h"

#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace evolution {

// Append-only candidate registry (Master 12.1 population / 18 lifecycle).
//
// Phase 5 evolution. Retains a diverse population; a weak branch is not deleted
// (it may remain informative negative evidence). Candidate records are immutable
// once registered: lifecycle advancement produces a new revision recorded against
// the same identity, preserving history. Writes are idempotent on (identity,
// revision). No automatic promotion: state advances only through explicit calls,
// never because a metric improved. Deterministic; no clock, no I/O.
class CandidateRegistry {
public:
    CandidateRegistry() = default;

    // Registers a new candidate (revision 0). Returns false for invalid candidates,
    // duplicate identity, or an initial state other than PROPOSED.
    bool propose(Candidate candidate) {
        if (!candidate.valid()) return false;
        if (candidate.state != CandidateState::PROPOSED) return false;
        auto it = chains_.find(candidate.candidate_id);
        if (it != chains_.end() && !it->second.empty()) return false;  // already exists
        candidate.revision = 0;
        order_.push_back(candidate.candidate_id);
        chains_[candidate.candidate_id].push_back(std::move(candidate));
        return true;
    }

    // Advances a candidate's lifecycle state, appending a new revision. Rejected
    // when the candidate is unknown or the transition is illegal per CandidateLifecycle.
    bool advance(const foundation::EntityId& id, CandidateState to) {
        auto it = chains_.find(id);
        if (it == chains_.end() || it->second.empty()) return false;
        const Candidate& current = it->second.back();
        if (!CandidateLifecycle::can_transition(current.state, to)) return false;
        Candidate next = current;
        next.state = to;
        next.revision = current.revision + 1;
        it->second.push_back(std::move(next));
        return true;
    }

    bool contains(const foundation::EntityId& id) const { return chains_.find(id) != chains_.end(); }

    const Candidate* latest(const foundation::EntityId& id) const {
        const auto it = chains_.find(id);
        if (it == chains_.end() || it->second.empty()) return nullptr;
        return &it->second.back();
    }

    std::vector<Candidate> history(const foundation::EntityId& id) const {
        std::vector<Candidate> out;
        const auto it = chains_.find(id);
        if (it == chains_.end()) return out;
        out.assign(it->second.begin(), it->second.end());
        return out;
    }

    // Distinct candidate identities in deterministic (sorted) order.
    std::vector<foundation::EntityId> identities() const {
        std::vector<foundation::EntityId> out;
        out.reserve(chains_.size());
        for (const auto& e : chains_) out.push_back(e.first);
        return out;
    }

    std::size_t population() const noexcept { return chains_.size(); }

    // Management counters used by the evolution graph/report.
    std::size_t count_in_state(CandidateState state) const {
        std::size_t n = 0;
        for (const auto& e : chains_) {
            if (!e.second.empty() && e.second.back().state == state) ++n;
        }
        return n;
    }

private:
    std::vector<foundation::EntityId> order_{};
    std::map<foundation::EntityId, std::vector<Candidate>> chains_{};
};

}  // namespace evolution
}  // namespace aura

#endif  // AURA_EVOLUTION_CANDIDATEREGISTRY_H
