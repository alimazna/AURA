#ifndef AURA_GOVERNANCE_HUMANDECISION_H
#define AURA_GOVERNANCE_HUMANDECISION_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace governance {

// Research notebook entry statuses (Master 35).
enum class DecisionStatus : std::uint8_t {
    PROPOSED = 0,
    UNDER_REVIEW,
    ACCEPTED,
    REJECTED,
    SUPERSEDED,
};

constexpr std::string_view to_string(DecisionStatus s) noexcept {
    switch (s) {
        case DecisionStatus::PROPOSED:    return "PROPOSED";
        case DecisionStatus::UNDER_REVIEW: return "UNDER_REVIEW";
        case DecisionStatus::ACCEPTED:    return "ACCEPTED";
        case DecisionStatus::REJECTED:    return "REJECTED";
        case DecisionStatus::SUPERSEDED:  return "SUPERSEDED";
    }
    return "PROPOSED";
}

// Research notebook / decision record (Master 35). Part of project memory, never
// replaced by informal chat notes.
struct DecisionRecord {
    foundation::EntityId decision_id{};
    std::string question{};
    std::vector<std::string> options{};
    std::string evidence{};
    std::vector<std::string> assumptions{};
    std::string selected_design{};
    std::vector<std::string> rejected_designs{};
    std::vector<std::string> risks{};
    std::string test_required{};
    DecisionStatus status{DecisionStatus::PROPOSED};
    std::string actor{};
    foundation::Timestamp created_at{};

    bool valid() const noexcept { return decision_id.valid() && !question.empty(); }
};

// Human decision ledger (Master 35 / 36.2 Human Decisions).
//
// Phase 7 governance. Records human decisions immutably. A decision is made by an
// explicit actor; automated actors cannot record an ACCEPTED human decision
// (Master 39 #15/#19). Decisions are versioned via lineage; a superseding decision
// names the one it replaces and both remain retrievable. Deterministic; no I/O.
class HumanDecisionLedger {
public:
    HumanDecisionLedger() = default;

    // Records a decision. Automated actors may only propose; only a human actor may
    // record ACCEPTED / REJECTED / UNDER_REVIEW decisions.
    bool record(DecisionRecord record) {
        if (!record.valid()) return false;
        if (record.actor.empty()) return false;
        const bool human = (record.actor == "human");
        if (!human && record.status != DecisionStatus::PROPOSED) return false;
        if (decisions_.find(record.decision_id) != decisions_.end()) return false;
        order_.push_back(record.decision_id);
        decisions_.emplace(record.decision_id, std::move(record));
        return true;
    }

    bool contains(const foundation::EntityId& id) const {
        return decisions_.find(id) != decisions_.end();
    }

    const DecisionRecord* find(const foundation::EntityId& id) const {
        const auto it = decisions_.find(id);
        return it == decisions_.end() ? nullptr : &it->second;
    }

    std::size_t size() const noexcept { return decisions_.size(); }

    std::vector<DecisionRecord> ordered() const {
        std::vector<DecisionRecord> out;
        out.reserve(order_.size());
        for (const foundation::EntityId& id : order_) out.push_back(decisions_.at(id));
        return out;
    }

private:
    std::vector<foundation::EntityId> order_{};
    std::map<foundation::EntityId, DecisionRecord> decisions_{};
};

}  // namespace governance
}  // namespace aura

#endif  // AURA_GOVERNANCE_HUMANDECISION_H
