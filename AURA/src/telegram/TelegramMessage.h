#ifndef AURA_TELEGRAM_TELEGRAMMESSAGE_H
#define AURA_TELEGRAM_TELEGRAMMESSAGE_H

#include "foundation/Timestamp.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace aura {
namespace telegram {

// Message class (V3-38 / section 29 assistant responsibilities).
enum class MessageClass : std::uint8_t {
    STATUS = 0,
    MARKET_SNAPSHOT,
    PREDICTION,
    RESULT,
    STATISTIC,
    ERROR,
    RESEARCH_ALERT,
    REPORT,
    NEW_FINDING_ALERT,
    CHANGE_PROPOSAL_SUMMARY,
    CANDIDATE_STATUS,
    VALIDATION_SUMMARY,
    APPROVAL_NEEDED,
    ROLLBACK_NOTIFICATION,
    INCIDENT,
};

constexpr std::string_view to_string(MessageClass c) noexcept {
    switch (c) {
        case MessageClass::STATUS:                  return "STATUS";
        case MessageClass::MARKET_SNAPSHOT:         return "MARKET_SNAPSHOT";
        case MessageClass::PREDICTION:              return "PREDICTION";
        case MessageClass::RESULT:                  return "RESULT";
        case MessageClass::STATISTIC:               return "STATISTIC";
        case MessageClass::ERROR:                   return "ERROR";
        case MessageClass::RESEARCH_ALERT:          return "RESEARCH_ALERT";
        case MessageClass::REPORT:                  return "REPORT";
        case MessageClass::NEW_FINDING_ALERT:       return "NEW_FINDING_ALERT";
        case MessageClass::CHANGE_PROPOSAL_SUMMARY: return "CHANGE_PROPOSAL_SUMMARY";
        case MessageClass::CANDIDATE_STATUS:        return "CANDIDATE_STATUS";
        case MessageClass::VALIDATION_SUMMARY:      return "VALIDATION_SUMMARY";
        case MessageClass::APPROVAL_NEEDED:         return "APPROVAL_NEEDED";
        case MessageClass::ROLLBACK_NOTIFICATION:   return "ROLLBACK_NOTIFICATION";
        case MessageClass::INCIDENT:                return "INCIDENT";
    }
    return "STATUS";
}

// Telegram is a notification channel. It may report and it MAY carry a bounded
// authenticated request, but it is never a command channel and never source of
// truth (V3-38). These classes are informational only.
constexpr bool is_informational(MessageClass c) noexcept {
    (void)c;
    return true;  // all current classes are informational notifications
}

struct TelegramMessage {
    std::string message_id{};
    MessageClass message_class{MessageClass::STATUS};
    std::string subject_ref{};   // e.g. candidate/decision id, for cross-reference
    std::string body{};
    foundation::Timestamp created_at{};

    bool valid() const noexcept { return !message_id.empty() && !body.empty(); }
};

}  // namespace telegram
}  // namespace aura

#endif  // AURA_TELEGRAM_TELEGRAMMESSAGE_H
