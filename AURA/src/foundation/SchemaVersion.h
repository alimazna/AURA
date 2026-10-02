#ifndef AURA_FOUNDATION_SCHEMAVERSION_H
#define AURA_FOUNDATION_SCHEMAVERSION_H

#include "foundation/Version.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace aura {
namespace foundation {

// Immutable schema version.
//
// FND-0009 / Phase 0 immutable foundation. The Master V3 requires a schema
// version in event and message metadata and requires receivers to reject or
// quarantine records with an invalid schema (V3-23). SchemaVersion is a thin,
// distinct wrapper over Version so a schema version can never be confused with a
// protocol or application version. It carries no migration behaviour.
class SchemaVersion {
public:
    SchemaVersion() = default;
    explicit SchemaVersion(Version version) : version_(version) {}

    static SchemaVersion from_string(std::string_view text) {
        return SchemaVersion(Version::from_string(text));
    }

    const Version& version() const noexcept { return version_; }

    std::string to_string() const { return version_.to_string(); }

    friend bool operator==(const SchemaVersion& a, const SchemaVersion& b) noexcept {
        return a.version_ == b.version_;
    }
    friend bool operator!=(const SchemaVersion& a, const SchemaVersion& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const SchemaVersion& a, const SchemaVersion& b) noexcept {
        return a.version_ < b.version_;
    }

    std::size_t hash() const noexcept { return version_.hash(); }

private:
    Version version_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_SCHEMAVERSION_H
