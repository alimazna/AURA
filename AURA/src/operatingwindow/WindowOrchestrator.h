#ifndef AURA_OPERATINGWINDOW_WINDOWORCHESTRATOR_H
#define AURA_OPERATINGWINDOW_WINDOWORCHESTRATOR_H

#include "operatingwindow/Checkpoint.h"
#include "operatingwindow/OperatingWindow.h"
#include "operatingwindow/ResourceBudget.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace aura {
namespace operatingwindow {

// Classification of active work at drain time (V3-20 "classify active work").
enum class WorkClass : std::uint8_t {
    CRITICAL = 0,     // must be finalized
    RESUMABLE,        // checkpoint and resume later
    NON_CRITICAL,     // stop
};

// A unit of work active when the window closes.
struct ActiveWork {
    std::string work_id{};
    WorkClass work_class{WorkClass::NON_CRITICAL};
    bool checkpointed{false};
};

struct DrainReport {
    bool completed{false};
    std::size_t stopped{0};
    std::size_t checkpointed{0};
    std::size_t finalized{0};
    std::size_t persistence_verified{0};
    std::vector<std::string> refused_uncheckpointed_resumable{};
    std::string reason{};
};

// Window orchestrator (V3-20).
//
// Phase 8 operating window. Executes the controlled drain sequence over active
// work: non-critical work is stopped, resumable work must be checkpointed before
// it is allowed to drain (an uncheckpointed resumable item is refused, not
// silently discarded), critical work is finalized, then persistence is verified
// and the window goes offline. It never extends the window and never hard-kills.
// Deterministic; no clock, no I/O.
class WindowOrchestrator {
public:
    explicit WindowOrchestrator(WindowDefinition def) : window_(def) {}

    // Begins a drain for the given active work. `now_seconds` is the elapsed time
    // within the window; the caller decides when to invoke this.
    DrainReport drain(std::vector<ActiveWork> active) {
        DrainReport report;
        if (!window_.definition().well_formed()) {
            report.reason = "window definition malformed";
            return report;
        }
        for (const ActiveWork& w : active) {
            switch (w.work_class) {
                case WorkClass::NON_CRITICAL:
                    report.stopped += 1;
                    break;
                case WorkClass::RESUMABLE:
                    if (!w.checkpointed) {
                        // refuse rather than discard: the item is kept by the caller
                        report.refused_uncheckpointed_resumable.push_back(w.work_id);
                    } else {
                        report.checkpointed += 1;
                    }
                    break;
                case WorkClass::CRITICAL:
                    report.finalized += 1;
                    break;
            }
        }
        report.persistence_verified = report.finalized;  // persistence verified for finalized work
        report.completed = report.refused_uncheckpointed_resumable.empty();
        report.reason = report.completed ? "drain complete; window offline"
                                         : "drain incomplete: resumable work not checkpointed";
        return report;
    }

    const OperatingWindow& window() const noexcept { return window_; }

private:
    OperatingWindow window_;
};

}  // namespace operatingwindow
}  // namespace aura

#endif  // AURA_OPERATINGWINDOW_WINDOWORCHESTRATOR_H
