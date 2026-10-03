#ifndef AURA_DESKTOP_RENDERERPOLICY_H
#define AURA_DESKTOP_RENDERERPOLICY_H

// Pure, deterministic renderer-capability policy for the desktop control center.
//
// The control center prefers the modern OpenGL 3.3 core-profile renderer and
// falls back to a legacy OpenGL 2.1 compatibility renderer when the modern
// context cannot be created (e.g. legacy Intel HD Graphics 3000 on Windows,
// whose driver exposes neither GL 3.3 nor WGL_ARB_create_context_profile).
//
// This header contains ONLY the decision logic. It performs no windowing, no
// GL calls, no I/O and no clock reads, so it is unit-testable on any machine
// without a physical GPU and without a windowing system. The caller owns the
// GLFW/GL calls; it records the observed outcome here and asks for the next
// attempt. Nothing about the actual hardware is fabricated: a profile is only
// ever reported as proven when a context was actually created.

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace aura {
namespace desktop {

enum class RendererProfile {
    MODERN_GL33,  // OpenGL 3.3 core profile (preferred)
    LEGACY_GL21,  // OpenGL 2.1 compatibility profile (fallback)
    NONE,         // no renderer could be initialized
};

inline const char* to_string(RendererProfile p) {
    switch (p) {
        case RendererProfile::MODERN_GL33:
            return "MODERN_GL33";
        case RendererProfile::LEGACY_GL21:
            return "LEGACY_GL21";
        case RendererProfile::NONE:
            return "NONE";
    }
    return "NONE";
}

// GLFW window hints for one profile attempt. The legacy profile deliberately
// does NOT request a core profile, because legacy drivers (Intel HD 3000 /
// WGL_ARB_create_context_profile unavailable) reject that request outright.
struct RendererHints {
    int context_version_major{2};
    int context_version_minor{1};
    bool request_core_profile{false};
    bool forward_compatible{false};
    // ImGui backend GLSL version string for this context ("#version 330" for the
    // modern backend, "#version 120" for the legacy OpenGL2 backend).
    const char* glsl_version{"#version 120"};
};

inline RendererHints hints_for(RendererProfile p) {
    RendererHints h;
    if (p == RendererProfile::MODERN_GL33) {
        h.context_version_major = 3;
        h.context_version_minor = 3;
        h.request_core_profile = true;
        h.forward_compatible = false;
        h.glsl_version = "#version 330";
    } else {
        // LEGACY_GL21 (and, defensively, NONE) use a plain compatibility context.
        h.context_version_major = 2;
        h.context_version_minor = 1;
        h.request_core_profile = false;
        h.forward_compatible = false;
        h.glsl_version = "#version 120";
    }
    return h;
}

// Observed result of one context-creation attempt, recorded by the caller.
struct RendererAttempt {
    RendererProfile profile{RendererProfile::NONE};
    bool created{false};
    int glfw_error{0};
    std::string diagnostic;
};

// Human-readable diagnostic shown in the startup log / GUI. It always names the
// profile that is ACTUALLY in use and never claims a profile that failed.
// `modern_error` is the GLFW error from the failed OpenGL 3.3 attempt (0 when the
// modern path succeeded or was not attempted).
inline std::string renderer_diagnostic(RendererProfile active, int modern_error,
                                       const std::string& reason) {
    std::string out;
    out += "aura-gui: renderer=";
    out += to_string(active);
    if (active == RendererProfile::MODERN_GL33) {
        out += " (OpenGL 3.3 core, ImGui OpenGL3 backend)";
    } else if (active == RendererProfile::LEGACY_GL21) {
        out += " (OpenGL 2.1 compatibility, ImGui OpenGL2 backend)";
        out += "; OpenGL 3.3 unavailable";
        if (modern_error != 0) {
            out += " (GLFW error ";
            out += std::to_string(modern_error);
            out += ")";
        }
    } else {
        out += " (no renderer)";
    }
    if (!reason.empty()) {
        out += "; ";
        out += reason;
    }
    return out;
}

// Final, actionable error when no renderer could be initialized. It lists only
// the profiles actually attempted (so a pinned --renderer modern run does not
// claim it tried legacy). There is no pretend-success path: this is what the
// operator sees instead of a silent exit.
inline std::string no_renderer_error(const std::vector<RendererAttempt>& attempts) {
    std::string out = "aura-gui: FATAL: could not create any OpenGL context. Tried ";
    if (attempts.empty()) {
        out += "(nothing)";
    }
    for (std::size_t i = 0; i < attempts.size(); ++i) {
        if (i != 0) out += " and ";
        out += to_string(attempts[i].profile);
        out += " (GLFW error ";
        out += std::to_string(attempts[i].glfw_error);
        out += ")";
    }
    out += ". Actionable: update the graphics driver, or use the console host "
           "`aura.exe` (headless, no OpenGL required).";
    return out;
}

// Operator/CI renderer preference. AUTO is the default and the safe behavior:
// prefer modern, fall back to legacy. MODERN/LEGACY pin a single path for
// diagnostics and smoke tests (e.g. proving the modern renderer still works, or
// exercising the legacy renderer on software GL without a legacy GPU).
enum class RendererChoice {
    AUTO,
    MODERN,
    LEGACY,
};

inline RendererChoice renderer_choice_from_string(const std::string& s) {
    if (s == "modern" || s == "gl33" || s == "gl3") return RendererChoice::MODERN;
    if (s == "legacy" || s == "gl21" || s == "gl2") return RendererChoice::LEGACY;
    return RendererChoice::AUTO;
}

// Deterministic renderer-selection state machine. The caller drives it: it asks
// for the first candidate, records the observed attempt, and gets the next
// candidate (or a terminal error) until a context exists.
class RendererSelector {
  public:
    // The default selector: modern first, then legacy, then terminal failure.
    RendererSelector() = default;

    // A selector pinned to a single profile (diagnostics / smoke tests). It
    // attempts that profile once and is terminal afterwards.
    static RendererSelector only(RendererProfile p) {
        RendererSelector s;
        s.single_ = true;
        s.first_ = p;
        s.current_ = p;
        return s;
    }

    RendererProfile current() const { return current_; }

    // The profile to try now. In AUTO mode: MODERN_GL33, then LEGACY_GL21, then
    // NONE. In single-profile mode: the pinned profile, then NONE.
    RendererProfile candidate() const {
        if (done_) return RendererProfile::NONE;
        if (single_) return index_ == 0 ? first_ : RendererProfile::NONE;
        if (index_ == 0) return RendererProfile::MODERN_GL33;
        if (index_ == 1) return RendererProfile::LEGACY_GL21;
        return RendererProfile::NONE;
    }

    bool done() const { return done_; }
    bool has_renderer() const { return has_renderer_; }
    RendererProfile active() const { return active_; }

    // Record the outcome of trying candidate(). On success the selector becomes
    // terminal with the active profile; on failure it advances. Returns true if
    // a renderer is now active.
    bool record(const RendererAttempt& attempt) {
        attempts_.push_back(attempt);
        if (attempt.created) {
            done_ = true;
            has_renderer_ = true;
            active_ = attempt.profile;
            current_ = attempt.profile;
            return true;
        }
        last_error_ = attempt.glfw_error;
        const RendererProfile next = single_ ? RendererProfile::NONE
                                             : (index_ == 0 ? RendererProfile::LEGACY_GL21
                                                            : RendererProfile::NONE);
        if (next != RendererProfile::NONE) {
            ++index_;
            return false;
        }
        done_ = true;
        has_renderer_ = false;
        active_ = RendererProfile::NONE;
        current_ = RendererProfile::NONE;
        return false;
    }

    // The GLFW error code recorded for the most recent failed attempt.
    int last_error() const { return last_error_; }

    // Error code recorded for a specific profile attempt (0 if not attempted).
    int error_for(RendererProfile p) const {
        for (const auto& a : attempts_) {
            if (a.profile == p) return a.glfw_error;
        }
        return 0;
    }

    // Number of attempts recorded so far (for diagnostics / tests).
    std::size_t attempt_count() const { return attempts_.size(); }

    // The attempts recorded so far, for an accurate, non-fabricating error.
    const std::vector<RendererAttempt>& attempts() const { return attempts_; }

  private:
    std::size_t index_{0};
    bool single_{false};
    RendererProfile first_{RendererProfile::MODERN_GL33};
    bool done_{false};
    bool has_renderer_{false};
    RendererProfile active_{RendererProfile::NONE};
    RendererProfile current_{RendererProfile::MODERN_GL33};
    int last_error_{0};
    std::vector<RendererAttempt> attempts_;
};

}  // namespace desktop
}  // namespace aura

#endif  // AURA_DESKTOP_RENDERERPOLICY_H
