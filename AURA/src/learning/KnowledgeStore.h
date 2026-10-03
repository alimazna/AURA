#ifndef AURA_LEARNING_KNOWLEDGESTORE_H
#define AURA_LEARNING_KNOWLEDGESTORE_H

#include "foundation/EntityId.h"
#include "learning/KnowledgeObject.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace aura {
namespace learning {

// Append-only, versioned knowledge store (Master V2-05/V2-06, V3-35 research memory).
//
// Phase 3 self-learning. Knowledge updates are versioned and historical knowledge
// is never silently overwritten: a superseding conclusion is stored as a new
// revision whose lineage names the prior revision, and every prior revision
// remains retrievable. Writes are idempotent on (identity, revision): re-recording
// an identical revision is a no-op success, while a conflicting payload for the
// same (identity, revision) is rejected. A record whose revision does not advance
// the identity's current revision is rejected (no silent overwrite / no rewind).
//
// Deterministic; no clock, no threads, no I/O.
class KnowledgeStore {
public:
    KnowledgeStore() = default;

    // Appends a new revision. Returns true on success or idempotent no-op.
    bool record(KnowledgeObject object) {
        if (!object.valid()) return false;
        auto& chain = chains_[object.knowledge_id];
        if (!chain.empty()) {
            const KnowledgeObject& current = chain.back();
            if (object.revision == current.revision) {
                return same_revision_content(current, object);
            }
            if (object.revision < current.revision) return false;  // no rewind
        } else if (object.revision != 0) {
            // First recorded revision of a new identity must be revision 0.
            return false;
        }
        chain.push_back(std::move(object));
        return true;
    }

    bool contains(const foundation::EntityId& id) const {
        return chains_.find(id) != chains_.end();
    }

    // Most recent revision for an identity, or nullptr.
    const KnowledgeObject* latest(const foundation::EntityId& id) const {
        const auto it = chains_.find(id);
        if (it == chains_.end() || it->second.empty()) return nullptr;
        return &it->second.back();
    }

    // Every revision for an identity, oldest first (append order). Empty if absent.
    std::vector<KnowledgeObject> revisions(const foundation::EntityId& id) const {
        std::vector<KnowledgeObject> out;
        const auto it = chains_.find(id);
        if (it == chains_.end()) return out;
        out.assign(it->second.begin(), it->second.end());
        return out;
    }

    // Distinct knowledge identities in deterministic (sorted) order.
    std::vector<foundation::EntityId> identities() const {
        std::vector<foundation::EntityId> out;
        out.reserve(chains_.size());
        for (const auto& entry : chains_) out.push_back(entry.first);
        return out;
    }

    std::size_t identity_count() const noexcept { return chains_.size(); }

    std::size_t revision_count() const noexcept {
        std::size_t n = 0;
        for (const auto& entry : chains_) n += entry.second.size();
        return n;
    }

    // All knowledge objects, oldest revision first per identity, identities sorted.
    // Deterministic; used for audit/export and tests.
    std::vector<KnowledgeObject> all_ordered() const {
        std::vector<KnowledgeObject> out;
        out.reserve(revision_count());
        for (const foundation::EntityId& id : identities()) {
            const auto& chain = chains_.at(id);
            out.insert(out.end(), chain.begin(), chain.end());
        }
        return out;
    }

private:
    static bool same_revision_content(const KnowledgeObject& a, const KnowledgeObject& b) {
        return a.knowledge_id == b.knowledge_id && a.revision == b.revision &&
               a.observation == b.observation && a.context == b.context &&
               a.conclusion == b.conclusion && a.validity_scope == b.validity_scope &&
               a.status == b.status && a.confidence == b.confidence &&
               a.evidence_refs == b.evidence_refs && a.lineage_refs == b.lineage_refs &&
               a.contradictory_refs == b.contradictory_refs;
    }

    std::map<foundation::EntityId, std::vector<KnowledgeObject>> chains_{};
};

}  // namespace learning
}  // namespace aura

#endif  // AURA_LEARNING_KNOWLEDGESTORE_H
