#ifndef AURA_OPERATINGWINDOW_OPERATINGWINDOW_H
#define AURA_OPERATINGWINDOW_OPERATINGWINDOW_H

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace operatingwindow {

// Window phase (V3-20). The system may not extend its own window.
enum class WindowPhase : std::uint8_t {
    ACTIVE = 0,
    DRAINING,
    OFFLINE,
};

constexpr std::string_view to_string(WindowPhase p) noexcept {
    switch (p) {
        case WindowPhase::ACTIVE:   return "ACTIVE";
        case WindowPhase::DRAINING: return "DRAINING";
        case WindowPhase::OFFLINE:  return "OFFLINE";
    }
    return "OFFLINE";
}

// Controlled drain steps (V3-20). A controlled draining event, not a hard kill.
enum class DrainStep : std::uint8_t {
    STOP_NEW_NON_CRITICAL = 0,
    CLASSIFY_ACTIVE_WORK,
    CHECKPOINT_RESUMABLE_WORK,
    FINALIZE_CRITICAL_PERSISTENCE,
    VERIFY_PERSISTENCE,
    DRAIN,
    OFFLINE,
};

constexpr std::string_view to_string(DrainStep s) noexcept {
    switch (s) {
        case DrainStep::STOP_NEW_NON_CRITICAL:          return "STOP_NEW_NON_CRITICAL";
        case DrainStep::CLASSIFY_ACTIVE_WORK:           return "CLASSIFY_ACTIVE_WORK";
        case DrainStep::CHECKPOINT_RESUMABLE_WORK:      return "CHECKPOINT_RESUMABLE_WORK";
        case DrainStep::FINALIZE_CRITICAL_PERSISTENCE:  return "FINALIZE_CRITICAL_PERSISTENCE";
        case DrainStep::VERIFY_PERSISTENCE:             return "VERIFY_PERSISTENCE";
        case DrainStep::DRAIN:                          return "DRAIN";
        case DrainStep::OFFLINE:                        return "OFFLINE";
    }
    return "OFFLINE";
}

// Human-defined bounded window (V3-20). Defaults are the documented 3-8h/day.
struct WindowDefinition {
    std::int64_t min_seconds{3 * 3600};
    std::int64_t max_seconds{8 * 3600};
    std::int64_t drain_seconds{300};  // portion of the window reserved for draining

    bool well_formed() const noexcept {
        return min_seconds > 0 && max_seconds >= min_seconds && drain_seconds >= 0 &&
               drain_seconds <= min_seconds;
    }
};

// Human-defined daily schedule entry. Runtime and research schedules are separate
// but each must remain bounded by authority (V3-20).
struct ScheduleRequest {
    std::string actor{};        // "human" | "runtime" | "research"
    std::int64_t duration_seconds{0};
};

struct ScheduleDecision {
    bool accepted{false};
    std::int64_t effective_seconds{0};
    std::string reason{};
};

// Operating window / schedule manager (V3-20).
//
// Phase 8 operating window. Enforces the human-defined bounded window and refuses
// self-extension: only a human actor may set a window up to max_seconds; runtime
// and research actors may not extend it. Draining is a controlled sequence, not a
// hard kill. Deterministic: the caller supplies elapsed time; no wall clock.
class OperatingWindow {
public:
    explicit OperatingWindow(WindowDefinition def) : def_(def) {}

    const WindowDefinition& definition() const noexcept { return def_; }

    // Validates a schedule request. Requests beyond max are refused (never
    // silently capped for automated actors) and shorter-than-min automated
    // requests are refused; a human may choose any duration in [min, max].
    ScheduleDecision schedule(const ScheduleRequest& request) const {
        ScheduleDecision d;
        if (!def_.well_formed()) {
            d.reason = "window definition malformed";
            return d;
        }
        if (request.duration_seconds <= 0) {
            d.reason = "duration must be positive";
            return d;
        }
        const bool human = (request.actor == "human");
        if (request.duration_seconds > def_.max_seconds) {
            d.reason = "window cannot be extended beyond max (self-extension refused)";
            d.effective_seconds = human ? def_.max_seconds : 0;
            d.accepted = false;
            return d;
        }
        if (!human && request.duration_seconds < def_.min_seconds) {
            d.reason = "automated schedule below minimum window refused";
            return d;
        }
        d.accepted = true;
        d.effective_seconds = request.duration_seconds;
        d.reason = human ? "human schedule accepted" : "bounded automated schedule accepted";
        return d;
    }

    // Phase given elapsed seconds within the accepted window. Draining begins at
    // (duration - drain_seconds); the window is OFFLINE at or after duration.
    WindowPhase phase_at(std::int64_t elapsed_seconds, std::int64_t window_seconds) const noexcept {
        if (window_seconds <= 0) return WindowPhase::OFFLINE;
        if (elapsed_seconds >= window_seconds) return WindowPhase::OFFLINE;
        if (elapsed_seconds >= window_seconds - def_.drain_seconds) return WindowPhase::DRAINING;
        return WindowPhase::ACTIVE;
    }

    // The canonical drain sequence (V3-20).
    static std::vector<DrainStep> drain_sequence() {
        return {DrainStep::STOP_NEW_NON_CRITICAL, DrainStep::CLASSIFY_ACTIVE_WORK,
                DrainStep::CHECKPOINT_RESUMABLE_WORK, DrainStep::FINALIZE_CRITICAL_PERSISTENCE,
                DrainStep::VERIFY_PERSISTENCE, DrainStep::DRAIN, DrainStep::OFFLINE};
    }

    // The system may not extend its own window/authority (V3-20/CLAUDE guardrail).
    static bool may_self_extend() noexcept { return false; }

private:
    WindowDefinition def_{};
};

}  // namespace operatingwindow
}  // namespace aura

#endif  // AURA_OPERATINGWINDOW_OPERATINGWINDOW_H
