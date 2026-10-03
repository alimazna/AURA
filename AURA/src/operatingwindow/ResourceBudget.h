#ifndef AURA_OPERATINGWINDOW_RESOURCEBUDGET_H
#define AURA_OPERATINGWINDOW_RESOURCEBUDGET_H

#include <cstdint>
#include <map>
#include <string_view>

namespace aura {
namespace operatingwindow {

// Resource categories (V3-20/V3-45 resource budgets).
enum class ResourceKind : std::uint8_t {
    CPU_SECONDS = 0,
    MEMORY_MB,
    DISK_MB,
    NETWORK_CALLS,
    API_CALLS,
};

constexpr std::string_view to_string(ResourceKind k) noexcept {
    switch (k) {
        case ResourceKind::CPU_SECONDS:  return "CPU_SECONDS";
        case ResourceKind::MEMORY_MB:    return "MEMORY_MB";
        case ResourceKind::DISK_MB:      return "DISK_MB";
        case ResourceKind::NETWORK_CALLS: return "NETWORK_CALLS";
        case ResourceKind::API_CALLS:    return "API_CALLS";
    }
    return "CPU_SECONDS";
}

enum class ResourceState : std::uint8_t {
    OK = 0,
    WARNING,
    EXHAUSTED,
};

constexpr std::string_view to_string(ResourceState s) noexcept {
    switch (s) {
        case ResourceState::OK:        return "OK";
        case ResourceState::WARNING:   return "WARNING";
        case ResourceState::EXHAUSTED: return "EXHAUSTED";
    }
    return "OK";
}

// Resource budget ledger (Phase 8).
//
// Tracks consumption against declared limits. Refuses consumption beyond a limit
// (no partial spend) and refuses negative consumption. A resource with no declared
// limit is unbounded and reported OK. Deterministic; no threads, no I/O.
class ResourceBudget {
public:
    ResourceBudget() = default;

    void set_limit(ResourceKind kind, std::int64_t limit) { limits_[kind] = limit; }

    std::int64_t consumed(ResourceKind kind) const {
        const auto it = consumed_.find(kind);
        return it == consumed_.end() ? 0 : it->second;
    }

    bool consume(ResourceKind kind, std::int64_t amount) {
        if (amount < 0) return false;
        const std::int64_t current = consumed(kind);
        const auto it = limits_.find(kind);
        if (it != limits_.end() && current + amount > it->second) return false;
        consumed_[kind] = current + amount;
        return true;
    }

    std::int64_t remaining(ResourceKind kind) const {
        const auto it = limits_.find(kind);
        if (it == limits_.end()) return -1;
        return it->second - consumed(kind);
    }

    // OK < 80% used, WARNING 80..99%, EXHAUSTED at/over limit (or limit <= 0).
    ResourceState state(ResourceKind kind) const {
        const auto it = limits_.find(kind);
        if (it == limits_.end()) return ResourceState::OK;
        if (it->second <= 0) return ResourceState::EXHAUSTED;
        const std::int64_t used = consumed(kind);
        if (used >= it->second) return ResourceState::EXHAUSTED;
        if (used * 10 >= it->second * 8) return ResourceState::WARNING;
        return ResourceState::OK;
    }

private:
    std::map<ResourceKind, std::int64_t> limits_{};
    std::map<ResourceKind, std::int64_t> consumed_{};
};

}  // namespace operatingwindow
}  // namespace aura

#endif  // AURA_OPERATINGWINDOW_RESOURCEBUDGET_H
