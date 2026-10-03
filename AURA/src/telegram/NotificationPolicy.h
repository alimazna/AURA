#ifndef AURA_TELEGRAM_NOTIFICATIONPOLICY_H
#define AURA_TELEGRAM_NOTIFICATIONPOLICY_H

#include "telegram/TelegramConfig.h"
#include "telegram/TelegramMessage.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace aura {
namespace telegram {

// Per-class delivery preference. Telegram is a notification policy, not a decision.
struct NotificationPreference {
    MessageClass message_class{MessageClass::STATUS};
    bool enabled{true};
    std::int64_t min_interval_seconds{0};  // rate-limit; 0 = no limit
};

// Notification policy (section 29 / "notification policy layered on top of the
// canonical decision").
//
// Phase 10 auxiliary Telegram. Decides whether a notification should be emitted,
// independent of the decision it describes. It never suppresses failure/incident
// notifications: ERROR, INCIDENT, ROLLBACK_NOTIFICATION and APPROVAL_NEEDED are
// always eligible (Master 39 #18 "suppression of failure signals" is forbidden).
// Deterministic: the caller supplies the current time; no wall clock.
class NotificationPolicy {
public:
    void set_preference(const NotificationPreference& pref) { prefs_[pref.message_class] = pref; }

    // Safety-relevant classes may never be disabled or rate-limited away.
    static bool always_eligible(MessageClass c) noexcept {
        switch (c) {
            case MessageClass::ERROR:
            case MessageClass::INCIDENT:
            case MessageClass::ROLLBACK_NOTIFICATION:
            case MessageClass::APPROVAL_NEEDED:
                return true;
            default:
                return false;
        }
    }

    // Returns true when a message should be emitted at `now_seconds` given prior
    // emission times recorded via `record_sent`.
    bool should_emit(MessageClass c, std::int64_t now_seconds) const {
        if (always_eligible(c)) return true;
        const auto it = prefs_.find(c);
        if (it == prefs_.end()) return true;  // default: enabled
        if (!it->second.enabled) return false;
        const auto last = last_sent_.find(c);
        if (last != last_sent_.end() && it->second.min_interval_seconds > 0) {
            if (now_seconds - last->second < it->second.min_interval_seconds) return false;
        }
        return true;
    }

    void record_sent(MessageClass c, std::int64_t now_seconds) { last_sent_[c] = now_seconds; }

private:
    std::map<MessageClass, NotificationPreference> prefs_{};
    std::map<MessageClass, std::int64_t> last_sent_{};
};

}  // namespace telegram
}  // namespace aura

#endif  // AURA_TELEGRAM_NOTIFICATIONPOLICY_H
