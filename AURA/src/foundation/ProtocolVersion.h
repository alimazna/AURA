#ifndef AURA_FOUNDATION_PROTOCOLVERSION_H
#define AURA_FOUNDATION_PROTOCOLVERSION_H

#include "foundation/Version.h"

#include <string>
#include <string_view>

namespace aura {
namespace foundation {

// Immutable protocol version.
//
// FND-0008 / Phase 0 immutable foundation. The Master V3 requires a protocol
// version in message metadata and requires receivers to reject or quarantine
// unknown protocols (V3-23). ProtocolVersion is a thin, distinct wrapper over
// Version so a protocol version can never be confused with a schema or
// application version. It carries no negotiation behaviour.
class ProtocolVersion {
public:
    ProtocolVersion() = default;
    explicit ProtocolVersion(Version version) : version_(version) {}

    static ProtocolVersion from_string(std::string_view text) {
        return ProtocolVersion(Version::from_string(text));
    }

    const Version& version() const noexcept { return version_; }

    std::string to_string() const { return version_.to_string(); }

    friend bool operator==(const ProtocolVersion& a, const ProtocolVersion& b) noexcept {
        return a.version_ == b.version_;
    }
    friend bool operator!=(const ProtocolVersion& a, const ProtocolVersion& b) noexcept {
        return !(a == b);
    }
    friend bool operator<(const ProtocolVersion& a, const ProtocolVersion& b) noexcept {
        return a.version_ < b.version_;
    }

    std::size_t hash() const noexcept { return version_.hash(); }

private:
    Version version_{};
};

}  // namespace foundation
}  // namespace aura

#endif  // AURA_FOUNDATION_PROTOCOLVERSION_H
