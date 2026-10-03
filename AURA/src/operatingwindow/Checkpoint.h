#ifndef AURA_OPERATINGWINDOW_CHECKPOINT_H
#define AURA_OPERATINGWINDOW_CHECKPOINT_H

#include "foundation/EntityId.h"
#include "foundation/Timestamp.h"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace operatingwindow {

// Checkpoint validity (V3-21). Invalid checkpoints must never be used to resume.
enum class CheckpointValidity : std::uint8_t {
    UNVERIFIED = 0,
    VALID,
    CORRUPT,
    STALE,
    INCOMPATIBLE_VERSION,
};

constexpr std::string_view to_string(CheckpointValidity v) noexcept {
    switch (v) {
        case CheckpointValidity::UNVERIFIED:            return "UNVERIFIED";
        case CheckpointValidity::VALID:                 return "VALID";
        case CheckpointValidity::CORRUPT:               return "CORRUPT";
        case CheckpointValidity::STALE:                 return "STALE";
        case CheckpointValidity::INCOMPATIBLE_VERSION:  return "INCOMPATIBLE_VERSION";
    }
    return "UNVERIFIED";
}

// A checkpoint (V3-21). Preserves enough state to continue without corrupting
// history or silently replaying invalid work. Provenance/version/digest auditable.
struct Checkpoint {
    foundation::EntityId checkpoint_id{};
    std::string schema_version{};
    std::string artifact_digest{};
    std::string provenance{};
    std::string last_processed_marker{};
    foundation::Timestamp created_at{};
    CheckpointValidity validity{CheckpointValidity::UNVERIFIED};

    bool valid() const noexcept {
        return checkpoint_id.valid() && !schema_version.empty() && !artifact_digest.empty();
    }
};

// Recovery outcome.
struct RecoveryDecision {
    bool resumed{false};
    bool used_known_good{false};
    std::string reason{};
};

// Checkpoint store and crash recovery (V3-21).
//
// Phase 8 operating window. Recovery must prefer a known-valid checkpoint or a
// known-good state rather than guessing. A checkpoint is only usable when its
// validity is VALID and its schema version is compatible; anything else is
// refused and the caller must fall back to a known-good state. The store is
// append-only. Deterministic; no I/O (the caller persists), no wall clock.
class CheckpointStore {
public:
    CheckpointStore() = default;

    bool record(Checkpoint checkpoint) {
        if (!checkpoint.valid()) return false;
        if (by_id_.find(checkpoint.checkpoint_id) != by_id_.end()) return false;
        order_.push_back(checkpoint.checkpoint_id);
        by_id_.emplace(checkpoint.checkpoint_id, std::move(checkpoint));
        return true;
    }

    const Checkpoint* latest() const {
        if (order_.empty()) return nullptr;
        return find_successor(order_.back());
    }

    // Most recent checkpoint that is VALID and compatible with `schema_version`.
    const Checkpoint* latest_usable(const std::string& schema_version) const {
        for (auto it = order_.rbegin(); it != order_.rend(); ++it) {
            const Checkpoint& c = by_id_.at(*it);
            if (c.validity == CheckpointValidity::VALID && c.schema_version == schema_version) {
                return &c;
            }
        }
        return nullptr;
    }

    bool contains(const foundation::EntityId& id) const { return by_id_.find(id) != by_id_.end(); }
    std::size_t size() const noexcept { return by_id_.size(); }

    // Decides how to recover. Prefers a usable checkpoint; otherwise refuses to
    // guess and requires a known-good state. Never silently replays unverified work.
    RecoveryDecision recover(const std::string& schema_version, bool known_good_available) const {
        RecoveryDecision d;
        if (const Checkpoint* c = latest_usable(schema_version)) {
            d.resumed = true;
            d.reason = "resumed from valid checkpoint " + c->checkpoint_id.value();
            return d;
        }
        if (known_good_available) {
            d.resumed = true;
            d.used_known_good = true;
            d.reason = "no usable checkpoint; recovered to known-good state";
            return d;
        }
        d.reason = "no usable checkpoint and no known-good state; refusing to guess";
        return d;
    }

private:
    const Checkpoint* find_successor(const foundation::EntityId& id) const {
        const auto it = by_id_.find(id);
        return it == by_id_.end() ? nullptr : &it->second;
    }

    std::vector<foundation::EntityId> order_{};
    std::map<foundation::EntityId, Checkpoint> by_id_{};
};

}  // namespace operatingwindow
}  // namespace aura

#endif  // AURA_OPERATINGWINDOW_CHECKPOINT_H
