#ifndef AURA_RUNTIME_APPLICATIONRECOVERY_H
#define AURA_RUNTIME_APPLICATIONRECOVERY_H

#include "foundation/EntityId.h"
#include "foundation/FilePersistenceStore.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "mt5/Mt5StreamManager.h"
#include "runtime/ShadowLedger.h"
#include "runtime/TimeframeStateStore.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace aura {
namespace runtime {

// Crash-recovery lifecycle classification (V2-36 / V3-21).
//
// These are the exact five states the Master requires the system to distinguish.
// They must not be collapsed: a CORRUPTED_STATE may never be treated as a
// CLEAN_SHUTDOWN, and an unknown state must not silently continue with
// production-impacting behaviour.
enum class LifecycleState : std::uint8_t {
    UNKNOWN_STATE = 0,
    CLEAN_SHUTDOWN,
    EXPECTED_PAUSE,
    INTERRUPTED_WORK,
    CORRUPTED_STATE,
};

constexpr std::string_view to_string(LifecycleState state) noexcept {
    switch (state) {
        case LifecycleState::UNKNOWN_STATE:   return "UNKNOWN_STATE";
        case LifecycleState::CLEAN_SHUTDOWN:  return "CLEAN_SHUTDOWN";
        case LifecycleState::EXPECTED_PAUSE:  return "EXPECTED_PAUSE";
        case LifecycleState::INTERRUPTED_WORK:return "INTERRUPTED_WORK";
        case LifecycleState::CORRUPTED_STATE: return "CORRUPTED_STATE";
    }
    return "UNKNOWN_STATE";
}

// Why the previous process stopped, as recorded at shutdown time. CRASH/UNKNOWN
// means no clean intent was ever written.
enum class ShutdownIntent : std::uint8_t {
    UNKNOWN = 0,
    CLEAN,
    PAUSE,
    CRASH,
};

constexpr std::string_view to_string(ShutdownIntent intent) noexcept {
    switch (intent) {
        case ShutdownIntent::UNKNOWN: return "UNKNOWN";
        case ShutdownIntent::CLEAN:   return "CLEAN";
        case ShutdownIntent::PAUSE:   return "PAUSE";
        case ShutdownIntent::CRASH:   return "CRASH";
    }
    return "UNKNOWN";
}

// The persisted manifest header. It carries the identity/version information
// required to verify compatibility before any resume, and an explicit
// completeness flag so partial results are never mistaken for complete ones.
struct PersistedManifest {
    std::string schema_version{};
    std::string strategy_version{};
    std::string configuration_version{};
    ShutdownIntent intent{ShutdownIntent::UNKNOWN};
    bool complete{false};
    std::string checkpoint_id{};
    foundation::Timestamp checkpoint_at{};
    std::string artifact_digest{};

    bool valid() const noexcept { return !schema_version.empty(); }
};

// The recovery decision. `resumable` is true only when it is safe to continue the
// exact prior work (state present, uncorrupted, version-compatible and complete,
// or explicitly interrupted but checkpointed). `used_known_good` marks recovery
// to a known-good state instead. A non-resumable decision always carries a reason.
struct RecoveryOutcome {
    LifecycleState detected{LifecycleState::UNKNOWN_STATE};
    bool resumable{false};
    bool used_known_good{false};
    bool fresh_start{false};
    std::string reason{};
    std::string checkpoint_id{};
};

// Application-level persistence + crash recovery coordinator (PERSIST-0001).
//
// Wires the concrete FilePersistenceStore into the running application and
// implements the V2-36 boot decision without inventing a new persistence
// contract:
//
//   BOOT -> load+verify store -> classify prior lifecycle -> verify compatibility
//        -> RESUME | RESTART(known-good) | REFUSE
//
// Invariants: a corrupted store is refused (never silently continued); a state
// whose schema/strategy/configuration identity is incompatible is refused rather
// than mis-resumed; an incomplete manifest is INTERRUPTED_WORK, never
// CLEAN_SHUTDOWN; and the whole component is deterministic and shadow-only (it
// records and reads lifecycle metadata only — no order, no live path).
class ApplicationRecovery {
public:
    static constexpr std::string_view kManifestRecordId = "aura.app.manifest";
    // Field separator for the recovery text codecs. A control character keeps the
    // codecs collision-free with '|' inside ids/payloads and with the store's tabs.
    static constexpr char kFieldSep = '\x1f';

    // Records the shutdown intent and completeness into the store and persists it
    // atomically. Called on a clean shutdown (CLEAN) or a deliberate pause (PAUSE).
    static foundation::PersistenceStatus record_shutdown(
        foundation::FilePersistenceStore& store, const PersistedManifest& manifest) {
        const foundation::PersistenceStatus status = store.append_record(
            foundation::PersistenceRecordMetadata(
                foundation::EntityId(std::string(kManifestRecordId)),
                foundation::SchemaVersion::from_string(manifest.schema_version), manifest.checkpoint_at),
            encode(manifest));
        if (!foundation::is_success(status) && status != foundation::PersistenceStatus::CONFLICT) {
            return status;
        }
        return store.flush();
    }

    // Loads the prior manifest from the store, verifying the store integrity first.
    // On OK the decoded manifest is placed in `out`.
    static foundation::PersistenceStatus load_manifest(foundation::FilePersistenceStore& store,
                                                       PersistedManifest& out) {
        const foundation::PersistenceStatus status = store.load();
        if (status != foundation::PersistenceStatus::OK) return status;
        const std::string payload =
            store.payload_of(foundation::EntityId(std::string(kManifestRecordId)));
        if (payload.empty()) return foundation::PersistenceStatus::NOT_FOUND;
        // The payload digest is verified by the store checksum; decode verifies the
        // field grammar. A present-but-undecodable manifest is reported, not guessed.
        return decode(payload, out) ? foundation::PersistenceStatus::OK
                                    : foundation::PersistenceStatus::CORRUPT;
    }

    // The V2-36 boot decision.
    //
    // `expected_schema`/`expected_strategy`/`expected_configuration` are the
    // identities this build requires. A prior state that does not match is refused
    // (never resumed). When the store is unreadable/not-found, `known_good_available`
    // decides between recovering to a known-good state (RESTART) and refusing.
    static RecoveryOutcome decide(foundation::FilePersistenceStore& store,
                                  const std::string& expected_schema,
                                  const std::string& expected_strategy,
                                  const std::string& expected_configuration,
                                  bool known_good_available) {
        PersistedManifest prior;
        const foundation::PersistenceStatus status = load_manifest(store, prior);

        RecoveryOutcome outcome;
        switch (status) {
            case foundation::PersistenceStatus::NOT_FOUND:
                // No prior state at all: a fresh start is safe and must not be
                // mislabelled as corrupted or interrupted.
                outcome.detected = LifecycleState::UNKNOWN_STATE;
                outcome.fresh_start = true;
                outcome.resumable = false;
                outcome.reason = "no prior persisted state; clean fresh start";
                return outcome;
            case foundation::PersistenceStatus::CORRUPT:
            case foundation::PersistenceStatus::UNAVAILABLE:
            case foundation::PersistenceStatus::FAILED:
                outcome.detected = LifecycleState::CORRUPTED_STATE;
                outcome.resumable = false;
                outcome.used_known_good = known_good_available;
                outcome.reason = std::string("persisted state is ") + std::string(foundation::to_string(status)) +
                                 (known_good_available ? "; recovering to known-good state"
                                                       : "; refusing to continue on unverified state");
                return outcome;
            default:
                break;
        }

        if (!prior.valid()) {
            outcome.detected = LifecycleState::CORRUPTED_STATE;
            outcome.resumable = false;
            outcome.reason = "manifest present but not decodable; refusing";
            return outcome;
        }

        // Classify the prior lifecycle from the recorded intent + completeness.
        outcome.detected = classify(prior);
        outcome.checkpoint_id = prior.checkpoint_id;

        // Verify identity compatibility before any resume (V2-35 89.2).
        if (prior.schema_version != expected_schema ||
            prior.strategy_version != expected_strategy ||
            prior.configuration_version != expected_configuration) {
            outcome.resumable = false;
            outcome.used_known_good = known_good_available;
            outcome.reason = "incompatible state (schema/strategy/configuration mismatch); refusing resume";
            return outcome;
        }

        if (outcome.detected == LifecycleState::CORRUPTED_STATE) {
            outcome.resumable = false;
            outcome.used_known_good = known_good_available;
            outcome.reason = "corrupted state; refusing resume";
            return outcome;
        }

        // CLEAN_SHUTDOWN / EXPECTED_PAUSE / INTERRUPTED_WORK are resumable only
        // when the prior work is not being mistaken for complete: an interrupted
        // manifest without a checkpoint is refused (nothing to safely resume from).
        if (outcome.detected == LifecycleState::INTERRUPTED_WORK && prior.checkpoint_id.empty()) {
            outcome.resumable = false;
            outcome.used_known_good = known_good_available;
            outcome.reason = "interrupted work with no checkpoint; refusing to resume unverified work";
            return outcome;
        }

        outcome.resumable = true;
        outcome.reason = std::string("resuming from ") + std::string(to_string(outcome.detected));
        return outcome;
    }

    // Maps a manifest (already known to be integrity-clean) to a lifecycle state.
    static LifecycleState classify(const PersistedManifest& prior) {
        if (!prior.valid()) return LifecycleState::CORRUPTED_STATE;
        switch (prior.intent) {
            case ShutdownIntent::CLEAN:
                return prior.complete ? LifecycleState::CLEAN_SHUTDOWN
                                      : LifecycleState::INTERRUPTED_WORK;
            case ShutdownIntent::PAUSE:
                return prior.complete ? LifecycleState::EXPECTED_PAUSE
                                      : LifecycleState::INTERRUPTED_WORK;
            case ShutdownIntent::CRASH:
            case ShutdownIntent::UNKNOWN:
            default:
                return LifecycleState::INTERRUPTED_WORK;
        }
    }

    // Deterministic text encoding of a manifest (round-trips exactly). A control
    // character is used as the field separator so it can never collide with the
    // '|' characters inside checkpoint ids or payload text.
    static std::string encode(const PersistedManifest& m) {
        const std::string sep(1, kFieldSep);
        return "manifest" + sep + m.schema_version + sep + m.strategy_version + sep +
               m.configuration_version + sep + std::string(to_string(m.intent)) + sep +
               (m.complete ? "1" : "0") + sep + m.checkpoint_id + sep +
               std::to_string(m.checkpoint_at.nanoseconds()) + sep + m.artifact_digest;
    }

    static bool decode(const std::string& text, PersistedManifest& out) {
        std::vector<std::string> fields;
        split_into(text, fields);
        if (fields.size() != 9 || fields[0] != "manifest") return false;
        PersistedManifest m;
        m.schema_version = fields[1];
        m.strategy_version = fields[2];
        m.configuration_version = fields[3];
        m.intent = intent_from_string(fields[4]);
        m.complete = fields[5] == "1";
        m.checkpoint_id = fields[6];
        std::int64_t ns = 0;
        if (!parse_i64(fields[7], ns)) return false;
        m.checkpoint_at = foundation::Timestamp::from_nanoseconds(ns);
        m.artifact_digest = fields[8];
        if (!m.valid()) return false;
        out = std::move(m);
        return true;
    }

    // Persists the live derived state (per-timeframe progress + ledger) plus the
    // shutdown manifest into the store, then flushes atomically. Returns the worst
    // status observed. Deterministic and idempotent: re-persisting unchanged state
    // appends nothing new.
    static foundation::PersistenceStatus persist_state(
        foundation::FilePersistenceStore& store, const runtime::TimeframeStateStore& state,
        const runtime::ShadowLedger& ledger, const PersistedManifest& manifest) {
        foundation::PersistenceStatus worst = foundation::PersistenceStatus::OK;
        const foundation::SchemaVersion schema = foundation::SchemaVersion::from_string(manifest.schema_version);

        for (const auto& entry : state.progress()) {
            const runtime::TimeframeProgress& progress = entry.second;
            const foundation::PersistenceStatus s = store.append_record(
                foundation::PersistenceRecordMetadata(
                    runtime::TimeframeStateStore::record_id_for(progress), schema, manifest.checkpoint_at),
                runtime::encode_progress(progress));
            worst = worse(worst, s);
        }

        for (const runtime::LedgerEntry& entry : ledger.ordered()) {
            const foundation::PersistenceStatus s = store.append_record(
                foundation::PersistenceRecordMetadata(entry.entry_id, schema, entry.recorded_at),
                encode_ledger_entry(entry));
            worst = worse(worst, s);
        }

        worst = worse(worst, record_shutdown(store, manifest));
        return worst;
    }

    // Restores per-timeframe progress from the store into the receiver, without
    // reprocessing history. Returns OK when the store loaded and every record was
    // decodable (an undecodable record is CORRUPT, never silently skipped).
    static foundation::PersistenceStatus restore_state(foundation::FilePersistenceStore& store,
                                                       mt5::Mt5StreamManager& receiver) {
        const foundation::PersistenceStatus status = store.load();
        if (status != foundation::PersistenceStatus::OK) return status;
        for (const foundation::FilePersistenceStore::View& view : store.ordered()) {
            if (view.record_id.compare(0, 8, "tfstate|") != 0) continue;
            runtime::TimeframeProgress progress;
            if (!runtime::decode_progress(view.payload, progress)) {
                return foundation::PersistenceStatus::CORRUPT;
            }
            receiver.restore_progress(progress);
        }
        return foundation::PersistenceStatus::OK;
    }

    // Restores the append-only ledger (the historical source of truth) from the
    // store. Idempotent: re-appending an existing identity is a no-op. Returns OK
    // when every ledger record decoded; an undecodable record is CORRUPT.
    static foundation::PersistenceStatus restore_ledger(foundation::FilePersistenceStore& store,
                                                        runtime::ShadowLedger& ledger) {
        const foundation::PersistenceStatus status = store.load();
        if (status != foundation::PersistenceStatus::OK) return status;
        for (const foundation::FilePersistenceStore::View& view : store.ordered()) {
            if (view.record_id.compare(0, 6, "ledger") != 0) continue;
            runtime::LedgerEntry entry;
            if (!decode_ledger_entry(view.payload, entry)) {
                return foundation::PersistenceStatus::CORRUPT;
            }
            ledger.append(std::move(entry));
        }
        return foundation::PersistenceStatus::OK;
    }

    // Deterministic text codec for a ledger entry (round-trips exactly).
    static std::string encode_ledger_entry(const runtime::LedgerEntry& entry) {
        const std::string sep(1, kFieldSep);
        return "ledger" + sep + entry.entry_id.value() + sep +
               std::string(runtime::to_string(entry.type)) + sep + entry.decision_id.to_hex() + sep +
               std::to_string(entry.recorded_at.nanoseconds()) + sep + entry.payload;
    }

    static bool decode_ledger_entry(const std::string& text, runtime::LedgerEntry& out) {
        // Five separators precede the payload, which may itself contain any other
        // character (only the control separator is reserved).
        std::vector<std::string> fields;
        split_into(text, fields);
        if (fields.size() < 6 || fields[0] != "ledger") return false;
        runtime::LedgerEntry entry;
        entry.entry_id = foundation::EntityId(fields[1]);
        entry.type = ledger_type_from_string(fields[2]);
        if (entry.type == runtime::LedgerEntryType::UNKNOWN) return false;
        entry.decision_id = foundation::HashDigest::from_hex(fields[3]);
        std::int64_t ns = 0;
        if (!parse_i64(fields[4], ns)) return false;
        entry.recorded_at = foundation::Timestamp::from_nanoseconds(ns);
        // Re-join any remaining fields to preserve payloads containing separators.
        const std::string sep(1, kFieldSep);
        std::string payload = fields[5];
        for (std::size_t i = 6; i < fields.size(); ++i) payload += sep + fields[i];
        entry.payload = std::move(payload);
        if (!entry.entry_id.valid()) return false;
        out = std::move(entry);
        return true;
    }

private:
    // Rank persistence outcomes so the worst observed is reported (never a
    // success over a real failure).
    static foundation::PersistenceStatus worse(foundation::PersistenceStatus a,
                                               foundation::PersistenceStatus b) noexcept {
        return rank(a) >= rank(b) ? a : b;
    }

    static int rank(foundation::PersistenceStatus s) noexcept {
        switch (s) {
            case foundation::PersistenceStatus::OK:          return 0;
            case foundation::PersistenceStatus::NOT_FOUND:   return 1;
            case foundation::PersistenceStatus::CONFLICT:    return 9;  // idempotent, not a failure
            case foundation::PersistenceStatus::UNKNOWN:     return 5;
            case foundation::PersistenceStatus::UNAVAILABLE: return 6;
            case foundation::PersistenceStatus::FAILED:      return 7;
            case foundation::PersistenceStatus::CORRUPT:     return 8;
        }
        return 5;
    }

    static runtime::LedgerEntryType ledger_type_from_string(std::string_view text) {
        using runtime::LedgerEntryType;
        const LedgerEntryType all[] = {
            LedgerEntryType::SIGNAL,     LedgerEntryType::SCORE,      LedgerEntryType::CONFIDENCE,
            LedgerEntryType::RISK_PROPOSAL, LedgerEntryType::SIMULATED_FILL, LedgerEntryType::POSITION,
            LedgerEntryType::RECONCILIATION, LedgerEntryType::OUTCOME};
        for (const LedgerEntryType t : all) {
            if (runtime::to_string(t) == text) return t;
        }
        return LedgerEntryType::UNKNOWN;
    }

    static ShutdownIntent intent_from_string(std::string_view text) {
        if (text == "CLEAN") return ShutdownIntent::CLEAN;
        if (text == "PAUSE") return ShutdownIntent::PAUSE;
        if (text == "CRASH") return ShutdownIntent::CRASH;
        return ShutdownIntent::UNKNOWN;
    }

    static void split_into(const std::string& text, std::vector<std::string>& out) {
        std::size_t start = 0;
        for (;;) {
            const std::size_t bar = text.find(kFieldSep, start);
            if (bar == std::string::npos) {
                out.push_back(text.substr(start));
                return;
            }
            out.push_back(text.substr(start, bar - start));
            start = bar + 1;
        }
    }

    static bool parse_i64(const std::string& text, std::int64_t& out) {
        if (text.empty()) return false;
        bool negative = false;
        std::size_t i = 0;
        if (text[0] == '-') {
            negative = true;
            i = 1;
        }
        std::int64_t value = 0;
        for (; i < text.size(); ++i) {
            if (text[i] < '0' || text[i] > '9') return false;
            value = value * 10 + (text[i] - '0');
        }
        out = negative ? -value : value;
        return true;
    }
};

}  // namespace runtime
}  // namespace aura

#endif  // AURA_RUNTIME_APPLICATIONRECOVERY_H
