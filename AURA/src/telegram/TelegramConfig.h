#ifndef AURA_TELEGRAM_TELEGRAMCONFIG_H
#define AURA_TELEGRAM_TELEGRAMCONFIG_H

#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace telegram {

// Telegram configuration (V3-38).
//
// Phase 10 auxiliary Telegram. The bot token is supplied by the human operator at
// configuration time and is never embedded in source or committed. This type holds
// no default secret: `bot_token` is empty until a human sets it. When the token is
// absent the channel is "not configured" and the gateway stays disabled; the core
// is unaffected because Telegram is auxiliary (V3-38).
struct TelegramConfig {
    bool enabled{false};
    std::string bot_token{};       // secret; empty by default; supplied out-of-band
    std::string chat_id{};         // destination chat
    std::string bot_role{};        // "operations" | "research_governance"
    std::vector<std::string> allowed_user_ids{};  // authorized humans only

    // Configured only when enabled, a token is present, a destination exists, and
    // at least one authorized user is declared. No default token is ever invented.
    bool configured() const noexcept {
        return enabled && !bot_token.empty() && !chat_id.empty() && !allowed_user_ids.empty();
    }

    // Whether a user may send authenticated requests. Unknown users are never
    // authorized by implication.
    bool is_user_authorized(const std::string& user_id) const {
        for (const auto& u : allowed_user_ids) {
            if (u == user_id) return true;
        }
        return false;
    }
};

}  // namespace telegram
}  // namespace aura

#endif  // AURA_TELEGRAM_TELEGRAMCONFIG_H
