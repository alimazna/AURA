// Phase 13 — durable persistence + crash-recovery verification artifact.
//
// PERSIST-0001. Proves, over real code paths (no mocks), that derived state
// survives a restart with a file-backed IPersistenceStore:
//   - append idempotency (identical re-append is a no-op OK; conflict is rejected)
//   - atomic flush + reload round-trip (records, digests and payloads preserved)
//   - deterministic artifacts (identical appends -> byte-identical files)
//   - corruption detection (tampered/truncated file -> CORRUPT, never silently OK)
//   - V2-36 lifecycle classification (CLEAN_SHUTDOWN / EXPECTED_PAUSE /
//     INTERRUPTED_WORK / CORRUPTED_STATE / UNKNOWN_STATE) and refusal to resume
//     incompatible or uncheckpointed state
//   - application-level restart: a fresh shell restores nine timeframe states and
//     the ledger, then continues accepting strictly-newer bars
//   - shadow-only safety: no risk proposal is ever an order
// Exits non-zero on any failure.

#include "foundation/FilePersistenceStore.h"
#include "foundation/HashAlgorithm.h"
#include "foundation/HashDigest.h"
#include "foundation/IHasher.h"
#include "foundation/PersistenceRecordMetadata.h"
#include "foundation/PersistenceStatus.h"
#include "foundation/SchemaVersion.h"
#include "foundation/Timestamp.h"
#include "mt5/ProtocolCodec.h"
#include "runtime/ApplicationRecovery.h"
#include "runtime/ApplicationShell.h"

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

using aura::foundation::EntityId;
using aura::foundation::FilePersistenceStore;
using aura::foundation::HashAlgorithm;
using aura::foundation::HashDigest;
using aura::foundation::PersistenceRecordMetadata;
using aura::foundation::PersistenceStatus;
using aura::foundation::SchemaVersion;
using aura::foundation::Timestamp;
using aura::runtime::ApplicationRecovery;
using aura::runtime::ApplicationShell;
using aura::runtime::LifecycleState;
using aura::runtime::PersistedManifest;
using aura::runtime::RecoveryOutcome;
using aura::runtime::ShutdownIntent;

static int g_failures = 0;
static void check(bool condition, const char* what) {
    if (!condition) {
        std::printf("FAIL: %s\n", what);
        ++g_failures;
    }
}

static const char* kTmp = "aura_persist_test.aura";
static const char* kTmpB = "aura_persist_test_b.aura";

static void cleanup() {
    std::remove(kTmp);
    std::remove(kTmpB);
    std::remove((std::string(kTmp) + ".tmp").c_str());
    std::remove((std::string(kTmpB) + ".tmp").c_str());
}

static PersistenceRecordMetadata meta(const std::string& id, const char* schema, std::int64_t ns) {
    return PersistenceRecordMetadata(EntityId(id), SchemaVersion::from_string(schema),
                                     Timestamp::from_nanoseconds(ns));
}

static std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// --- 1. append idempotency, digest/payload retention, reload round-trip -------
static void test_store_round_trip() {
    cleanup();
    {
        FilePersistenceStore store(kTmp);
        check(store.load() == PersistenceStatus::NOT_FOUND, "absent file loads as NOT_FOUND");
        check(store.append_record(meta("r1", "1.0.0", 100), "alpha") == PersistenceStatus::OK,
              "first append OK");
        check(store.append_record(meta("r2", "1.0.0", 200), "beta") == PersistenceStatus::OK,
              "second append OK");
        check(store.size() == 2, "two records");
        check(store.contains(EntityId("r1")) == PersistenceStatus::OK, "r1 present");
        check(store.contains(EntityId("nope")) == PersistenceStatus::NOT_FOUND, "absent id NOT_FOUND");
        // identical re-append is a no-op success
        check(store.append_record(meta("r1", "1.0.0", 100), "alpha") == PersistenceStatus::OK,
              "idempotent re-append OK");
        check(store.size() == 2, "idempotent append does not grow");
        // conflicting payload for the same identity is rejected
        check(store.append_record(meta("r1", "1.0.0", 100), "different") ==
                  PersistenceStatus::CONFLICT,
              "conflicting re-append CONFLICT");
        check(store.flush() == PersistenceStatus::OK, "flush OK");
    }
    {
        FilePersistenceStore reloaded(kTmp);
        check(reloaded.load() == PersistenceStatus::OK, "reload OK");
        check(reloaded.size() == 2, "reload preserves record count");
        check(reloaded.payload_of(EntityId("r2")) == "beta", "payload preserved across reload");
        const std::unique_ptr<aura::foundation::IHasher> hasher =
            aura::foundation::create_hasher(HashAlgorithm::SHA256);
        check(reloaded.digest_of(EntityId("r2")) == hasher->hash("beta").to_hex(),
              "digest preserved across reload");
    }
    cleanup();
}

// --- 2. deterministic artifacts ----------------------------------------------
static void test_determinism() {
    cleanup();
    auto build = [](const char* path) {
        FilePersistenceStore s(path);
        s.append_record(meta("a", "1.0.0", 10), "one");
        s.append_record(meta("b", "1.0.0", 20), "two");
        s.flush();
    };
    build(kTmp);
    build(kTmpB);
    check(read_file(kTmp) == read_file(kTmpB), "identical appends produce byte-identical files");
    check(!read_file(kTmp).empty(), "artifact is non-empty and carries a checksum");
    cleanup();
}

// --- 3. corruption detection --------------------------------------------------
static void test_corruption_detected() {
    cleanup();
    {
        FilePersistenceStore store(kTmp);
        store.append_record(meta("r1", "1.0.0", 100), "alpha");
        store.flush();
    }
    std::string content = read_file(kTmp);
    // Tamper one payload byte after the checksum was computed.
    const std::size_t pos = content.find("alpha");
    check(pos != std::string::npos, "payload present to tamper");
    content[pos] = 'X';
    {
        std::ofstream out(kTmp, std::ios::binary | std::ios::trunc);
        out << content;
    }
    {
        FilePersistenceStore store(kTmp);
        check(store.load() == PersistenceStatus::CORRUPT, "tampered file detected as CORRUPT");
    }
    // Truncate away the checksum line entirely.
    {
        FilePersistenceStore store(kTmp);
        store.append_record(meta("r1", "1.0.0", 100), "alpha");
        store.flush();
    }
    const std::string full = read_file(kTmp);
    const std::size_t marker = full.find("#checksum");
    check(marker != std::string::npos, "checksum marker present");
    {
        std::ofstream out(kTmp, std::ios::binary | std::ios::trunc);
        out << full.substr(0, marker);
    }
    {
        FilePersistenceStore store(kTmp);
        check(store.load() == PersistenceStatus::CORRUPT, "checksum-less file detected as CORRUPT");
    }
    cleanup();
}

// --- 4. lifecycle classification + boot decision -------------------------------
static PersistedManifest manifest(ShutdownIntent intent, bool complete,
                                  const std::string& checkpoint) {
    PersistedManifest m;
    m.schema_version = "1.0.0";
    m.strategy_version = "1.0.0";
    m.configuration_version = "1.0.0";
    m.intent = intent;
    m.complete = complete;
    m.checkpoint_id = checkpoint;
    m.checkpoint_at = Timestamp::from_nanoseconds(500);
    return m;
}

static void test_lifecycle_decisions() {
    cleanup();
    // fresh start
    {
        FilePersistenceStore store(kTmp);
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "1.0.0", "1.0.0", "1.0.0", false);
        check(o.fresh_start && !o.resumable, "no prior state => fresh start");
        check(o.detected == LifecycleState::UNKNOWN_STATE, "fresh start is UNKNOWN_STATE");
    }
    // clean shutdown => resumable
    {
        FilePersistenceStore store(kTmp);
        check(ApplicationRecovery::record_shutdown(store, manifest(ShutdownIntent::CLEAN, true, "cp|500")) ==
                  PersistenceStatus::OK,
              "record clean shutdown");
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "1.0.0", "1.0.0", "1.0.0", false);
        check(o.detected == LifecycleState::CLEAN_SHUTDOWN, "classified CLEAN_SHUTDOWN");
        check(o.resumable, "clean shutdown resumable");
    }
    cleanup();
    // expected pause
    {
        FilePersistenceStore store(kTmp);
        ApplicationRecovery::record_shutdown(store, manifest(ShutdownIntent::PAUSE, true, "cp|500"));
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "1.0.0", "1.0.0", "1.0.0", false);
        check(o.detected == LifecycleState::EXPECTED_PAUSE, "classified EXPECTED_PAUSE");
        check(o.resumable, "expected pause resumable");
    }
    cleanup();
    // interrupted work (crash) with checkpoint => resumable
    {
        FilePersistenceStore store(kTmp);
        ApplicationRecovery::record_shutdown(store, manifest(ShutdownIntent::CRASH, true, "cp|500"));
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "1.0.0", "1.0.0", "1.0.0", false);
        check(o.detected == LifecycleState::INTERRUPTED_WORK, "classified INTERRUPTED_WORK");
        check(o.resumable, "interrupted work with checkpoint resumable");
    }
    cleanup();
    // interrupted work without checkpoint => refused
    {
        FilePersistenceStore store(kTmp);
        ApplicationRecovery::record_shutdown(store, manifest(ShutdownIntent::CRASH, true, ""));
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "1.0.0", "1.0.0", "1.0.0", true);
        check(!o.resumable, "interrupted work without checkpoint refused");
        check(o.used_known_good, "refusal falls back to known-good when available");
    }
    cleanup();
    // incomplete clean manifest => INTERRUPTED_WORK, never CLEAN_SHUTDOWN
    {
        FilePersistenceStore store(kTmp);
        ApplicationRecovery::record_shutdown(store, manifest(ShutdownIntent::CLEAN, false, "cp|500"));
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "1.0.0", "1.0.0", "1.0.0", false);
        check(o.detected == LifecycleState::INTERRUPTED_WORK, "incomplete manifest is INTERRUPTED_WORK");
    }
    cleanup();
    // incompatible identity => refused
    {
        FilePersistenceStore store(kTmp);
        ApplicationRecovery::record_shutdown(store, manifest(ShutdownIntent::CLEAN, true, "cp|500"));
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "2.0.0", "1.0.0", "1.0.0", false);
        check(!o.resumable, "incompatible schema refuses resume");
        check(!o.used_known_good, "no known-good available => no silent fallback");
    }
    cleanup();
    // corrupted store => CORRUPTED_STATE
    {
        FilePersistenceStore store(kTmp);
        ApplicationRecovery::record_shutdown(store, manifest(ShutdownIntent::CLEAN, true, "cp|500"));
        std::string content = read_file(kTmp);
        content[content.find("manifest") + 9] = 'X';
        std::ofstream out(kTmp, std::ios::binary | std::ios::trunc);
        out << content;
        out.close();
        const RecoveryOutcome o = ApplicationRecovery::decide(store, "1.0.0", "1.0.0", "1.0.0", true);
        check(o.detected == LifecycleState::CORRUPTED_STATE, "corrupted store classified CORRUPTED_STATE");
        check(!o.resumable, "corrupted store refuses resume");
    }
    cleanup();
}

// --- 5. application-level restart over real frames ----------------------------
static std::vector<std::string> frames(int steps) {
    std::vector<std::string> out;
    const auto tfs = aura::runtime::all_timeframes();
    for (int i = 0; i < steps; ++i) {
        for (std::size_t k = 0; k < tfs.size(); ++k) {
            const std::int64_t step = 60 * static_cast<std::int64_t>(k + 1);
            const std::int64_t close = 2'000'000 + static_cast<std::int64_t>(i) * step;
            aura::runtime::MarketBar b;
            b.timeframe = tfs[k];
            b.open_time = Timestamp::from_seconds(close - 60);
            b.close_time = Timestamp::from_seconds(close);
            b.open = 200.0 + i + static_cast<double>(k);
            b.high = b.open + 1.0;
            b.low = b.open - 0.5;
            b.close = b.open + 0.8;
            b.volume = 100.0;
            b.closed = true;
            out.push_back(aura::mt5::ProtocolCodec::encode_closed_bar(
                b, "XAUUSD", static_cast<std::uint64_t>(i + 1),
                Timestamp::from_seconds(close + 1)));
        }
    }
    return out;
}

static void test_application_restart() {
    cleanup();
    std::size_t first_ledger = 0;
    std::string first_render;
    {
        ApplicationShell shell(aura::runtime::ApplicationPipeline::Config{}, kTmp);
        for (const std::string& f : frames(30)) shell.feed(f + "\n");
        check(shell.pipeline().state_store().size() == 9, "all nine streams advanced");
        first_ledger = shell.pipeline().ledger().size();
        check(first_ledger > 0, "ledger populated");
        check(shell.persist_state() == PersistenceStatus::OK, "persist_state OK");
        first_render = read_file(kTmp);
    }
    {
        ApplicationShell shell(aura::runtime::ApplicationPipeline::Config{}, kTmp);
        const RecoveryOutcome o = shell.recover(false);
        check(o.detected == LifecycleState::CLEAN_SHUTDOWN, "restart classified CLEAN_SHUTDOWN");
        check(o.resumable, "restart resumable");
        check(shell.apply_resume(), "apply_resume restores state");
        check(shell.pipeline().state_store().size() == 9, "restored nine timeframe states");
        check(shell.pipeline().ledger().size() == first_ledger, "restored ledger size");
        check(!shell.pipeline().last_proposal().is_order, "restored proposal is never an order");

        // Continue with strictly newer bars: must be accepted (resume, not repaint).
        const std::uint64_t accepted_before = shell.pipeline().status().accepted;
        const auto tfs = aura::runtime::all_timeframes();
        for (std::size_t k = 0; k < tfs.size(); ++k) {
            const std::int64_t step = 60 * static_cast<std::int64_t>(k + 1);
            const std::int64_t close = 2'000'000 + 100 * step;  // far ahead of the persisted close
            aura::runtime::MarketBar b;
            b.timeframe = tfs[k];
            b.open_time = Timestamp::from_seconds(close - 60);
            b.close_time = Timestamp::from_seconds(close);
            b.open = 500.0;
            b.high = 501.0;
            b.low = 499.5;
            b.close = 500.8;
            b.volume = 100.0;
            b.closed = true;
            shell.feed(aura::mt5::ProtocolCodec::encode_closed_bar(
                           b, "XAUUSD", 1, Timestamp::from_seconds(close + 1)) + "\n");
        }
        check(shell.pipeline().status().accepted == accepted_before + 9,
              "post-resume strictly-newer bars accepted (no repaint)");
        check(!shell.pipeline().last_proposal().is_order, "no order path after resume");

        // Idempotent re-persist: persisting unchanged state adds no records.
        check(shell.persist_state() == PersistenceStatus::OK, "re-persist OK");
        const std::size_t after_first = shell.store().size();
        check(shell.persist_state() == PersistenceStatus::OK, "second re-persist OK");
        check(shell.store().size() == after_first, "re-persist is idempotent (no new records)");
    }
    check(read_file(kTmp) != first_render, "post-resume state persisted after new work");
    cleanup();
}

int main() {
    test_store_round_trip();
    test_determinism();
    test_corruption_detected();
    test_lifecycle_decisions();
    test_application_restart();

    if (g_failures == 0) {
        std::printf("PersistenceTests: ALL PASS\n");
        return 0;
    }
    std::printf("PersistenceTests: %d FAILURE(S)\n", g_failures);
    return 1;
}
