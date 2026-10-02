#ifndef AURA_FOUNDATION_CONFIGURATIONSNAPSHOT_H
#define AURA_FOUNDATION_CONFIGURATIONSNAPSHOT_H

#include "foundation/ConfigurationKey.h"
#include "foundation/ConfigurationScope.h"
#include "foundation/ConfigurationValue.h"
#include "foundation/Version.h"

#include <cstddef>
#include <map>
#include <utility>

namespace aura {
namespace foundation {

// Immutable, versioned configuration snapshot.
//
// CFG-0003 / Phase 0 configuration foundation. The Master V3 requires versioned
// configuration, forbids silent configuration mutation, and requires that
// decisions retain the configuration version they depended on (V3-26). A snapshot
// is therefore a frozen key/value set tagged with a Version; there is no mutator.
// A new configuration is a new snapshot with a new version, never an in-place
// change. Entries are held in an ordered map so iteration is deterministic.
class ConfigurationSnapshot {
public:
    using entry_map = std::map<ConfigurationKey, ConfigurationValue>;

    ConfigurationSnapshot() = default;

    ConfigurationSnapshot(Version version, ConfigurationScope scope, entry_map entries)
        : version_(version), scope_(scope), entries_(std::move(entries)) {}

    const Version& version() const noexcept { return version_; }
    ConfigurationScope scope() const noexcept { return scope_; }
    std::size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }
    const entry_map& entries() const noexcept { return entries_; }

    bool contains(const ConfigurationKey& key) const {
        return entries_.find(key) != entries_.end();
    }

    // Returns the value for `key`, or nullptr when absent. A missing key is not a
    // silent default; callers must decide explicitly.
    const ConfigurationValue* find(const ConfigurationKey& key) const {
        const auto it = entries_.find(key);
        return it == entries_.end() ? nullptr : &it->second;
    }

private:
    Version version_{};
    ConfigurationScope scope_{ConfigurationScope::RUNTIME};
    entry_map entries_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_CONFIGURATIONSNAPSHOT_H
