#ifndef AURA_FOUNDATION_CONFIGURATIONCHANGE_H
#define AURA_FOUNDATION_CONFIGURATIONCHANGE_H

#include "foundation/ConfigurationKey.h"
#include "foundation/ConfigurationValue.h"
#include "foundation/Timestamp.h"

#include <string>
#include <utility>

namespace aura {
namespace foundation {

// Immutable record of a single configuration change.
//
// CFG-0005 / Phase 0 configuration foundation. Because no silent configuration
// mutation is allowed (V3-26), every change is an explicit, auditable record of
// which key changed, from what value to what value, when, by whom, and why. This
// type records a change; it does not apply it.
class ConfigurationChange {
public:
    ConfigurationChange() = default;

    ConfigurationChange(ConfigurationKey key, ConfigurationValue previous,
                        ConfigurationValue next, Timestamp timestamp, std::string actor,
                        std::string reason)
        : key_(std::move(key)),
          previous_(std::move(previous)),
          next_(std::move(next)),
          timestamp_(timestamp),
          actor_(std::move(actor)),
          reason_(std::move(reason)) {}

    const ConfigurationKey& key() const noexcept { return key_; }
    const ConfigurationValue& previous_value() const noexcept { return previous_; }
    const ConfigurationValue& next_value() const noexcept { return next_; }
    Timestamp timestamp() const noexcept { return timestamp_; }
    const std::string& actor() const noexcept { return actor_; }
    const std::string& reason() const noexcept { return reason_; }

    // A change is meaningful only when it names a key and actually alters the
    // value; a no-op change must not be recorded as a change.
    bool effective() const noexcept { return key_.valid() && !(previous_ == next_); }

private:
    ConfigurationKey key_{};
    ConfigurationValue previous_{};
    ConfigurationValue next_{};
    Timestamp timestamp_{};
    std::string actor_{};
    std::string reason_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_CONFIGURATIONCHANGE_H
