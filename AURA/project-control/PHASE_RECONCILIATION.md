# AURA — Phase Numbering Reconciliation (Conceptual Phase 3 / 4 / 5)

Status: CANONICAL RECONCILIATION — PROJECT DECISION
Date: 2026-10-02
Authority: `project-control/DECISIONS.md` (2026-10-02, "Phase numbering reconciliation — conceptual
Phase 3/4/5 vs manifest phase order")
Related: Master V3 §V3-40 (authoritative phase order), §V3-29 (timeframe authority),
`TASK_MANIFEST.yaml`, `MT5_ONE_EA_NINE_STREAM_CONTROL.md`.

## 1. The mismatch

The continuation request refers to "PHASE 3 = Timeframe State, PHASE 4 = Feature / Structure /
Regime, PHASE 5 = Signal". The repository's canonical Master V3 authoritative phase order (§V3-40)
is different:

```text
PHASE 0   Immutable Foundations
PHASE 0.5 Resilience & Graceful Degradation
PHASE 1   Deterministic Runtime        (MT5 adapters, data bus, validation, timeframe state,
                                        core runtime, shadow ledger)
PHASE 2   Observation & Outcomes
PHASE 3   Self-Learning
PHASE 4   Research Plane
PHASE 5   Evolution
...
```

So the request's Phase 3/4/5 names are a **conceptual/working decomposition**, not the Master's
phase numbers. Per `DECISIONS.md` source-of-truth order and V3-40, the Master's phase order is
authoritative; the conceptual names are mapped onto it rather than renaming the Master.

## 2. Where the conceptual phases actually live

The conceptual Phase 3/4/5 content is already implemented inside **manifest Phase 1 (Deterministic
Runtime)**, which was intentionally a broad integration phase:

| Conceptual phase | Content | Existing implementation (manifest Phase 1) |
| --- | --- | --- |
| P3 Timeframe State | explicit timeframe/symbol identity, closed-bar identity, last processed bar, per-stream sequence, event/receive time, data-quality, health, provenance, last error, init/recovery, duplicate handling, no-lookahead | `TimeframeStateStore.h` (RT-0005), `BarFinalizer.h` (RT-0004), `AdapterManager.h` (RT-0001), `DataValidator.h` (RT-0003), `DataBus.h` (RT-0002) |
| P4 Feature/Structure/Regime | features, structure (H4 authority), regime, macro context, market quality | `FeatureEngine.h` (RT-0006), `StructureEngine.h` (RT-0007), `RegimeEngine.h` (RT-0008), `EligibilityEngine.h` (RT-0009), `MacroContextEngine.h` (RT-0013), `MarketQualityEngine.h` (RT-0014) |
| P5 Signal | signal identity, eligibility, score, confidence, risk gate, shadow-only execution | `SignalEngine.h` (RT-0010), `EligibilityEngine.h` (RT-0009), `ScoreEngine.h` (RT-0011), `ConfidenceEngine.h` (RT-0012), `RiskEngine.h` (RT-0015), `ShadowExecutionEngine.h` (RT-0016), `PositionSimulator.h` (RT-0017), `ReconciliationEngine.h` (RT-0018), `ShadowLedger.h` (RT-0019), `ReplayEngine.h` (RT-0020) |

None of this is duplicated. The continuation **hardens and proves** it rather than reimplementing it.

## 3. What was genuinely missing

1. **Integrated multi-timeframe wiring.** `TimeframeStateStore`/`FeatureEngine` were exercised only
   on single-timeframe fixtures. There was no artifact that ingests all nine streams and proves
   per-stream isolation, per-stream sequence/health/quality, failure isolation and duplicate
   rejection across the whole set.
2. **Analysis-pipeline provenance and no-lookahead proof across timeframes** (H4 authority, M15
   operational, bias, determinism, degraded-input handling).
3. **Signal-pipeline authority and shadow-only proof** (M15 trigger under H4 context, risk gate,
   no live path).
4. **The actual MT5 side.** Only `src/runtime/Mt5Boundary.md` (a contract document, RT-0021) existed.
   There was **no** MQL5 EA and **no** C++ protocol receiver implementing the approved one-EA /
   nine-logical-stream deployment.

## 4. Resolution

The continuation is executed as:

| Conceptual phase | Canonical mapping | New tasks |
| --- | --- | --- |
| P3 Timeframe State | hardening/proof phase of manifest Phase 1 timeframe state | `PH3-0001` (`src/runtime/TimeframeStateTests.cpp`) |
| P4 Feature/Structure/Regime | hardening/proof phase of manifest Phase 1 analysis engines | `PH4-0001` (`src/runtime/AnalysisPipelineTests.cpp`) |
| P5 Signal | hardening/proof phase of manifest Phase 1 signal pipeline | `PH5-0001` (`src/runtime/SignalPipelineTests.cpp`) |
| MT5 one-EA / nine streams | implements RT-0021 boundary in real source | `MT5-0001`..`MT5-0005` (see `MT5_ONE_EA_NINE_STREAM_CONTROL.md`) |

Placement decision: the conceptual P3/P4/P5 verification artifacts live under `src/runtime/`
(the layer they verify) rather than a new directory, to avoid inventing a new layer and to keep
the dependency direction (runtime depends only on foundation/resilience/runtime). The MT5 C++
receiver lives under `src/mt5/`; `src/mt5/` may depend on `foundation`, `runtime` and `resilience`,
and no lower layer depends on `src/mt5/`.

No new runtime *behaviour* contracts are added unless a genuine defect is found; the phase work is
verification-first. If a defect is found, it is corrected in the owning Phase 1 file rather than
duplicated in a new file.

## 5. Explicit non-goals

- The Master V3 document is **not** modified.
- Manifest Phases 3–11 (Self-Learning, Research Plane, Evolution, …) remain `DEFERRED` and
  untouched; the conceptual P3/P4/P5 numbering does **not** promote them.
- No new error taxonomy, no new transport speculation beyond the approved one-EA protocol,
  no live-order path.
