#ifndef AURA_RESILIENCE_DEPENDENCYGRAPH_H
#define AURA_RESILIENCE_DEPENDENCYGRAPH_H

#include "resilience/CapabilityId.h"
#include "resilience/DependencyDescriptor.h"

#include <cstddef>
#include <map>
#include <set>
#include <vector>

namespace aura {
namespace resilience {

// Directed capability dependency graph used for impact resolution.
//
// RS-0010 / Phase 0.5 resilience foundation. Encodes the capability dependency
// graph explicitly (V3-15) rather than implying it. Edges are directed
// (`dependent` requires `dependency`) and self-edges are rejected. Traversal is
// deterministic: nodes and adjacency are held in ordered containers, so iteration
// and derived orders do not depend on insertion order. The graph is cycle-safe:
// `has_cycle()` and `topological_order()` never loop on a cyclic graph. This type
// stores and traverses edges only; it performs no health evaluation, applies no
// isolation and performs no I/O.
class DependencyGraph {
public:
    DependencyGraph() = default;

    // Adds a directed edge. Returns false, with no change, when the edge is
    // invalid (including a self-edge) or already present.
    bool add_edge(DependencyDescriptor edge) {
        if (!edge.valid()) return false;
        // Ensure both endpoints exist as nodes.
        adjacency_.emplace(edge.dependent(), std::vector<CapabilityId>{});
        adjacency_.emplace(edge.dependency(), std::vector<CapabilityId>{});
        reverse_.emplace(edge.dependency(), std::vector<CapabilityId>{});
        reverse_.emplace(edge.dependent(), std::vector<CapabilityId>{});

        std::vector<CapabilityId>& deps = adjacency_[edge.dependent()];
        if (contains_sorted(deps, edge.dependency())) return false;
        insert_sorted(deps, edge.dependency());

        std::vector<CapabilityId>& dependents = reverse_[edge.dependency()];
        insert_sorted(dependents, edge.dependent());
        return true;
    }

    bool contains(const CapabilityId& node) const {
        return adjacency_.find(node) != adjacency_.end();
    }

    std::size_t node_count() const noexcept { return adjacency_.size(); }

    // All nodes in deterministic (sorted) order.
    std::vector<CapabilityId> nodes() const {
        std::vector<CapabilityId> result;
        result.reserve(adjacency_.size());
        for (const auto& entry : adjacency_) result.push_back(entry.first);
        return result;
    }

    // Direct capabilities that `node` depends on, sorted.
    std::vector<CapabilityId> dependencies_of(const CapabilityId& node) const {
        const auto it = adjacency_.find(node);
        return it == adjacency_.end() ? std::vector<CapabilityId>{} : it->second;
    }

    // Direct capabilities that depend on `node`, sorted.
    std::vector<CapabilityId> dependents_of(const CapabilityId& node) const {
        const auto it = reverse_.find(node);
        return it == reverse_.end() ? std::vector<CapabilityId>{} : it->second;
    }

    // Transitive set of capabilities that depend on `node` (the impact set), in
    // deterministic order. `node` itself is included only if it is reachable from
    // itself (i.e. part of a cycle).
    std::vector<CapabilityId> reachable_dependents(const CapabilityId& node) const {
        std::set<CapabilityId> visited;
        std::vector<CapabilityId> stack;
        const auto direct = reverse_.find(node);
        if (direct != reverse_.end()) {
            for (const CapabilityId& c : direct->second) stack.push_back(c);
        }
        while (!stack.empty()) {
            const CapabilityId current = stack.back();
            stack.pop_back();
            if (!visited.insert(current).second) continue;
            const auto it = reverse_.find(current);
            if (it != reverse_.end()) {
                for (const CapabilityId& c : it->second) stack.push_back(c);
            }
        }
        return std::vector<CapabilityId>(visited.begin(), visited.end());
    }

    // True when the graph contains a directed cycle.
    bool has_cycle() const { return topological_order().size() != node_count(); }

    // Deterministic topological order (dependencies before dependents). Returns an
    // empty vector when the graph contains a cycle.
    std::vector<CapabilityId> topological_order() const {
        std::map<CapabilityId, std::size_t> indegree;
        for (const auto& entry : adjacency_) indegree[entry.first] = entry.second.size();

        std::set<CapabilityId> ready;
        for (const auto& entry : indegree) {
            if (entry.second == 0) ready.insert(entry.first);
        }

        std::vector<CapabilityId> order;
        order.reserve(adjacency_.size());
        while (!ready.empty()) {
            const CapabilityId current = *ready.begin();
            ready.erase(ready.begin());
            order.push_back(current);
            const auto it = reverse_.find(current);
            if (it == reverse_.end()) continue;
            for (const CapabilityId& dependent : it->second) {
                auto deg = indegree.find(dependent);
                if (deg == indegree.end() || deg->second == 0) continue;
                if (--deg->second == 0) ready.insert(dependent);
            }
        }
        if (order.size() != adjacency_.size()) return std::vector<CapabilityId>{};
        return order;
    }

private:
    static bool contains_sorted(const std::vector<CapabilityId>& v, const CapabilityId& id) {
        for (const CapabilityId& c : v) {
            if (c == id) return true;
        }
        return false;
    }

    static void insert_sorted(std::vector<CapabilityId>& v, const CapabilityId& id) {
        auto pos = v.begin();
        while (pos != v.end() && *pos < id) ++pos;
        v.insert(pos, id);
    }

    std::map<CapabilityId, std::vector<CapabilityId>> adjacency_{};  // node -> dependencies
    std::map<CapabilityId, std::vector<CapabilityId>> reverse_{};    // node -> dependents
};

}  // namespace resilience
}  // namespace aura

#endif  // AURA_RESILIENCE_DEPENDENCYGRAPH_H
