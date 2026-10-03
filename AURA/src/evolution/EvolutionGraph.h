#ifndef AURA_EVOLUTION_EVOLUTIONGRAPH_H
#define AURA_EVOLUTION_EVOLUTIONGRAPH_H

#include "foundation/EntityId.h"
#include "evolution/Candidate.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace aura {
namespace evolution {

// A node in the evolution graph: a version and its explicit single parent.
struct EvolutionNode {
    std::string version{};
    std::string parent_version{};  // empty for a root
    foundation::EntityId candidate_id{};
    bool valid{false};
};

// Evolution graph (Master 33.1/33.2).
//
// Phase 5 evolution. Answers "where did this version come from?" Each child has
// exactly one explicit parent; many research branches may coexist. The graph is
// acyclic by construction: adding a node whose version equals an existing node is
// rejected, and a cycle (a parent that is not already present, except for a
// declared root) is rejected. Deterministic; no I/O.
class EvolutionGraph {
public:
    // Adds a root version (no parent). Fails if the version already exists.
    bool add_root(const std::string& version) {
        if (version.empty()) return false;
        if (nodes_.find(version) != nodes_.end()) return false;
        EvolutionNode n;
        n.version = version;
        n.valid = true;
        order_.push_back(version);
        nodes_.emplace(version, std::move(n));
        return true;
    }

    // Adds a child version with an explicit parent that must already exist. This
    // ordering requirement makes cycles impossible without a separate check.
    bool add_child(const std::string& parent_version, const std::string& child_version,
                   const foundation::EntityId& candidate_id = {}) {
        if (child_version.empty() || nodes_.find(child_version) != nodes_.end()) return false;
        const auto pit = nodes_.find(parent_version);
        if (pit == nodes_.end()) return false;  // unknown parent: rejected (no cycle)
        EvolutionNode n;
        n.version = child_version;
        n.parent_version = parent_version;
        n.candidate_id = candidate_id;
        n.valid = true;
        order_.push_back(child_version);
        nodes_.emplace(child_version, std::move(n));
        return true;
    }

    bool contains(const std::string& version) const { return nodes_.find(version) != nodes_.end(); }

    const EvolutionNode* find(const std::string& version) const {
        const auto it = nodes_.find(version);
        return it == nodes_.end() ? nullptr : &it->second;
    }

    // Root-to-node lineage, inclusive of both ends. Empty if the node is unknown.
    std::vector<std::string> lineage(const std::string& version) const {
        std::vector<std::string> out;
        const EvolutionNode* n = find(version);
        while (n != nullptr) {
            out.insert(out.begin(), n->version);
            if (n->parent_version.empty()) break;
            n = find(n->parent_version);
        }
        return out;
    }

    std::size_t size() const noexcept { return nodes_.size(); }

private:
    std::vector<std::string> order_{};
    std::map<std::string, EvolutionNode> nodes_{};
};

}  // namespace evolution
}  // namespace aura

#endif  // AURA_EVOLUTION_EVOLUTIONGRAPH_H
