#ifndef AURA_RUNTIME_REPLAYENGINE_H
#define AURA_RUNTIME_REPLAYENGINE_H

#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/Timestamp.h"
#include "runtime/BarFinalizer.h"
#include "runtime/DataBus.h"
#include "runtime/ShadowLedger.h"
#include "runtime/TimeframeStateStore.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace aura {
namespace runtime {

// Result of a deterministic replay.
//
// RT-0020 / Phase 1 deterministic runtime. `fingerprint` is a deterministic hash
// over the finalized bar identities in replay order; two replays of the same
// input must produce the same fingerprint. `rejected` counts events refused by
// causality/repaint rules.
struct ReplayResult {
    std::size_t processed{0};
    std::size_t finalized{0};
    std::size_t rejected{0};
    foundation::HashDigest fingerprint{};
    bool valid{false};
};

// Deterministic replay engine.
//
// RT-0020 / Phase 1 deterministic runtime. Replays a recorded market-event stream
// through the deterministic data bus (RT-0002), closed-bar finalizer (RT-0004)
// and timeframe state store (RT-0005), and produces a fingerprint that is a pure
// function of the input. Replay enforces the same causality and repaint rules as
// live processing: future-dated events are rejected (no lookahead) and
// non-advancing bars are rejected (no repaint), so replay cannot fabricate a
// better history than was observed. `reproducible()` verifies determinism by
// replaying twice and comparing fingerprints. No clock, no threads, no I/O.
class ReplayEngine {
public:
    ReplayEngine() = default;

    // Replays events in deterministic order. When `ledger` is non-null, finalized
    // bars are appended idempotently. `replay_time` is the record timestamp used
    // for ledger provenance.
    ReplayResult replay(const std::vector<MarketEvent>& events, ShadowLedger* ledger = nullptr,
                        foundation::Timestamp replay_time = foundation::Timestamp{}) const {
        ReplayResult result;

        std::vector<MarketEvent> ordered = events;
        std::stable_sort(ordered.begin(), ordered.end(),
                         [](const MarketEvent& a, const MarketEvent& b) { return a.precedes(b); });

        BarFinalizer finalizer;
        TimeframeStateStore store;
        const std::unique_ptr<foundation::IHasher> hasher =
            foundation::create_hasher(foundation::HashAlgorithm::SHA256);
        if (hasher == nullptr) return result;

        std::string material;
        for (const MarketEvent& event : ordered) {
            ++result.processed;
            if (!event.metadata.valid()) {
                ++result.rejected;
                continue;
            }
            if (event.metadata.event_time() > event.metadata.receive_time()) {
                ++result.rejected;  // no lookahead
                continue;
            }
            if (!event.bar.closed) {
                ++result.rejected;  // no repaint
                continue;
            }

            FinalizedBar finalized;
            if (!finalizer.finalize(event, finalized)) {
                ++result.rejected;  // would repaint or is not closed
                continue;
            }
            store.advance(finalized);
            ++result.finalized;
            material += finalized.identity.canonical_string() + "|" +
                        std::string(foundation::to_string(finalized.quality)) + ";";
            if (ledger != nullptr) {
                const std::string payload = finalized.identity.canonical_string() + "|" +
                                            std::string(foundation::to_string(finalized.quality));
                ledger->record(LedgerEntryType::OUTCOME, foundation::HashDigest{}, replay_time, payload);
            }
        }

        result.fingerprint = hasher->hash(material);
        result.valid = true;
        return result;
    }

    // Replays the event stream twice and reports whether both replays produced
    // identical fingerprints (determinism proof).
    bool reproducible(const std::vector<MarketEvent>& events) const {
        const ReplayResult a = replay(events);
        const ReplayResult b = replay(events);
        return a.valid && b.valid && a.fingerprint == b.fingerprint && a.finalized == b.finalized;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_REPLAYENGINE_H
