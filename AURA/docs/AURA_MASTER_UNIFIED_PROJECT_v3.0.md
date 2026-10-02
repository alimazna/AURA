
# XAUUSD SOVEREIGN — V3 CONTROL & IMPLEMENTATION AMENDMENT
## Current Canonical Overlay / Continuity-Safe Master Specification

**Document status:** MASTER / CANONICAL / RESEARCH-FIRST / IMPLEMENTATION-CONTROLLED  
**Version:** Unified v3.0  
**Date:** 2026-09-25  
**Purpose:** This document preserves the complete V2 master body while incorporating the subsequent project decisions, implementation workflow, file-level decomposition rules, resilience requirements, dependency discipline, and conversation-continuity rules agreed during planning.

> **Canonical precedence rule:** Sections in this V3 Control & Implementation Amendment have precedence over older statements in the preserved V2 body when there is a direct conflict. The preserved V2 body remains valuable source material and must not be deleted merely because a newer rule exists.

---

# V3-00. DOCUMENT CHARTER

This document is the project control reference for the AURA system.

It combines:

1. the original XAUUSD multi-timeframe deterministic shadow architecture;
2. the human-governed self-learning and self-evolution architecture;
3. the resilience and graceful-degradation decisions made during implementation planning;
4. the exact one-file-per-prompt implementation discipline;
5. the dependency and `BLOCKED` protocol;
6. the Phase 0 contract inventory and dependency order;
7. the rules required to continue implementation safely even when an AI conversation ends or changes.

This file is a **planning and architecture authority**, not a claim that implementation, profitability, broker validation, probability calibration, or live safety has been proven.

---

# V3-01. CANONICAL PROJECT IDENTITY

```text
PROJECT: AURA
ASSET: XAUUSD ONLY
PRIMARY MODE: SHADOW
RUNTIME CORE: LOCAL C++
MARKET/DATA BOUNDARY: MT5 / MQL5 ADAPTERS
RESEARCH / REPLAY: PYTHON-COMPATIBLE ENVIRONMENT
PRIMARY HUMAN CONTROL CENTER: DESKTOP APPLICATION
TELEGRAM: OPTIONAL AUXILIARY ASSISTANT / NOT SOURCE OF TRUTH
PERSISTENT LEDGER: HISTORICAL SOURCE OF TRUTH
PRIMARY DESIGN: MULTI-TIMEFRAME / EVENT-DRIVEN / DETERMINISTIC / BROKER-AWARE / RESEARCH-FIRST
CURRENT AI DEPENDENCY: NONE
PROFITABILITY: UNPROVEN
PROBABILITY CALIBRATION: NOT ESTABLISHED
BROKER VALIDATION: REQUIRED
```

The Desktop Application is the primary human control center. Telegram may notify, summarize, and assist, but it must never become a hidden authority or core dependency.

---

# V3-02. SOURCE-OF-TRUTH HIERARCHY

When project artifacts are read together, use this precedence order:

```text
1. EXPLICIT CURRENT HUMAN ARCHITECTURAL DECISION
2. THIS V3 MASTER CONTROL & IMPLEMENTATION AMENDMENT
3. GLOBAL AI CODING RULES
4. CURRENT TASK MANIFEST / PHASE INVENTORY
5. PRESERVED V2 MASTER BODY
6. HISTORICAL / RESEARCH NOTES
7. AI INFERENCE — NEVER AN AUTHORITY
```

An AI agent must never resolve an unresolved architectural conflict by silently choosing its own preference.

If two current sources materially conflict and there is no higher-authority resolution, the implementation task is `BLOCKED`.

---

# V3-03. NORMATIVE LANGUAGE

```text
MUST = hard project invariant
MUST NOT = forbidden behavior
SHOULD = strong recommendation unless a stronger constraint exists
MAY = optional and explicitly bounded
PROPOSED = implementation decomposition, not architecture truth by itself
OPEN DECISION = must not be silently frozen by an implementation agent
BLOCKED = implementation must stop until the named dependency/decision exists
BROKER-DEPENDENT = must be measured on the real broker/feed
RESEARCH HYPOTHESIS = not established fact
```

---

# V3-04. END-TO-END SYSTEM FLOW

The canonical runtime/research chain is:

```text
MARKET DATA
  -> DATA INGESTION
  -> DATA VALIDATION
  -> TIMEFRAME STATE
  -> FEATURES
  -> STRUCTURE
  -> REGIME
  -> STRATEGY ELIGIBILITY
  -> SIGNAL
  -> SIGNAL VALIDATION
  -> SCORE / CONFIDENCE
  -> MACRO / EVENT / MARKET-QUALITY GATES
  -> RISK PROPOSAL
  -> SHADOW EXECUTION
  -> SIMULATED POSITION LIFECYCLE
  -> OUTCOME
  -> AUDIT / RESEARCH LEDGER
  -> RESEARCH / LEARNING / EVOLUTION
  -> HUMAN GOVERNANCE
  -> APPROVED KNOWN-GOOD STATE
```

The central design principle remains:

> The system is a deterministic state machine surrounding research hypotheses, not a collection of indicators surrounding an order function.

---

# V3-05. NON-NEGOTIABLE ARCHITECTURE INVARIANTS

The following are hard project rules:

```text
Data before signal.
Causal time before performance claims.
Structure before entry.
Regime before strategy eligibility.
Signal before risk.
Risk before execution.
Execution before outcome.
Reconciliation before trust.
Research before promotion.
Shadow before live.
Evidence before claim.
Unknown is not safe or neutral.
Broker reality beats abstract assumptions.
Raw data remains traceable.
Decisions remain reproducible.
Configuration is versioned.
Failures are research data.
Score is not probability.
Confidence is not a guarantee.
No future information.
No lookahead.
No silent repainting.
No blind resend.
No unprotected exposure.
```

---

# V3-06. HUMAN AUTHORITY / AI ROLE

## Human Integration Architect

The human Integration Architect owns:

```text
architecture
contract changes
dependency decisions
file integration
integration ordering
acceptance decisions
promotion decisions
production authority
```

## AI implementation agent

The AI is an implementation worker, not the architecture authority.

It may:

```text
implement the requested file
review an already existing file when explicitly tasked
run deterministic checks within scope
identify missing dependencies
return BLOCKED when architecture is insufficient
```

It must not silently:

```text
redesign architecture
change locked contracts
create unrequested files
modify unrelated files
invent missing types
invent missing policies
bypass governance
```

---

# V3-07. ONE TASK = ONE PROMPT = ONE SOURCE FILE

This is a project-wide implementation invariant.

```text
1 TASK
   =
1 PROMPT
   =
1 OUTPUT PATH
   =
1 SOURCE FILE
```

A prompt must explicitly state the exact output path.

The AI must return the complete contents of that file and no additional source files.

A module is allowed to contain multiple files. A task is not.

---

# V3-08. DEPENDENCY CONTRACT

Every implementation prompt must explicitly contain:

```text
REQUIRED INPUT FILES
OPTIONAL INPUT FILES
DEPENDENCY RULE
```

### Required input

A file is `REQUIRED` when the target source file cannot be correctly implemented without reading its actual contract/content.

### Optional input

A file is `OPTIONAL` only when it can improve context without being required for contract correctness.

### Missing dependency behavior

If a required dependency is missing:

```text
STATUS: BLOCKED
```

Then the agent must identify:

```text
- exact missing file
- why it is required
- smallest needed decision/contract
- affected task(s) or file(s), when known
```

The agent must not create a substitute file.

---

# V3-09. PROMPT STANDALONE / CONVERSATION-SAFE RULE

Every task prompt must be understandable without relying on memory of an earlier AI conversation.

The prompt must carry or clearly identify:

```text
PROJECT ID
TASK ID
OUTPUT PATH
OBJECTIVE
REQUIRED DEPENDENCIES
OPTIONAL DEPENDENCIES
TASK-SPECIFIC REQUIREMENTS
FORBIDDEN SCOPE
ACCEPTANCE TESTS
INTEGRATION NOTES
BLOCKED BEHAVIOR
ONE-FILE OUTPUT RULE
```

A prompt may refer to the **Global AI Coding Rules** as a global companion document when that file is supplied once at the beginning of an AI conversation.

No task may depend on the AI remembering a contract merely because it appeared in an earlier chat message.

---

# V3-10. WHAT THE USER SENDS TO AN AI CODING CHAT

## Initial bootstrap for a new AI coding conversation

Send:

```text
1. XAUUSD_SOVEREIGN_GLOBAL_AI_CODING_RULES_v1.0.md
2. The relevant task prompt
3. Only the REQUIRED INPUT FILES listed by that task
4. Optional files only when genuinely useful
```

Do not automatically send the entire project or entire Master for every task.

## If a task says `Required files: None`

Send the Global Rules + the task prompt.

## If a task lists dependencies

Send the Global Rules + the task prompt + every required dependency exactly as named.

## If the AI reports `BLOCKED`

Stop that task. Resolve the named architectural/dependency issue first. Then regenerate/reissue the task prompt if the decision changes its contract.

---

# V3-11. BLOCKED IS A VALID SUCCESS STATE

`BLOCKED` means the system prevented an AI agent from silently inventing architecture.

It is preferable to a superficially compiling file built on false assumptions.

Typical causes:

```text
missing dependency
open contract decision
conflicting canonical sources
unspecified serialization identity
unknown versioning semantics
missing broker contract
missing platform contract
missing security boundary
```

---

# V3-12. REVIEW / INTEGRATION PIPELINE

Every implementation follows:

```text
TASK READY
  -> BUILD / IMPLEMENT
  -> REVIEW
  -> INTEGRATION
  -> TEST
  -> APPROVED / REWORK / BLOCKED
```

Important separation:

```text
IMPLEMENTATION AGENT = writes the requested file
REVIEW AGENT = analyzes correctness; does not modify code unless explicitly assigned a separate implementation task
INTEGRATION = checks system-level compatibility
TEST = validates behavior and invariants
```

A file is not `INTEGRATED` merely because it compiles.

Integration must consider:

```text
compile
link
contracts
dependency direction
circular dependencies
namespace conflicts
schema compatibility
ownership
idempotency
state transitions
persistence implications
versioning
observability
fault behavior
```

---

# V3-13. RESILIENCE AND GRACEFUL DEGRADATION

The project explicitly requires that failure of a non-critical subsystem does not automatically fail the whole application.

Canonical pattern:

```text
FAILURE
  -> ISOLATE
  -> MARK FAILED / DEGRADED / OFFLINE / RECOVERING / PAUSED / BLOCKED / ERROR
  -> DISABLE ONLY DEPENDENT CAPABILITIES
  -> KEEP SAFE INDEPENDENT CAPABILITIES RUNNING
  -> ALERT HUMAN
  -> RECOVER WHEN POSSIBLE
```

Never fabricate missing data.
Never treat stale data as fresh.
Never treat `UNKNOWN` as safe or neutral.

---

# V3-14. SYSTEM VS SERVICE STATES

## Service/subsystem state

```text
STARTING
ONLINE
DEGRADED
OFFLINE
RECOVERING
PAUSED
BLOCKED
ERROR
```

## System operating mode

```text
STARTING
RECOVERY
NORMAL
DEGRADED
SHADOW
PAUSED
MANUAL
EMERGENCY
HALTED
```

These are different concepts and should not be merged just for implementation convenience.

---

# V3-15. GRACEFUL-DEGRADATION DECISIONS

The following are explicit project decisions:

### Self-Learning paused/stopped

The system may continue:

```text
prediction
analysis
risk processing
shadow operation
monitoring
persistence
desktop control
```

The UI must identify:

```text
what paused
why it paused
which capabilities are affected
which capabilities remain operational
```

### Individual timeframe feed stopped

The application continues when safe, and reports the exact failed timeframe.

Examples:

```text
M1 OFFLINE
-> M1 microstructure unavailable
-> M1-dependent features disabled
-> M15/H4 may continue if independently healthy

M5 OFFLINE
-> local execution-context unavailable
-> M5-dependent filters disabled

M15 OFFLINE
-> primary M15 setup unavailable
-> M15-triggered decisions blocked

H4 OFFLINE
-> structural authority unavailable
-> H4-dependent strategies blocked

TICK OFFLINE
-> quote freshness / execution state unknown
-> new execution proposals blocked
```

The exact capability dependency graph must be encoded rather than implied by UI behavior.

---

# V3-16. PROPOSED RESILIENCE PLANE

Because resilience is now a project decision, its decomposition is explicitly planned as **Phase 0.5** between immutable foundations and deterministic runtime.

Proposed contract concepts:

```text
ServiceDescriptor
CapabilityId
CapabilityDescriptor
DependencyDescriptor
HealthSnapshot
FreshnessState
RecoveryAction
DegradationImpact
```

Proposed runtime/resilience components:

```text
SystemSupervisor
CapabilityRegistry
DependencyGraph
HealthStateEngine
DataFreshnessMonitor
StaleCandleDetector
SubsystemIsolationManager
GracefulDegradationManager
RecoveryManager
PauseResumeManager
CriticalityPolicy
ImpactResolver
```

These names are decomposition proposals, not automatic canonical filenames until accepted through integration review.

---

# V3-17. GUARDIAN / SAFETY AUTHORITY

The Guardian / protected control plane has higher authority than research or evolution.

Critical integrity failures may force:

```text
SAFE MODE
or
HALT
```

The Guardian must not be self-evolved by the evolution engine.

Automatic rollback is allowed only to a previously approved `KNOWN-GOOD` state under explicit policy.

The system may preserve itself; it may not grant itself new authority.

---

# V3-18. SELF-LEARNING / SELF-EVOLUTION BOUNDARY

Current system status:

```text
CURRENT AI DEPENDENCY = NONE
SELF-LEARNING = ALGORITHMIC / STATISTICAL / RULE-BASED KNOWLEDGE ACQUISITION
SELF-LEARNING != SELF-MODIFICATION
EVOLUTION = CANDIDATE GENERATION + VALIDATION, NOT AUTHORITY
```

Allowed research behavior:

```text
observation
detection
diagnosis
root-cause analysis
knowledge acquisition
contradiction tracking
decay / revalidation
hypothesis creation
bounded experiments
candidate generation
candidate comparison
validation
stress / regression
checkpointing
research reporting
no-change conclusions
```

Forbidden behavior:

```text
direct production mutation
bypass of approval
modification of evaluator
unlocking protected holdout
erasing history
rewriting provenance
changing criteria after results
budget escalation
operating-window escalation
credential escalation
Guardian modification
auto-promotion
```

---

# V3-19. HUMAN-GOVERNED EVOLUTION

The autonomy model is separated into distinct concerns:

```text
LEARNING      = acquires knowledge
RESEARCH      = tests knowledge
EVOLUTION     = creates candidates
EVALUATION    = judges evidence
HUMAN         = grants authority
RUNTIME       = executes approved state
GUARDIAN      = protects the system
OPERATING WIN = bounds time/resources
MEMORY        = preserves history
```

The system may conclude:

```text
NO_CHANGE
INSUFFICIENT_EVIDENCE
INCONCLUSIVE
REFUTED
CONTRADICTED
```

These are valid outcomes, not failures of autonomy.

---

# V3-20. DAILY OPERATING WINDOW

The operating window is human-defined and bounded:

```text
3–8 HOURS / DAY
```

The system may not extend this window on its own.

At the end of the window:

```text
STOP NEW NON-CRITICAL WORK
-> CLASSIFY ACTIVE WORK
-> CHECKPOINT RESUMABLE WORK
-> FINALIZE CRITICAL PERSISTENCE / AUDIT
-> VERIFY PERSISTENCE
-> DRAIN
-> OFFLINE
```

This is a controlled draining event, not an unsafe hard kill.

Separate schedules for runtime and research are allowed and must remain bounded by authority.

---

# V3-21. CHECKPOINT / CRASH RECOVERY

Long-running research and runtime state must support controlled interruption where applicable.

At minimum, a valid checkpoint concept must preserve enough state to continue without corrupting history or silently replaying invalid work.

Checkpoint validity, provenance, version, and recovery result must be auditable.

Recovery must prefer:

```text
known valid checkpoint
or
known-good state
```

rather than guessing.

---

# V3-22. PERSISTENCE REQUIREMENT

Critical state must survive process restart.

At minimum, the architecture requires persistence for relevant:

```text
adapter health
last processed bar by timeframe
decision IDs
signals
scores / confidence
risk proposals
shadow positions / fills / outcomes
configuration versions
strategy versions
system mode
errors / alerts
research / experiment IDs
```

Persistence failure is a safety event.

The persistent ledger is the historical source of truth.

---

# V3-23. MESSAGE / EVENT / DECISION IDENTITY

Minimum event metadata remains:

```text
event_id
event_type
symbol
source
source_instance
event_time
receive_time
sequence_id
schema_version
```

Minimum message metadata remains:

```text
protocol_version
schema_version
message_id
timestamp
source
destination
payload
checksum/hash where appropriate
```

Receivers should reject or quarantine appropriately when metadata is:

```text
unknown protocol
invalid schema
duplicate
expired
future-dated
clock-anomalous
invalid payload
unknown source
```

Signal identity should remain deterministic. The project’s proposed decision identity is:

```text
DecisionID = Hash(
    symbol,
    trigger timeframe,
    closed bar ID,
    strategy version,
    configuration version
)
```

Idempotency is mandatory at relevant boundaries.

---

# V3-24. CLOCK AND CAUSALITY

The system uses distinct temporal concepts where needed:

```text
event_time
receive_time
closed-bar time
processing time
research time
```

No future information may enter a decision.

No hidden repainting.

No performance claim may be based on information unavailable at the decision time.

The distinction between structural clock, execution clock, safety clock, and research clock is preserved from the V2 architecture.

---

# V3-25. DATA QUALITY

Canonical data quality states:

```text
VALID
DEGRADED
INVALID
UNKNOWN
STALE
MISSING
OUT_OF_ORDER
DUPLICATE
INCOMPLETE
```

`UNKNOWN` is not a neutral success state.

Data quality must propagate into capability eligibility and decision gating rather than merely appearing as a warning in logs.

---

# V3-26. CONFIGURATION AND VERSIONING

Configuration must be versioned.

Relevant decisions may depend on:

```text
thresholds
weights
risk limits
strategy enablement
event policies
execution assumptions
alert thresholds
simulation parameters
```

Runtime configuration and research configuration are distinct concepts.

Every important decision should retain the relevant:

```text
configuration version
strategy version
model version, where applicable
```

No silent configuration mutation is allowed.

---

# V3-27. ERROR / ALERT / AUDIT

Errors are structured records, not plain strings.

At minimum an error record should carry appropriate:

```text
error_id
component
severity
timestamp
state
message
context
recovery_action
```

Audit remains append-only.

Historical records cannot be rewritten merely to make the current system look correct.

Failed, rejected, or rolled-back candidates remain research data.

---

# V3-28. MODULE CONTRACT REGISTRY

Canonical module contracts currently recognized by the Master are:

```text
AdapterManager
DataBus
DataValidator
BarFinalizer
TimeframeStateStore
FeatureEngine
StructureEngine
RegimeEngine
EligibilityEngine
SignalEngine
ScoreEngine
ConfidenceEngine
ProbabilityEngine
MacroContextEngine
MarketQualityEngine
RiskEngine
PortfolioRiskEngine
ShadowExecutionEngine
PositionSimulator
ReconciliationEngine
PersistenceEngine
ResearchEngine
ReplayEngine
AlertEngine
TelegramGateway
HealthMonitor
Watchdog
ConfigurationRegistry
AuditEngine
```

These are canonical modules, not automatically one-file modules.

No module should silently bypass another canonical stage in the data/decision chain.

---

# V3-29. TIMEFRAME AUTHORITY

Nine MT5/MQL5 timeframe adapters are canonical:

```text
M1
M5
M15
M30
H1
H4
D1
W1
MN1
```

Current analytical authority model:

```text
M15 = PRIMARY OPERATIONAL / SETUP TIMEFRAME
H4  = PRIMARY STRUCTURAL AUTHORITY
H1/M30 = INTERMEDIATE CONTEXT
M5/M1/TICK = EXECUTION / MICROSTRUCTURE CONTEXT
D1/W1/MN1 = LONG-HORIZON CONTEXT
```

The M15/H4 dual-engine relationship is preserved.

---

# V3-30. CORE STRATEGY FAMILIES

The preserved Master baseline includes:

```text
S01 — Secular Trend Following
S02 — Multi-Year Macro Swing
S03 — Sovereign Breakout
S04 — Regime Transition
S05 — Structural Retest
S06 — Defensive Exit / De-Risking
```

Other strategy-family concepts such as trend following, mean reversion, and breakout eligibility remain subject to the canonical baseline and empirical validation.

Strategy-specific parameters remain research hypotheses unless explicitly frozen.

---

# V3-31. SCORE / CONFIDENCE / PROBABILITY

The project keeps these concepts separate:

```text
SCORE != PROBABILITY
CONFIDENCE != GUARANTEE
```

Probability calibration is not established.

A score may be useful as a deterministic ranking/quality mechanism without being called a probability.

Any future calibrated probability claim must be supported by appropriate evidence and validation.

---

# V3-32. SHADOW IS THE PRIMARY OPERATING MODE

SHADOW is not merely a signal-display mode.

It must model the relevant simulated execution lifecycle, including where appropriate:

```text
opportunity
risk proposal
order proposal
simulated fill
slippage
commission
swap
partial-fill behavior
position lifecycle
exit
outcome
reconciliation
```

A shadow result is evidence for research, not proof of live profitability or execution equivalence.

---

# V3-33. RECONCILIATION / BROKER REALITY

Broker-specific reality beats abstract assumptions.

The broker profile should be measured for:

```text
spread
quote behavior
tick freshness
symbol specification
volume constraints
margin behavior
execution assumptions
market sessions
gaps / weekend behavior
cost model
latency characteristics
```

The shadow engine should distinguish assumed behavior from observed broker behavior.

---

# V3-34. RESEARCH / VALIDATION FIREWALLS

Research architecture must preserve:

```text
point-in-time data
locked holdout boundaries
candidate/control separation
evaluator integrity
metric versioning
multiple-testing awareness
OOS validation
walk-forward validation
stress testing
regression testing
trial-history preservation
```

A candidate cannot modify the evaluator that judges the candidate.

Changing metric definitions creates a new evidence family rather than rewriting the old one.

---

# V3-35. RESEARCH MEMORY

The system may preserve:

```text
knowledge objects
knowledge history
contradictions
failure memory
root-cause analyses
hypotheses
experiments
experiment fingerprints
candidate history
evidence
governance decisions
incidents
postmortems
```

Failed or rejected research is valuable historical data and must remain visible.

---

# V3-36. RESEARCH EXIT STATES

Research may terminate with:

```text
SUPPORTED
REFUTED
INCONCLUSIVE
CONTRADICTED
NO_CHANGE
DATA_INSUFFICIENT
VALIDATION_FAILED
ROBUSTNESS_FAILED
REGRESSION_FAILED
BUDGET_EXHAUSTED
CONTAMINATED
HUMAN_REJECTED
HUMAN_REQUESTED_MORE_RESEARCH
APPROVED_FOR_SHADOW
APPROVED_FOR_PROMOTION_REVIEW
```

No-change and insufficient-evidence outcomes are legitimate.

---

# V3-37. DESKTOP CONTROL CENTER

The Desktop Application is the primary control center and should eventually expose the full control context, including as appropriate:

```text
system overview
market/data health
signals
risk
shadow positions
research
knowledge
candidates
validation evidence
approval center
evolution graph
incidents
schedule
checkpoints
health
watchdog
audit
configuration/version context
```

The Desktop Application does not make an AI agent the authority; it is the human control surface for the protected control plane.

---

# V3-38. TELEGRAM

Telegram is explicitly auxiliary.

It may provide:

```text
operations status
periodic reports
sniper alerts
system warnings
research/governance notifications
authenticated bounded requests
```

It must not:

```text
become source of truth
bypass approval
replace desktop authority
become required for core safety
```

Telegram outage is non-fatal to the core when the rest of the system is healthy.

---

# V3-39. CURRENT PROJECT STATUS

```text
ARCHITECTURE = DEFINED
FILE-LEVEL IMPLEMENTATION = IN PROGRESS / TO BE BUILT
EMPIRICAL VALIDATION = REQUIRED
BROKER VALIDATION = REQUIRED
PROBABILITY CALIBRATION = NOT ESTABLISHED
PROFITABILITY = UNPROVEN
PRIMARY MODE = SHADOW
SELF-LEARNING = RESEARCH / ALGORITHMIC, OPTIONAL TO RUNTIME
SELF-EVOLUTION = SANDBOX + VALIDATION + HUMAN GATE
LIVE AUTOMATION = NOT INITIAL OBJECTIVE
```

---

# V3-40. IMPLEMENTATION PHASES — CURRENT AUTHORITATIVE ORDER

```text
PHASE 0
Immutable Foundations

PHASE 0.5
Resilience & Graceful Degradation

PHASE 1
Deterministic Runtime

PHASE 2
Observation & Outcomes

PHASE 3
Self-Learning

PHASE 4
Research Plane

PHASE 5
Evolution

PHASE 6
Validation

PHASE 7
Governance

PHASE 8
Operating Window & Recovery

PHASE 9
Desktop Control Center

PHASE 10
Auxiliary Telegram

PHASE 11
Controlled Real-World Validation
```

### Phase 0 — Immutable Foundations

```text
Repository
Configuration Schema
Versioning Rules
Audit Format
Identity / Hashing
Basic Persistence
Guardian Skeleton
```

### Phase 0.5 — Resilience & Graceful Degradation

```text
Service State
System Supervision
Capability Registration
Dependency Graph
Health State
Data Freshness
Stale Detection
Subsystem Isolation
Graceful Degradation
Recovery
Pause / Resume
Critical vs Non-Critical Policy
Impact Resolution
Degraded UI State
Degraded Alerts
Degradation Tests
```

### Phase 1 — Deterministic Runtime

```text
MT5 adapters
C++ data bus
data validation
timeframe state
core runtime
shadow ledger
```

### Phase 2 — Observation & Outcomes

```text
prediction ledger
outcome engine
failure detection
system health
```

### Phase 3 — Self-Learning

```text
knowledge store
knowledge lifecycle
context learning
contradiction engine
knowledge decay
failure memory
```

### Phase 4 — Research Plane

```text
hypothesis engine
research planner
experiment ledger
research budgets
experiment fingerprint
sandbox
```

### Phase 5 — Evolution

```text
candidate generator
candidate registry
evolution graph
candidate comparison
```

### Phase 6 — Validation

```text
validation firewall
OOS
walk-forward
stress
regression
holdout service
evaluator firewall
```

### Phase 7 — Governance

```text
approval gate
change investigation report
promotion gate
known-good registry
rollback
incident / postmortem
```

### Phase 8 — Operating Window & Recovery

```text
schedule manager
3–8h enforcement
DRAINING
checkpoint/resume
crash recovery
resource budgets
```

### Phase 9 — Desktop Control Center

```text
dashboard
research UI
knowledge UI
candidate UI
approval center
evolution graph UI
incident UI
schedule UI
audit UI
```

### Phase 10 — Auxiliary Telegram

```text
operations assistant
governance notifications
authenticated requests
```

### Phase 11 — Controlled Real-World Validation

Only after the preceding layers are stable and the applicable evidence/safety gates have been satisfied.

---

# V3-41. MVP ENGINEERING STRATEGY

The first implementation should not attempt every strategy and every timeframe as an independent finished system.

The Master baseline recommends an MVP centered on:

```text
XAUUSD
M15
H4
M5
M1
basic higher-timeframe context
data validation
regime
one strategy family
risk proposal
shadow execution
shadow ledger
health monitoring
replay
```

Telegram may be added as an auxiliary layer after the core is trustworthy.

Then expand authority incrementally.

Suggested analytical expansion order:

```text
M15
-> H4
-> M5/M1
-> H1/M30
-> D1
-> W1
-> MN1
```

Adapters may exist earlier than analytical authority.

---

# V3-42. PHASE 0 FILE-LEVEL CANONICAL DECOMPOSITION

The current implementation inventory contains 36 Phase 0 tasks.

## Foundation contracts

```text
FND-0001 EntityId.h
FND-0002 Timestamp.h
FND-0003 Version.h
FND-0004 ServiceState.h
FND-0005 SystemMode.h
FND-0006 HashDigest.h
FND-0007 DataQualityState.h
FND-0008 ProtocolVersion.h
FND-0009 SchemaVersion.h
FND-0010 MessageMetadata.h
FND-0011 EventId.h
FND-0012 EventType.h
FND-0013 EventMetadata.h
FND-0014 ErrorSeverity.h
FND-0015 ErrorCode.h
FND-0016 ErrorRecord.h
FND-0017 RecoveryAction.h
```

## Configuration

```text
CFG-0001 ConfigurationKey.h
CFG-0002 ConfigurationValue.h
CFG-0003 ConfigurationSnapshot.h
CFG-0004 ConfigurationScope.h
CFG-0005 ConfigurationChange.h
```

## Audit

```text
AUD-0001 AuditAction.h
AUD-0002 AuditOutcome.h
AUD-0003 AuditRecord.h
```

## Integrity

```text
INT-0001 HashAlgorithm.h
INT-0002 IHasher.h
INT-0003 Hasher.cpp
```

## Persistence

```text
PER-0001 PersistenceStatus.h
PER-0002 PersistenceRecordMetadata.h
PER-0003 IPersistenceStore.h
PER-0004 PersistenceTransaction.h
```

## Guardian

```text
GDN-0001 GuardianStatus.h
GDN-0002 GuardianPolicy.h
GDN-0003 IGuardian.h
GDN-0004 Guardian.h
```

These task/file names are the current implementation decomposition. `CANONICAL`, `PROJECT DECISION`, `PROPOSED`, and `OPEN DECISION` status must remain distinct.

---

# V3-43. PHASE 0 TASK STATUS DEFINITIONS

```text
PLANNED
READY
ASSIGNED
IN_PROGRESS
IMPLEMENTED
REVIEW_PENDING
REVIEW_FAILED
APPROVED
INTEGRATED
TESTED
BLOCKED
DEFERRED
REPLACED
```

A task status tracks process state and must not be confused with source authority.

---

# V3-44. PHASE 0 DEPENDENCY WAVES

## Wave 0A — Pure foundations

```text
FND-0001
FND-0002
FND-0003
FND-0004
FND-0005
FND-0006
FND-0007
FND-0008
FND-0009
FND-0012
FND-0014
FND-0017
```

## Wave 0B — Dependent foundation

```text
FND-0011 <- FND-0001
FND-0010 <- FND-0001, FND-0002, FND-0008, FND-0009
FND-0013 <- FND-0001, FND-0002, FND-0009, FND-0011, FND-0012
FND-0015 <- architectural decision / ErrorCode taxonomy
FND-0016 <- FND-0015 + FND-0017 + FND-0002/FND-0004/FND-0014 as required by frozen contract
```

## Wave 0C — Configuration

```text
CFG-0001
CFG-0002
CFG-0003
CFG-0004
CFG-0005
```

Configuration tasks may have internal dependencies; do not infer their final type system until the contract is frozen.

## Wave 0D — Audit / Integrity / Persistence

```text
AUD-0001
AUD-0002
AUD-0003
INT-0001
INT-0002
INT-0003
PER-0001
PER-0002
PER-0003
PER-0004
```

## Wave 0E — Guardian

```text
GDN-0001
GDN-0002
GDN-0003
GDN-0004
```

Guardian implementation must not silently redefine the protected control-plane authority.

---

# V3-45. ERROR-CONTRACT NUMBERING RESOLUTION

This section explicitly resolves a previous planning inconsistency.

The current authoritative Phase 0 numbering is:

```text
FND-0014 = ErrorSeverity.h
FND-0015 = ErrorCode.h
FND-0016 = ErrorRecord.h
FND-0017 = RecoveryAction.h
```

`FND-0015 ErrorCode.h` is currently an **OPEN DECISION** unless the authoritative specification explicitly freezes its semantics/taxonomy.

`FND-0016 ErrorRecord.h` is **BLOCKED** until the `ErrorCode` contract and required `RecoveryAction` semantics are frozen sufficiently to make the record stable.

The following old alternative:

```text
FND-0015 = ErrorRecord.h
```

is superseded by V3 and must not be used for new task generation.

---

# V3-46. PHASE 0 STATUS CLASSIFICATION

Use these authority labels:

```text
CANONICAL
Directly supported by the Master architecture.

PROJECT DECISION
Explicitly decided during planning and now required by the project.

PROPOSED
Useful decomposition for implementation, still subject to integration review.

OPEN DECISION
Not safe to freeze without a human architectural decision.
```

No AI may upgrade `PROPOSED` or `OPEN DECISION` into `CANONICAL` by implication.

---

# V3-47. FILE OWNERSHIP RULES

Each source path has one implementation owner at a time.

No two active tasks may own the same file.

A task must not modify a dependency file merely because the dependency makes the target harder to implement.

If the dependency is wrong, create a separate contract-change task under human architectural authority.

---

# V3-48. PARALLELIZATION RULE

Tasks may run in parallel only when:

```text
all required dependencies are complete
AND
no two tasks own the same file
AND
no task changes a locked contract
AND
module boundaries remain independent
AND
shared architectural decisions are already frozen
```

Parallel execution is subordinate to contract stability.

---

# V3-49. INTEGRATION CHECKLIST

Every integrated file should be checked for:

```text
correct path
correct namespace
correct includes
supported language standard
compile correctness
link correctness where relevant
public API correctness
dependency direction
no circular dependency
no hidden higher-layer dependency
no duplicate type definition
no accidental ownership overlap
idempotency where relevant
determinism where required
versioning/provenance
error semantics
observability
failure behavior
persistence implications
```

---

# V3-50. CONTINUITY / HANDOFF PROTOCOL

The project is designed so that the end of an AI conversation does not destroy project state.

The human should treat persistent files as the state, not the chat history.

Minimum persistent control set:

```text
XAUUSD_SOVEREIGN_MASTER_UNIFIED_PROJECT_v3.0.md
XAUUSD_SOVEREIGN_GLOBAL_AI_CODING_RULES_v1.0.md
XAUUSD_SOVEREIGN_AI_PROMPT_BUILD_PROTOCOL_v1.0.md
XAUUSD_SOVEREIGN_MASTER_TASK_MANIFEST...
XAUUSD_SOVEREIGN_PHASE0_IMPLEMENTATION_INVENTORY...
source tree
review records
integration/test records
```

When a conversation ends, the next conversation does not need to remember the previous one.

The next task is resumed from:

```text
TASK MANIFEST STATUS
+
CURRENT TASK ID
+
CURRENT FILES ON DISK
+
REVIEW/INTEGRATION/TEST STATUS
+
BLOCKED DECISIONS
```

No essential architecture should exist only in chat memory.

---

# V3-51. TASK RESUME RULE

For a task that is `IN_PROGRESS`, `REVIEW_PENDING`, or otherwise interrupted:

1. Read the task manifest.
2. Identify the exact task ID and output path.
3. Read the task prompt.
4. Read only the required dependencies.
5. Inspect the current file if it already exists.
6. Do not assume that an earlier AI response is correct merely because it exists.
7. Continue through the review/integration/test lifecycle.

If the current file conflicts with the frozen contract, the conflict must be surfaced rather than silently repaired across unrelated files.

---

# V3-52. PROMPT GENERATION CONTRACT

Every generated implementation prompt should contain, at minimum:

```text
TASK ID
PHASE
MODULE
OUTPUT PATH
SOURCE AUTHORITY STATUS
OBJECTIVE
REQUIRED INPUT FILES
OPTIONAL INPUT FILES
DEPENDENCY CHECK
IMPLEMENTATION REQUIREMENTS
FORBIDDEN SCOPE
ACCEPTANCE TESTS
INTEGRATION NOTES
BLOCKED CONDITION
OUTPUT-ONLY-ONE-FILE RULE
```

Recommended dual form:

```text
SHORT PROMPT = compact execution prompt
FULL PROMPT  = complete standalone implementation contract
```

The short prompt must not weaken a hard invariant from the full prompt.

---

# V3-53. REVIEW PROMPT CONTRACT

A reviewer prompt must explicitly say:

```text
REVIEW ONLY
DO NOT MODIFY CODE
```

It should check:

```text
contract compliance
architecture alignment
dependency correctness
error semantics
edge cases
determinism
state semantics
security boundaries
persistence implications
performance risks
testability
forbidden behavior
```

A reviewer may recommend a change but must not silently implement that change in the review task.

---

# V3-54. INTEGRATION AUTHORITY

Integration is where file-level output becomes part of the system.

The Integration Architect decides whether a file is:

```text
ACCEPTED
REWORK REQUIRED
BLOCKED
REPLACED
```

Compilation alone is not sufficient for acceptance.

---

# V3-55. RESEARCH-FIRST CLAIM DISCIPLINE

The project must never claim:

```text
profitability proven
probability calibration proven
live safety proven
broker-independent behavior
shadow/live equivalence
automatic evolution is inherently safe
```

unless the appropriate evidence actually establishes the claim.

The preferred evidence sequence remains:

```text
OBSERVATION
-> HYPOTHESIS
-> CONTROLLED EXPERIMENT
-> VALIDATION
-> OOS / WALK-FORWARD / STRESS / REGRESSION
-> EVIDENCE PACKAGE
-> HUMAN DECISION
-> SHADOW
-> PROMOTION REVIEW
```

---

# V3-56. CANONICAL DATABASE DOMAINS

The preserved V2 domains remain, with the V2 additions formalized around:

```text
knowledge_objects
knowledge_history
knowledge_contradictions
knowledge_revalidation
failures
failure_evidence
rca_cases
hypotheses
research_campaigns
experiments
experiment_trials
experiment_fingerprints
candidates
candidate_artifacts
validation_runs
validation_evidence
holdout_queries
contamination_records
evaluator_versions
metric_registry_versions
human_decisions
change_proposals
promotion_packages
known_good_versions
incidents
postmortems
evolution_edges
evidence_edges
checkpoints
operating_schedules
operating_window_events
resource_budgets
resource_events
learning_cycles
guardian_events
system_snapshots
```

---

# V3-57. CANONICAL RESEARCH GAPS

The following remain explicitly unresolved research questions rather than architecture defects:

```text
adaptive search under non-stationarity
self-evolving validation safety
RCA + evolution + statistical validity
human review compression
failure learning without evidence leakage
meta-evolution safety
adaptive holdout design
knowledge confidence calibration
complexity penalties
broker-profile drift
checkpoint granularity
human-decision representation
minimum shadow duration by candidate class
```

An unresolved research question must remain visible.

---

# V3-58. FINAL V3 DEFINITION OF DONE

The system should eventually demonstrate:

```text
reproducible observation
structured failure objects
bounded knowledge acquisition
falsifiable hypotheses
bounded experiments
isolated candidates
candidate/control comparison
protected evaluator
protected holdout
OOS / walk-forward / stress / regression where applicable
complete Change Investigation Report
Desktop Approval Center
human decision attribution
Guardian integrity
known-good rollback
3–8h operating window
draining
checkpoint/resume
crash recovery
Desktop as primary control center
Telegram as auxiliary layer
graceful subsystem degradation
exact timeframe-failure visibility
persistent audit/history
```

---

# V3-59. FINAL CANONICAL PRINCIPLE

> **The project is built so that the intelligence may explore, the research plane may learn, the evolution plane may create candidates, the evaluator may judge evidence, the human may grant authority, the runtime may execute only approved state, and the Guardian may protect the system. No AI conversation, code generator, failed experiment, or temporary runtime state is allowed to become the hidden authority of the project.**

---

# V3-60. PRESERVED V2 BODY

The complete V2 master is preserved below so that no previously written architecture, equations, research material, implementation detail, source mapping, or appendix is silently lost.


# XAUUSD SOVEREIGN — MASTER PROJECT REFERENCE
## Unified Research, Architecture, Shadow System, Self-Evolution, Human Governance & Implementation Specification

**Document status:** MASTER / RESEARCH-FIRST / IMPLEMENTATION-READY BLUEPRINT  
**Version:** Unified v2.0  
**Generated:** 2026-09-25 08:55 UTC  
**Primary objective:** establish one canonical reference that consolidates the existing XAUUSD system architecture with the researched Human-Governed Self-Evolution architecture and the decisions made during planning.

> **This document is the canonical planning reference.** It does not prove profitability, does not provide investment advice, does not authorize unattended production evolution, and does not authorize the self-system to extend its own operating window, authority, safety boundaries, or protected control plane.

---

# 0. DOCUMENT CHARTER

## 0.1 Why this document exists

The project has two bodies of work that must now be treated as one system:

1. The **AURA Multi-Timeframe Shadow Research System**, which defines the market-data, state, feature, regime, strategy, risk, shadow-execution, replay, validation, audit, and runtime foundations.
2. The **Human-Governed Autonomous Research & Evolution Engine**, which defines how the system can observe its own behavior, detect weaknesses, form falsifiable hypotheses, perform controlled experiments, generate candidate versions, validate them, present evidence to a human, and evolve only through explicit governance.

The goal of this unified reference is not to replace the original architecture, but to place the new evolution/research layer around it without violating its core boundaries.

## 0.2 Canonical statement

> **The system is autonomous in research and bounded in action; the human is the final authority for production change; the evaluator is a protected authority for evidence; production remains a controlled state rather than a self-modifying workspace.**

## 0.3 Core separation

```text
INTELLIGENCE
    ├─ Observation
    ├─ Diagnosis
    ├─ Hypothesis
    ├─ Experiment design
    ├─ Candidate generation
    └─ Research analysis

AUTHORITY
    ├─ Policy
    ├─ Risk constraints
    ├─ Evaluator ownership
    ├─ Production version selection
    └─ Human approval

EVIDENCE
    ├─ Versioned datasets
    ├─ Point-in-time data
    ├─ Validation protocol
    ├─ Holdout boundary
    ├─ Trial history
    └─ Audit/provenance

EXECUTION
    ├─ Shadow
    ├─ Controlled deployment
    ├─ Monitoring
    └─ Rollback to known-good state
```

## 0.4 Non-goals

This project reference does **not** assert that:

- the strategy is profitable;
- a score is a calibrated probability;
- a backtest is evidence sufficient for live deployment;
- an evolutionary search is automatically safe;
- an LLM should be given production authority;
- a self-improving system should modify its own evaluator;
- every detected anomaly requires a strategy change.

## 0.5 Current project status

```text
ARCHITECTURE:               DEFINED
IMPLEMENTATION:             TO BE BUILT / ITERATIVE
BROKER VALIDATION:          REQUIRED
NUMERICAL PARAMETERS:       RESEARCH HYPOTHESES
PROBABILITY CALIBRATION:    NOT ESTABLISHED
PROFITABILITY:              UNPROVEN
SHADOW ENGINE:              CORE TARGET
SELF-EVOLUTION:             RESEARCH SANDBOX + HUMAN-GATED
TELEGRAM:                   OPTIONAL ASSISTANT LAYER
LIVE AUTOMATION:            NOT THE INITIAL OBJECTIVE
```

---

# 1. SOURCE INTEGRATION MAP

This unified document was derived from the following project materials:

| Source | Role in this master |
|---|---|
| `XAUUSD_SOVEREIGN_MASTER_PROJECT_REFERENCE(2)(4).md` | Canonical XAUUSD architecture, runtime, data, state, strategy, risk, shadow, validation, implementation foundations |
| `Pasted markdown(10).md` | Deep research on self-evolution, human governance, evaluator security, overfitting protection, provenance, shadow/canary, and evolution architecture |
| `Pasted markdown(8)(1)(2).md` | Prior planning decisions about the desktop application, Telegram assistants, approval workflow, automatic error discovery, research priority, and self-learning without direct self-modification |

## 1.1 Source integrity fingerprints

```text
SOURCE SHA-256

core:
b3b3480a6905bbacb26cdbf0649a1b927a914de31f5ccc8b4903b0455c2a519c

research:
0436cfb456e21ee24cb2a62102a708a617c82b695a7be588eb3ee7287b5d291c

planning-notes:
4d6883c5aca816800ec79dc92fb5b11b75365b212264c3bce41d817fc067e8d2
```

## 1.3 Version 2.0 Update Charter

Version 2.0 incorporates the project decisions made after the original unified reference was created. The update is intentionally **additive and normative**: the preserved baseline remains the technical market-system foundation, while the new sections below define the stronger learning, evolution, self-preservation, application-control, and scheduling architecture that now surrounds it.

The Version 2.0 update establishes the following project-level truths:

```text
1. THE SYSTEM CURRENTLY CONTAINS NO AI DEPENDENCY.

2. SELF-LEARNING IS ALGORITHMIC / STATISTICAL / RULE-BASED KNOWLEDGE ACQUISITION.

3. SELF-LEARNING DOES NOT IMPLY SELF-MODIFICATION.

4. SELF-EVOLUTION CREATES CANDIDATES; IT DOES NOT GRANT AUTHORITY.

5. HUMAN APPROVAL IS A CONTROL-PLANE AUTHORITY, NOT A METRIC.

6. THE DESKTOP APPLICATION IS THE PRIMARY CONTROL CENTER.

7. TELEGRAM IS OPTIONAL AUXILIARY INTERFACE / ASSISTANT INFRASTRUCTURE.

8. THE GUARDIAN / PROTECTED CORE HAS HIGHER PRIORITY THAN EVOLUTION.

9. THE SYSTEM OPERATES INSIDE A HUMAN-DEFINED DAILY WINDOW OF 3–8 HOURS.

10. THE OPERATING WINDOW CANNOT BE EXTENDED OR BYPASSED BY RESEARCH / EVOLUTION.

11. THE END OF THE DAILY WINDOW IS A CONTROLLED DRAINING + CHECKPOINT EVENT, NOT A HARD KILL.

12. LEARNING, RESEARCH, CANDIDATE GENERATION, VALIDATION, AND RECOVERY ARE ALL AUDITABLE.

13. FAILED / REJECTED / ROLLED-BACK CANDIDATES REMAIN RESEARCH DATA.

14. THE SYSTEM MAY CONCLUDE NO CHANGE / INSUFFICIENT EVIDENCE.

15. THE SYSTEM MAY AUTOMATICALLY RETURN TO A KNOWN-GOOD APPROVED STATE,
    BUT MAY NOT AUTOMATICALLY AUTHORIZE AN UNKNOWN FUTURE STATE.
```

The new Version 2.0 architecture is therefore a **Human-Governed Deterministic Self-Learning and Self-Evolution Research System**, not a self-modifying AI trading bot. The V2 amendment package is contained in sections `V2-01` through `V2-74` plus the V2 appendices.

## 1.2 How to read this master

- Sections **0–40** define the unified design decisions and the new architecture surrounding the original system.
- Section **41** preserves the complete canonical XAUUSD reference from the original master as a technical baseline.
- Section **42** preserves the complete self-evolution research record.
- Later appendices provide implementation contracts, schemas, gates, state machines, and a change-control model.

This preserves the source material while giving the project a single top-level architecture.

---

# 2. MASTER SYSTEM DEFINITION

## 2.1 One-line definition

> **A deterministic, research-first, multi-timeframe XAUUSD shadow system surrounded by a human-governed autonomous research and evolution engine.**

## 2.2 System layers

```text
                      HUMAN
                        │
                GOVERNANCE CORE
                        │
       ┌────────────────┼────────────────┐
       │                │                │
       ▼                ▼                ▼
  POLICY / RISK      EVIDENCE        APPROVAL
       │                │                │
       └────────────────┼────────────────┘
                        │
               ┌────────▼────────┐
               │   CORE SYSTEM   │
               │ XAUUSD Runtime  │
               └────────┬────────┘
                        │
               ┌────────▼────────┐
               │ RESEARCH PLANE  │
               │ Observe         │
               │ Detect          │
               │ Diagnose        │
               │ Hypothesize     │
               │ Experiment      │
               │ Evolve          │
               └────────┬────────┘
                        │
                    SANDBOX
                        │
               VALIDATION FIREWALL
                        │
                   CONTROL/CANDIDATE
                        │
                     HUMAN GATE
                        │
                      SHADOW
                        │
                   PROMOTION GATE
                        │
                   PRODUCTION
                        │
                   MONITORING
                        │
               INCIDENT / OUTCOME
                        │
                POSTMORTEM / MEMORY
                        │
                     RESEARCH
```

## 2.3 Fundamental asymmetry

The project intentionally gives more freedom to **research** than to **production**.

```text
Research Plane:
HIGH EXPLORATION
LOW AUTHORITY

Control Plane:
LOW EXPLORATION
HIGH AUTHORITY

Production Plane:
LOW MUTABILITY
STRICT AUTHORITY
```

---

# 3. FOUNDATIONAL INVARIANTS

The following are project-level invariants. They are intended to be enforced by architecture and permissions, not by agent instructions alone.

### INVARIANT 01 — No direct research-to-production mutation

```text
Research Engine MUST NOT have direct Production write access.
```

### INVARIANT 02 — Every change is versioned

Every proposed change must exist as an immutable candidate with an explicit parent.

### INVARIANT 03 — Hypothesis before change

A material evolution should originate from a hypothesis or a documented exploratory campaign.

### INVARIANT 04 — Evaluator separation

A candidate cannot modify the evaluator that determines whether the candidate is successful.

### INVARIANT 05 — Metric immutability within an experiment family

Metric definitions are versioned. Changing the metric starts a new evidence family.

### INVARIANT 06 — Holdout is a trust boundary

Locked holdout data is not ordinary research data.

### INVARIANT 07 — Trial history is evidence

The number and structure of adaptive trials are part of the interpretation of results.

### INVARIANT 08 — Failed experiments are first-class objects

Failure is retained and searchable.

### INVARIANT 09 — No forced evolution

The system may conclude `NO_CHANGE`.

### INVARIANT 10 — Promotion requires explicit authority

Passing automated validation is not equivalent to production authorization.

### INVARIANT 11 — Rollback is bounded

Automatic rollback may return only to a pre-approved known-good state within predefined safety rules.

### INVARIANT 12 — History cannot be rewritten

Research may not rewrite historical evidence; production may not rewrite research history.

### INVARIANT 13 — Audit is append-only

Critical decisions and transitions are logged durably.

### INVARIANT 14 — Human decision is not hidden

Approval/rejection is attributable, timestamped, and linked to the exact candidate and evidence snapshot.

### INVARIANT 15 — Unknown is a valid state

When evidence is insufficient or contradictory, the system must be able to remain undecided.

---

# 4. TWO AXES OF AUTONOMY

The evolution model must never collapse `what may evolve` and `where it may act` into one scalar level.

## 4.1 Axis A — Evolution depth

```text
D0  Observation-only metadata
D1  Parameter evolution
D2  Rule evolution
D3  Feature evolution
D4  Strategy evolution
D5  Research-method evolution
D6  Architecture evolution
D7  Meta-evolution
```

## 4.2 Axis B — Permission scope

```text
P0  Read-only observation
P1  Research sandbox execution
P2  Candidate registry creation
P3  Automated validation
P4  Shadow execution
P5  Human-gated promotion
P6  Production runtime recovery only
```

## 4.3 Rule

> **Depth is not permission.**

A candidate may be structurally sophisticated while still having zero production authority.

---

# 5. EVOLUTION MATURITY MODEL

The project shall mature its autonomy progressively rather than beginning with unrestricted self-modification.

## L0 — OBSERVATION

Reads system state and historical outcomes.

No candidate creation.

## L1 — DIAGNOSIS

Detects anomalies/degradation and constructs failure objects.

No system modification.

## L2 — RESEARCH SANDBOX

Generates hypotheses and experiments in isolation.

No production access.

## L3 — CANDIDATE EVOLUTION

Creates multiple candidate versions inside an explicitly bounded search space.

## L4 — AUTONOMOUS VALIDATION

Runs predefined validation, stress, regression, duplicate checks, and statistical analysis.

It may reject candidates automatically, but may not promote.

## L5 — SHADOW

Candidate runs in parallel without execution authority.

## L6 — HUMAN-GATED PROMOTION

Human explicitly approves a candidate for controlled deployment.

## L7 — GUARDED RUNTIME RECOVERY

The system may automatically return from a bad deployed state to a previously approved known-good version when pre-defined safety invariants are violated.

This is **self-preservation**, not self-evolution.

## L8 — META-EVOLUTION

Research-only experiments can evolve the hypothesis generator, experiment planner, candidate generator, or search policy.

Meta-evolution cannot directly modify production authority.

---

# 6. PRIMARY AUTONOMOUS LOOP

```text
OBSERVE
  ↓
DETECT
  ↓
CLASSIFY
  ↓
DIAGNOSE
  ↓
ROOT-CAUSE HYPOTHESES
  ↓
FALSIFIABLE HYPOTHESIS
  ↓
EXPERIMENT DESIGN
  ↓
EXPERIMENT BUDGET CHECK
  ↓
CANDIDATE GENERATION
  ↓
SANDBOX EXECUTION
  ↓
DATA-LEAKAGE CHECK
  ↓
VALIDATION FIREWALL
  ↓
ROBUSTNESS / STRESS
  ↓
CONTROL vs CANDIDATE
  ↓
REGRESSION
  ↓
STATISTICAL VALIDATION
  ↓
CANDIDATE REVIEW
  ↓
HUMAN GATE
  ↓
SHADOW
  ↓
PROMOTION GATE
  ↓
CONTROLLED DEPLOYMENT
  ↓
MONITORING
  ↓
INCIDENT / OUTCOME
  ↓
ROLLBACK if required
  ↓
POSTMORTEM
  ↓
FAILURE MEMORY
  ↓
LEARN
  ↓
OBSERVE
```

## 6.1 Why `DIAGNOSE` precedes optimization

`Performance ↓` is not a diagnosis.

Potential causes include:

```text
DATA_FAILURE
DISTRIBUTION_SHIFT
FEATURE_FAILURE
REGIME_FAILURE
SIGNAL_FAILURE
EXECUTION_FAILURE
RISK_FAILURE
LATENCY_FAILURE
SYSTEM_FAILURE
```

Therefore:

```text
Failure
   ↓
Root-cause hypothesis
   ↓
Mechanism
   ↓
Experiment
```

must generally precede parameter search.

---

# 7. OBSERVATION ENGINE

## 7.1 Mission

Observe system behavior without changing system state.

## 7.2 Inputs

```text
Market data
Timeframe states
Features
Regimes
Signals
Scores
Confidence
Market quality
Risk state
Shadow outcomes
Execution simulation
System telemetry
Data-quality state
```

## 7.3 Outputs

```text
Observation record
Anomaly
Degradation event
Behavioral deviation
Potential failure
Research priority candidate
```

## 7.4 Required properties

- Point-in-time correctness.
- Stable identifiers.
- Versioned context.
- No silent overwrites.
- Complete timestamps.
- Explainable source fields.

---

# 8. FAILURE DETECTION ENGINE

## 8.1 Failure is a structured object

```text
Failure ID
Detected At
First Seen
Last Seen
Component
Version
Context
Regime
Trigger
Affected Predictions
Affected Outcomes
Severity
Reproducibility
Evidence Strength
Candidate Root Causes
Status
```

## 8.2 Failure classes

```text
DATA_FAILURE
SCHEMA_FAILURE
CLOCK_FAILURE
CONNECTION_FAILURE
FEATURE_FAILURE
REGIME_FAILURE
ELIGIBILITY_FAILURE
SIGNAL_FAILURE
RISK_FAILURE
EXECUTION_FAILURE
RECONCILIATION_FAILURE
PERSISTENCE_FAILURE
MODEL_FAILURE
CONFIG_FAILURE
TELEGRAM_FAILURE
RECOVERY_FAILURE
UNKNOWN_FAILURE
```

## 8.3 Automatic error discovery patterns

Research discovery may search for:

- repeated M15 errors;
- volatility-specific errors;
- spread-specific errors;
- session-specific errors;
- post-event behavior changes;
- H4/M15 conflicts;
- regime transitions;
- time-of-day concentration;
- delayed entries;
- signals that are theoretically correct but have poor simulated execution outcomes.

The discovery itself does **not** authorize a change.

---

# 9. ROOT-CAUSE ANALYSIS ENGINE

## 9.1 Design goal

The RCA engine must output:

```text
ROOT_CAUSE_HYPOTHESIS
+
CONFIDENCE
+
EVIDENCE
+
ALTERNATIVE EXPLANATIONS
```

It should not output `root cause = truth` unless the evidence and the test design justify such a statement.

## 9.2 Candidate causes

```text
Data quality
Feature construction
Time alignment
Regime classification
Strategy eligibility
Signal construction
Execution assumptions
Spread / cost assumptions
Latency
Risk constraints
Broker behavior
Persistence / recovery
```

## 9.3 Alternative hypothesis requirement

For material failures, the engine should attempt at least one competing explanation where feasible.

Example:

```text
H1: M15 momentum degrades in regime transition.
H2: The apparent degradation is actually caused by spread expansion.
H3: The sample is dominated by one unusual event cluster.
```

This reduces the tendency to jump directly from symptom to favored explanation.

---

# 10. HYPOTHESIS ENGINE

## 10.1 Hypothesis schema

```text
Hypothesis ID
Observed Failure
Context / Regime
Root Cause Belief
Mechanism
Prediction
Proposed Change
Expected Direction
Primary Metric
Secondary Metrics
Safety Constraints
Falsification Condition
Experiment Design
Required Data
Budget
Parent Version
```

## 10.2 Required property: falsifiability

A strong hypothesis must specify what evidence would make it fail.

Bad:

```text
Change threshold to 0.65.
```

Better:

```text
Observed:
Performance degradation is concentrated in regime X.

Hypothesis:
The current rule overweights feature X in regime X.

Prediction:
Reducing reliance on X specifically in regime X should reduce
failure concentration without introducing unacceptable regression
in other regimes.

Falsification:
If the candidate does not improve the pre-declared failure slice,
or causes material regression outside it, reject the hypothesis.
```

## 10.3 Hypothesis population

The engine may maintain multiple competing hypotheses rather than one favored explanation.

```text
H1
H2
H3
  ↓
Evidence
  ↓
Experiment allocation
  ↓
Survivors / falsified hypotheses
```

---

# 11. EXPERIMENT ENGINE

## 11.1 Experiment object

```text
Experiment ID
Campaign ID
Parent Version
Hypothesis ID
Failure ID
Question
Dataset Version
Feature Version
Parameter Space
Candidate Generator Version
Evaluator Version
Metric Registry Version
Environment Version
Control Version
Candidate Definition
Trial Count
Random Seed Policy
Budget
Validation Protocol
Result
Decision
Failure Reason
Holdout Exposure
Contamination Status
```

## 11.2 Controlled experiment principle

The default comparison is:

```text
CONTROL = current approved version

CANDIDATE = proposed version
```

Under matched:

```text
Data
Environment
Time window
Execution assumptions
Evaluator
Transaction assumptions
Random seed policy where relevant
```

## 11.3 Experiment outcomes

```text
PROMISING
INCONCLUSIVE
FALSIFIED
REGRESSED
CONTAMINATED
DUPLICATE
BUDGET_REJECTED
POLICY_REJECTED
NO_CHANGE
```

---

# 12. CANDIDATE GENERATOR

## 12.1 Population, not best-only

The research engine should retain a **diverse population** of candidates where feasible.

```text
Parent V1
├── Candidate A
├── Candidate B
├── Candidate C
└── Candidate D
```

A weak branch is not automatically deleted; it may remain informative as negative research evidence.

## 12.2 Evolution mutation levels

### Parameter evolution

Examples:

```text
threshold
window size
filter level
buffer
cooldown
```

### Rule evolution

Changes logical conditions within an explicit schema.

### Feature evolution

Adds/removes/transforms features subject to data-lineage and timestamp checks.

### Strategy evolution

Changes strategy structure under parent-child versioning.

### Research-method evolution

Changes how experiments or validation protocols are constructed; requires stricter governance.

### Architecture evolution

Changes system structure. Research-only in early versions.

### Meta-evolution

Experiments with the machinery that creates hypotheses, experiments, candidates, or search policy. Research-only.

---

# 13. EVOLUTION BUDGET

## 13.1 Budget is not just compute

The system must track at least:

```text
COMPUTE_BUDGET
EXPERIMENT_BUDGET
TRIAL_BUDGET
CANDIDATE_BUDGET
STRUCTURAL_MUTATION_BUDGET
ADAPTIVE_DECISION_BUDGET
HOLDOUT_QUERY_BUDGET
VALIDATION_RUN_BUDGET
BRANCH_BUDGET
```

## 13.2 Why budget is evidence

A candidate discovered after one test and a candidate discovered after 100,000 adaptive tests are not evidentially equivalent, even if they have identical headline performance.

Therefore every campaign records:

```text
search space
number of trials
number of candidates
number of adaptive decisions
validation paths
holdout exposure
```

## 13.3 Budget states

```text
GREEN     within policy
YELLOW    approaching limit
RED       exhausted
FROZEN    no further adaptive research allowed
```

---

# 14. VALIDATION FIREWALL

## 14.1 Purpose

The Validation Firewall is the boundary between:

```text
interesting result
```

and

```text
credible evidence
```

## 14.2 Conceptual pipeline

```text
Candidate
  ↓
Leakage Checks
  ↓
Time-Aware Validation
  ↓
Walk-Forward
  ↓
Purged / CPCV where applicable
  ↓
Statistical Controls
  ↓
PBO / DSR where applicable
  ↓
Stress / Monte Carlo
  ↓
Regression
  ↓
Locked OOS / Holdout policy
  ↓
Human Review
```

Not every candidate uses every method. The **Validation Planner** chooses the protocol before the candidate is evaluated, rather than letting the candidate choose the most favorable test afterward.

## 14.3 Multiple-testing protection

Relevant research tools include:

```text
Probability of Backtest Overfitting
Deflated Sharpe Ratio
White Reality Check
Superior Predictive Ability
CPCV / Purged validation
Walk-forward validation
Monte Carlo / stress testing
```

These are evidence tools, not guarantees.

---

# 15. EVIDENCE FIREWALL

## 15.1 Evidence zones

```text
E0 — Exploration
E1 — Development / Validation
E2 — Out-of-Sample
E3 — Locked Holdout
```

## 15.2 Holdout service model

The preferred protected design is:

```text
RESEARCH ENGINE
    │
    │ Candidate Hash
    │ Experiment Definition
    │ Artifact Hash
    │ Evaluator Version
    ▼
HOLDOUT SERVICE
    │
    ├─ no raw holdout access
    ├─ immutable query audit
    ├─ predefined outputs
    └─ query budget
```

## 15.3 Contamination state machine

```text
CLEAN
  ↓
VALIDATED
  ↓
OOS_EXPOSED
  ↓
HOLDOUT_EXPOSED
  ↓
CONTAMINATED
  ↓
RETIRED
```

Once a candidate has seen locked-holdout evidence, it should not silently return to the same optimization loop using that evidence.

---

# 16. EVALUATOR FIREWALL

The evaluator belongs to the trusted control plane.

Candidate code may not modify:

```text
Evaluator
Metric Definitions
Validation Code
Holdout Service
Audit Logger
Budget Policy
Rollback Policy
Approval Policy
```

Any intentional evaluator change creates a new evaluator version and a new evidence family.

---

# 17. REWARD-HACKING DEFENSE

The system must assume that an optimizer can exploit a metric or evaluator.

Therefore candidate success cannot be reduced to one mutable scalar.

Use:

```text
HARD SAFETY CONSTRAINTS
       +
STATISTICAL VALIDITY
       +
ROBUSTNESS
       +
QUALITY OBJECTIVES
```

Rule:

> **Constraints are vetoes. Metrics are optimizers.**

The candidate may optimize within the approved metric family, but it cannot redefine the game it is being judged by.

---

# 18. CANDIDATE LIFECYCLE

```text
PROPOSED
  ↓
SCHEMA_VALIDATED
  ↓
SANDBOX_READY
  ↓
EXPERIMENTAL
  ↓
VALIDATING
  ↓
PASSED / FAILED / INCONCLUSIVE
  ↓
REVIEW_READY
  ↓
HUMAN_APPROVED / HUMAN_REJECTED / MORE_RESEARCH
  ↓
SHADOW_READY
  ↓
SHADOW_ACTIVE
  ↓
PROMOTION_READY
  ↓
PROMOTED / ABORTED
  ↓
MONITORED
  ↓
STABLE / ROLLED_BACK / RETIRED
```

No hidden transitions.

---

# 19. CANDIDATE SCHEMA

```text
Candidate ID
Parent Version
Parent Hash
Campaign ID
Hypothesis ID
Failure ID
RCA ID
Reason
Change Type
Changed Components
Dataset Version(s)
Feature Version(s)
Parameter Set
Code Commit
Environment Image
Dependency Lock
Experiment Definition
Search Space
Trial Number
Random Seed Policy
Evaluator Version
Metric Registry Version
Control Version
Validation Results
OOS Results
Stress Results
Regression Results
PBO / DSR / Statistical Results where applicable
Risk Analysis
Known Limitations
Unexpected Effects
Contamination Status
Human Decision
Human Reviewer
Approval Timestamp
Deployment Status
Shadow Status
Promotion Status
Rollback Target
Rollback Procedure
Provenance / Artifact Digest
```

---

# 20. EXPERIMENT LEDGER

## 20.1 Append-only record

The ledger must store failures and rejected branches, not only wins.

```text
Experiment ID
Timestamp
Actor
Campaign
Parent Version
Hypothesis
RCA
Dataset
Feature Set
Parameters
Search Space
Number of Trials
Control
Candidate
Metrics
Validation
Stress
Decision
Failure Reason
Rejection Reason
Unexpected Effects
Evaluator Version
Code Version
Environment Version
Seed
Holdout Exposure
Contamination Status
```

## 20.2 Experiment fingerprint

```text
Fingerprint = Hash(
    Parent
    + Hypothesis
    + Dataset
    + Features
    + Search Space
    + Code
    + Evaluator
    + Environment
)
```

Exact duplicates should be blocked.

Near duplicates should trigger a research warning with references to previous experiments.

---

# 21. EVOLUTION MEMORY

The system should maintain several memory classes instead of one “best strategy” table.

```text
POSITIVE MEMORY
NEGATIVE MEMORY
CAUSAL MEMORY
SEARCH MEMORY
EVIDENCE MEMORY
CONTAMINATION MEMORY
HUMAN DECISION MEMORY
INCIDENT MEMORY
```

The core question stored for each item is:

> **What happened, why did it happen, what was tested, what did the evidence support, under which context, and what should be avoided or revisited later?**

---

# 22. FAILURE MEMORY

```text
FailurePattern
{
    type,
    context,
    trigger,
    evidence,
    root_cause_hypotheses,
    confirmed_root_cause,
    false_hypotheses,
    affected_versions,
    rejected_changes,
    recurrence_count,
    prevention_actions
}
```

A rollback must generate a structured incident, not simply restore an old binary.

```text
Candidate Failed
    ↓
Rollback
    ↓
Incident
    ↓
RCA
    ↓
Failure Pattern
    ↓
Knowledge Update
    ↓
Future Experiment Filter
```

---

# 23. DO-NOTHING ENGINE

This is a required component.

The engine must be allowed to conclude:

```text
Finding detected
    ↓
Investigated
    ↓
Evidence insufficient
    ↓
NO CHANGE
```

or:

```text
Candidate improvement
    ↓
Not robust across regimes
    ↓
REJECT
```

The number of prevented changes should be treated as a healthy output of the research process, not as a failure of the system.

---

# 24. AUTOMATIC RESEARCH PRIORITY

If many problems exist, prioritize investigation using transparent criteria such as:

```text
Affected prediction count
Evidence strength
Reproducibility
Potential system impact
Regime coverage
Data quality
Validation confidence
Novelty
Recurrence
```

The output is:

> “This problem should be investigated first under the declared research policy.”

It is **not**:

> “This is the best strategy.”

---

# 25. HUMAN GOVERNANCE MODEL

The governance model combines three roles:

## Human-in-the-Command

Human owns:

```text
Goal
Policy
Risk constraints
Authority
Boundaries
```

## Human-on-the-Loop

System runs inside boundaries while human monitors and can intervene.

## Human-in-the-Loop

Human must explicitly participate for high-impact decisions.

The project should use all three according to the operation rather than choosing one label for the whole system.

---

# 26. HUMAN APPROVAL CONTRACT

## 26.1 Approval is an evidence review

The application must present:

```text
Proposal ID
Current Version
Candidate Version
Problem
First Seen
Detection Method
Affected Cases
Evidence Strength
Root-Cause Hypotheses
Hypothesis
Proposed Change
Expected Effects
Observed Effects
Regression Analysis
Regime Analysis
Stress Results
Validation Results
Contamination Status
Known Risks
Unknowns
Rollback Target
```

## 26.2 Human decisions

```text
APPROVE
REJECT
REQUEST_MORE_RESEARCH
MODIFY_PROPOSAL
FREEZE
```

## 26.3 Human decision record

```text
Proposal ID
Decision
Reason
Evidence Snapshot
Reviewer
Timestamp
Candidate Hash
Policy Version
```

## 26.4 Human investigation workflow

The intended human role is not to click “approve” blindly.

The system should point the human to the exact cases that produced the finding so the human can inspect the market context, event context, data quality, and assumptions.

---

# 27. CHANGE INVESTIGATION REPORT

A change proposal should be rendered in a report resembling:

```text
CHANGE PROPOSAL
────────────────────────────
Proposal ID:
Detected At:
First Evidence:
Affected Component:
Current Version:
Candidate Version:

1. WHAT WAS DETECTED?
2. WHEN DID IT FIRST APPEAR?
3. HOW WAS IT DETECTED?
4. HOW WAS IT VERIFIED?
5. WHAT IS CURRENTLY BELIEVED TO BE WRONG?
6. WHAT WILL CHANGE?
7. WHY THIS CHANGE?
8. EXPECTED EFFECT
9. NEGATIVE EFFECTS / RISKS
10. HISTORICAL COMPARISON
11. VALIDATION RESULTS
12. AFFECTED PERIODS / REGIMES
13. DATA / PREDICTION / REPLAY / EXPERIMENT IDS
14. FINAL SYSTEM RECOMMENDATION
15. HUMAN DECISION
```

The recommendation is a research finding, not an automatic production command.

---

# 28. APPLICATION AS PRIMARY CONTROL CENTER

The desktop application is the primary human interface and control center.

## 28.1 Application domains

```text
Market Dashboard
Prediction Center
Research Center
Auto-Improvement Center
Change Proposal Center
Version Registry
Evolution Graph
Evidence Explorer
Shadow Center
Monitoring Center
Incident Center
Audit Center
System Configuration
```

## 28.2 Core principle

```text
Application = Primary authority interface
Core         = System brain
Research     = Self-learning engine
Database     = Memory
Telegram     = Optional assistants
Human        = Final authority for production changes
```

## 28.3 Application must survive Telegram outage

```text
Telegram DOWN
     ↓
Application continues
Research continues
Predictions continue
Database continues
Core continues
```

Telegram is an accessory, not a dependency for core correctness.

---

# 29. TELEGRAM ASSISTANTS

## Bot 1 — Market / Operations Assistant

Typical responsibilities:

```text
Status
Market snapshot
Predictions
Results
Statistics
Errors
Research alerts
Reports
```

## Bot 2 — Research / Governance Assistant

Typical responsibilities:

```text
New finding alert
Change proposal summary
Candidate status
Validation summary
Research report notification
Approval-needed notification
Rollback notification
```

## Telegram authority rule

Telegram does not outrank the application or the control plane.

If an approval command is ever exposed through Telegram, it must pass through:

```text
Telegram request
    ↓
Approval Service
    ↓
Verify Proposal ID
    ↓
Verify User Authorization
    ↓
Verify Candidate Hash / Version
    ↓
Verify Current State
    ↓
Record Approval
    ↓
Promotion Manager
```

A Telegram failure must not cause an arbitrary production mutation.

---

# 30. SHADOW / CHALLENGER MODEL

## 30.1 Shadow definition

A candidate sees the same relevant inputs as the control but has no execution authority.

```text
CURRENT VERSION ───────┐
                        ├── Parallel evaluation
CANDIDATE VERSION ──────┘
```

## 30.2 Shadow is not canary

```text
SHADOW:
Candidate has no real execution authority.

CANARY:
Candidate receives controlled production exposure.
```

The preferred progression is:

```text
Research
  ↓
Validation
  ↓
Human Approval
  ↓
Shadow
  ↓
Restricted Canary where separately justified
  ↓
Human/Policy Promotion Decision
```

## 30.3 Shadow comparisons

Measure:

```text
Signals
Signal disagreements
Regime behavior
Risk proposals
Expected outcomes
Execution assumptions
Costs / spread assumptions
Tail behavior
Robustness
Unexpected effects
```

---

# 31. PROMOTION GATE

A candidate is promotion-ready only when the promotion package satisfies all required gates.

```text
Human Approval = YES
Evidence Complete = YES
Candidate Hash Verified = YES
Evaluator Verified = YES
Metric Registry Verified = YES
No Forbidden State = YES
Rollback Target Ready = YES
Monitoring Ready = YES
Shadow Requirement Satisfied = YES
Policy Version Compatible = YES
```

Promotion is a separate control-plane action; passing research tests does not automatically mutate production.

---

# 32. ROLLBACK & SELF-PRESERVATION

## 32.1 Automatic rollback boundaries

Rollback can happen automatically only when:

- the invariant is pre-defined;
- the rollback target is known-good;
- the target is already approved;
- the rollback itself is logged;
- the operation is reversible where appropriate.

## 32.2 Rollback loop

```text
Production Version
      ↓
Invariant Violation
      ↓
Emergency Protection
      ↓
Rollback to Known-Good
      ↓
Incident Created
      ↓
Human Notified
      ↓
Postmortem
```

## 32.3 Formal principle

> **The system may automatically return to a known-good state, but it may not automatically authorize an unknown future state.**

---

# 33. EVOLUTION GRAPH & EVIDENCE GRAPH

## 33.1 Evolution Graph

```text
                    V1.0
                  /  |  \
               V1.1A V1.1B V1.1C
                 |     |      |
                 D     E      F
```

Each child has one explicit parent, but a project may have many research branches.

## 33.2 Evidence Graph

```text
Version
  ↓
Hypothesis
  ↓
Experiment
  ↓
Result
  ↓
Validation
  ↓
Decision
  ↓
Version
```

Example:

```text
Version B
  ← Hypothesis H19
  ← Experiment E44
  ← Dataset D7
  ← Evaluator EV17
  ← OOS O3
  ← Human Decision D12
```

## 33.3 Why two graphs

Evolution answers:

> Where did this version come from?

Evidence answers:

> Why was this transition justified?

Together they create reproducible project history.

---

# 34. PROVENANCE & TRUSTED CONTROL PLANE

## 34.1 Trusted control plane components

```text
Policy Engine
Approval Gate
Metric Registry
Evaluator Registry
Version Registry
Audit Ledger
Evidence Firewall
Budget Manager
Promotion Manager
Rollback Manager
```

## 34.2 Research plane components

```text
Observation
Detection
RCA
Hypothesis
Experiment Planning
Candidate Generation
Evolution Search
Sandbox Execution
Research Analysis
```

## 34.3 Runtime plane components

```text
Production Runtime
Shadow Engine
Monitoring
Incident Detection
Rollback
```

The planes exchange signed/versioned artifacts and events rather than arbitrary mutable state.

---

# 35. RESEARCH NOTEBOOK / ENGINEERING MEMORY

Every material design decision gets:

```text
DECISION_ID
QUESTION
OPTIONS
EVIDENCE
ASSUMPTIONS
SELECTED_DESIGN
REJECTED_DESIGNS
RISKS
TEST_REQUIRED
STATUS
```

This is part of the project memory and should not be replaced by informal chat notes.

---

# 36. DATABASE DOMAINS FOR THE UNIFIED SYSTEM

The original architecture already defines the core database domains. The evolution layer adds specialized domains.

## 36.1 Core domains

```text
Market Data
Bars
Ticks
Timeframe State
Features
Regimes
Strategies
Signals
Scores
Confidence
Events
Macro State
Market Quality
Risk
Shadow Orders
Shadow Positions
Outcomes
Audit
System Health
```

## 36.2 Evolution domains

```text
Failures
RCA Cases
Hypotheses
Experiments
Candidates
Candidate Artifacts
Validation Runs
Metric Versions
Evaluator Versions
Holdout Queries
Contamination States
Evolution Budget
Human Decisions
Incidents
Postmortems
Failure Memory
Evolution Graph Edges
Evidence Graph Edges
Promotion Packages
Rollback Records
```

## 36.3 Suggested identity relations

```text
Version
 ├── parent_version
 ├── code_artifact
 ├── dependency_lock
 ├── dataset_version
 ├── feature_version
 ├── evaluator_version
 └── metric_registry_version

Hypothesis
 └── failure_id

Experiment
 ├── hypothesis_id
 ├── control_version
 └── candidate_version(s)

Decision
 ├── experiment_id
 ├── candidate_id
 └── evidence_snapshot

Deployment
 ├── candidate_id
 └── rollback_target
```

---

# 37. CANDIDATE / EVIDENCE STATE MACHINES

## 37.1 Candidate state machine

```text
DRAFT
  ↓
VALIDATED_SCHEMA
  ↓
SANDBOX
  ↓
VALIDATING
  ├── FAILED
  ├── INCONCLUSIVE
  └── PASSED
          ↓
      HUMAN_REVIEW
       ├─ REJECTED
       ├─ MORE_RESEARCH
       └─ APPROVED
              ↓
           SHADOW
              ↓
         PROMOTION_GATE
          ├─ ABORT
          └─ PROMOTE
              ↓
          MONITOR
          ├─ STABLE
          └─ ROLLBACK
```

## 37.2 Evidence contamination state

```text
CLEAN
  ↓
DEV_VALIDATED
  ↓
OOS_EXPOSED
  ↓
HOLDOUT_EXPOSED
  ↓
CONTAMINATED
  ↓
RETIRED / NEW_EVIDENCE_FAMILY
```

---

# 38. VALIDATION POLICY BY CHANGE TYPE

The validation protocol must be selected **before** the candidate runs.

| Change type | Minimum conceptual controls | Additional controls |
|---|---|---|
| Parameter | time-aware validation, OOS, sensitivity | multiple-testing correction |
| Rule | regression, regime slices, OOS | logical/static checks |
| Feature | point-in-time checks, lineage, leakage tests | ablation / availability tests |
| Strategy | full candidate protocol | robust multi-regime analysis |
| Risk rule | invariant suite, worst-case tests | human approval required |
| Execution model | broker/market-quality validation | dedicated simulation tests |
| Research method | evidence-family separation | stricter human governance |
| Evaluator | trusted-plane review | new evaluator version + new evidence family |
| Architecture | sandbox only initially | structural review + integration tests |
| Meta-evolution | research only | dedicated governance policy |

This table is a planning policy, not a claim that one exact test suite is universally sufficient.

---

# 39. HARD-FORBIDDEN BEHAVIORS

The following are intended to be rejected at the architecture/security boundary:

```text
01  Self-modifying production code
02  Direct production writes from the research engine
03  Candidate modification of evaluator
04  Candidate modification of metric definitions during evaluation
05  Deletion of failed experiments
06  Editing historical results
07  Raw access to locked holdout
08  Re-optimization on holdout results in the same evidence cycle
09  Unlimited experimentation
10  Future-information access
11  Timestamp manipulation
12  Hidden external data access
13  Unauthorized data-source changes
14  Modification of risk constraints by candidate code
15  Bypassing approval gates
16  Hidden side channels into evaluator
17  Optimizing the evaluator instead of the declared objective
18  Suppression of failure signals
19  Self-promotion of an unapproved successor
20  Self-creation of credentials or permissions
21  Modification of the audit logger by candidate code
22  Alteration of rollback target by candidate code
23  Candidate evaluating itself through a mutable evaluator
24  Circular metric changes that declare the candidate successful
```

---

# 40. RESEARCH GAPS & FUTURE QUESTIONS

The integrated research identified areas where no simple mature blueprint should be assumed.

## Gap A — Adaptive search over evolving financial data

How should continuously adaptive search be statistically accounted for when the search space itself evolves and data are dependent and non-stationary?

## Gap B — Self-evolving validation

How can a system research new validation methods without allowing the evaluator to become an optimization target?

## Gap C — RCA + evolution + statistical validity

How can causal/root-cause reasoning be tightly coupled to candidate generation without turning RCA confidence into unjustified certainty?

## Gap D — Human review compression

How can one human review thousands of research candidates without reducing the process to a simplistic score?

## Gap E — Failure learning without evidence leakage

How can the system learn from failed experiments without contaminating future evaluation protocols?

## Gap F — Meta-evolution safety

How far can the research system evolve its own search machinery while keeping the evaluator and evidence boundary trustworthy?

## Gap G — Adaptive holdout

Could privacy-preserving adaptive holdout techniques eventually support more flexible research without compromising evidence integrity? This is a future research track, not a dependency for the first implementation.

---

# 41. CANONICAL XAUUSD SYSTEM REFERENCE — PRESERVED BASELINE

The following section preserves the original master architecture exactly as the primary technical baseline for the XAUUSD system. The unified evolution architecture above is designed to wrap around it rather than silently alter it.

# XAUUSD SOVEREIGN MULTI-TIMEFRAME SHADOW RESEARCH SYSTEM
## Canonical Master Reference, Architecture, Operating Model, Research Framework, Safety Model, Telegram Intelligence Layer, and Implementation Blueprint

**Document type:** Canonical project reference / system-thinking document  
**Asset:** XAUUSD only  
**Primary operating mode:** SHADOW  
**Primary decision architecture:** Multi-timeframe, event-driven, deterministic, broker-aware, research-first  
**Runtime core:** Local C++ orchestration and decision core  
**Market adapters:** 9 MT5/MQL5 timeframe adapters  
**Research layer:** Python-compatible research/replay/analysis environment  
**Human interface:** Telegram intelligence and alert interface  
**Execution objective:** Shadow simulation first; no blind automated trading  
**Production status:** Architecture and research specification only; profitability is unproven  
**Normative language:** MUST = hard invariant; SHOULD = strong recommendation; MAY = optional; RESEARCH_HYPOTHESIS = unproven; BROKER_DEPENDENT = must be measured on the actual broker feed.

---

# 0. PURPOSE OF THIS DOCUMENT

This document is the single canonical reference for the XAUUSD project.

It replaces the need to reason from nine separate timeframe documents during architecture design. The nine source documents are treated as the technical foundation from which this master reference is consolidated. The purpose is not to blindly concatenate them. The purpose is to create one coherent system model that:

1. preserves the useful mathematical, operational, risk, execution, research, and safety concepts;
2. removes contradictory authority between timeframes;
3. separates structural information from execution information;
4. defines a deterministic multi-timeframe data contract;
5. defines SHADOW mode as a complete simulated execution lifecycle rather than a simple signal display;
6. defines Telegram as an observation and decision-intelligence interface, not as the trading brain;
7. prevents false claims such as treating a heuristic score as a real probability;
8. preserves point-in-time and no-lookahead principles;
9. makes broker-specific XAUUSD behavior explicit;
10. provides implementation contracts for C++, MT5 adapters, persistence, research, replay, and monitoring;
11. creates a durable place for future experiments, decisions, failures, and architecture changes.

The system is not designed around the statement:

```text
INDICATOR -> BUY/SELL
```

It is designed around:

```text
MARKET DATA
    ->
DATA INGESTION
    ->
DATA VALIDATION
    ->
TIMEFRAME STATE
    ->
FEATURES
    ->
STRUCTURE
    ->
REGIME
    ->
STRATEGY ELIGIBILITY
    ->
SIGNAL
    ->
SIGNAL VALIDATION
    ->
SCORE / CONFIDENCE
    ->
MACRO / EVENT / MARKET-QUALITY GATES
    ->
RISK PROPOSAL
    ->
SHADOW EXECUTION
    ->
SIMULATED POSITION LIFECYCLE
    ->
OUTCOME
    ->
AUDIT / RESEARCH LEDGER
    ->
TELEGRAM INTELLIGENCE
```

The central philosophy is:

> **The system is a deterministic state machine surrounding research hypotheses. It is not a collection of indicators surrounding an order function.**

---

# 1. PROJECT MISSION

The project is an XAUUSD-only research and decision-support system.

The first operational objective is not to maximize trading frequency and not to claim a high win rate. The first objective is to build a trustworthy experimental machine capable of answering:

```text
What did the system know?
When did it know it?
What did it infer?
Why did it infer it?
What constraints were active?
What would it have done?
What would the simulated execution have looked like?
What actually happened afterward?
Was the original hypothesis supported?
```

The system must therefore optimize for:

```text
DATA INTEGRITY
REPRODUCIBILITY
CAUSAL TIMING
DETERMINISM
SAFETY
AUDITABILITY
EXPERIMENTAL QUALITY
BROKER REALISM
FAILURE RECOVERY
```

Profitability is not an architectural assumption.

A backtest, a high score, a high historical hit rate, or a visually convincing chart is not sufficient evidence for production promotion.

---

# 2. CANONICAL SYSTEM BOUNDARIES

## 2.1 Asset boundary

The project is restricted to:

```text
XAUUSD
```

The system trades the broker's representation of XAUUSD.

Therefore:

```text
BROKER XAUUSD != UNIVERSAL GOLD PRICE
```

Broker-specific differences may include:

```text
symbol name
digits
point
tick size
tick value
contract size
volume minimum
volume maximum
volume step
volume limit
stops level
freeze level
margin rules
swap rules
sessions
spread
execution mode
filling mode
price history
tick history
server timezone
bar boundaries
```

These values MUST be observed dynamically where possible.

---

## 2.2 Operational boundary

The canonical deployment is:

```text
9 MT5/MQL5 ADAPTERS
        |
        v
LOCAL C++ DATA/IPC LAYER
        |
        v
C++ CORE
        |
        +--> PERSISTENT STATE / LEDGER
        |
        +--> SHADOW EXECUTION ENGINE
        |
        +--> TELEGRAM INTERFACE
        |
        +--> RESEARCH / REPLAY DATA
```

The MT5 adapters are not the research brain.

The C++ core is not allowed to assume that a broker adapter is always healthy.

The Telegram bot is not allowed to become the source of truth.

The persistent ledger is the historical record of system decisions and simulated outcomes.

---

# 3. THE NINE TIMEFRAME ADAPTERS

The project uses nine timeframe data streams:

```text
M1
M5
M15
M30
H1
H4
D1
W1
MN1
```

Each adapter has the same fundamental responsibilities:

```text
CONNECT
IDENTIFY
CAPTURE
VALIDATE
TIMESTAMP
SEQUENCE
HEARTBEAT
REPORT HEALTH
PUBLISH CLOSED BARS
PUBLISH REAL-TIME QUOTES/TICKS WHEN AVAILABLE
PUBLISH SYMBOL SPECIFICATION
RECOVER AFTER DISCONNECT
```

An adapter is a data/execution boundary, not an independent strategy.

A timeframe document may define specialized features, but the canonical system must prevent nine independent brains from generating nine conflicting orders.

---

# 4. TIMEFRAME AUTHORITY MODEL

The system MUST NOT treat all nine timeframes as equal directional authorities.

The canonical authority model is:

```text
MN1
    |
    +-- secular / macro context

W1
    |
    +-- long-horizon structural context

D1
    |
    +-- multi-day structural context

H4
    |
    +-- primary structural/regime authority

H1
    |
    +-- intermediate structural confirmation

M30
    |
    +-- local structural context

M15
    |
    +-- primary operational setup / decision trigger

M5
    |
    +-- local execution context

M1
    |
    +-- microstructure / execution context

TICK
    |
    +-- real-time broker/execution state
```

This hierarchy does NOT mean that every trade must have perfect directional agreement across all nine levels.

It means each timeframe has a defined role.

The system must distinguish:

```text
CONTEXT
from
DIRECTIONAL AUTHORITY
from
SETUP TRIGGER
from
EXECUTION STATE
```

---

# 5. H4 + M15 DUAL ENGINE

The central operating concept is:

```text
H4 = STRUCTURAL COMPASS
M15 = PRIMARY OPERATIONAL TRIGGER
```

H4 determines the structural market environment.

M15 determines whether an actionable setup exists inside that environment.

Therefore:

```text
H4:
    What type of market are we in?

M15:
    Is there a valid setup now?

M5/M1:
    Is the current execution environment acceptable?

TICK:
    Is the broker/execution state currently healthy?
```

A lower timeframe MUST NOT silently reverse a higher-timeframe structural thesis.

Example:

```text
H4 = TREND_UP
M15 = PULLBACK
M5 = SHORT-TERM_DOWN
```

does not automatically mean:

```text
GLOBAL DIRECTION = SELL
```

It means:

```text
H4 trend remains UP
M15 may be in a pullback
M5 provides local execution information
```

The architecture must preserve these states separately.

---

# 6. CLOSED-BAR PRINCIPLE

Every structural timeframe has two fundamental states:

```text
index 0 = current / forming
index 1 = latest completed
index 2 = previous completed
...
```

Structural decisions MUST use completed bars.

Examples:

```text
H4 shift 1 -> latest completed H4
M15 shift 1 -> latest completed M15
D1 shift 1 -> latest completed D1
W1 shift 1 -> latest completed W1
MN1 shift 1 -> latest completed MN1
```

The forming candle may be observed for execution state, but it must not retroactively rewrite a completed structural decision.

Hard invariant:

```text
NO INCOMPLETE STRUCTURAL BAR
MAY CREATE OR REPAINT A COMPLETED DECISION
```

---

# 7. DECISION CLOCKS

The system has multiple clocks and must never conflate them.

## 7.1 Structural clock

Driven by completed bars:

```text
MN1 CLOSE
W1 CLOSE
D1 CLOSE
H4 CLOSE
H1 CLOSE
M30 CLOSE
M15 CLOSE
M5 CLOSE
M1 CLOSE
```

## 7.2 Execution clock

Driven by:

```text
ticks
bid/ask
spread
quote freshness
broker session
latency
execution events
```

## 7.3 Safety clock

Driven by:

```text
connection events
health events
account state
position state
reconciliation state
risk state
watchdog state
```

## 7.4 Research clock

Driven by:

```text
event_time
publication_time
knowledge_time
collector_time
decision_time
outcome horizon
```

The system must never use a future research observation merely because it exists in the database.

---

# 8. EVENT-DRIVEN CORE

The C++ core is event-driven.

Canonical event categories:

```text
TICK
BAR_CLOSED
BAR_UPDATED
SYMBOL_SPEC_CHANGED
CONNECTION_CHANGED
HEARTBEAT
NEWS_EVENT
MACRO_EVENT
ACCOUNT_CHANGED
ORDER_EVENT
DEAL_EVENT
POSITION_EVENT
SYSTEM_ERROR
DATA_QUALITY_EVENT
TIMER
RECOVERY_REQUEST
```

Every event must contain enough metadata to reconstruct its causal place in the system.

Minimum:

```text
event_id
event_type
symbol
source
source_instance
event_time
receive_time
sequence_id
schema_version
protocol_version
payload
quality_state
```

---

# 9. DATA BUS

The local C++ server is the central transport boundary.

It should support logical message classes:

```text
COMMAND
QUERY
EVENT
RESPONSE
ERROR
HEARTBEAT
```

Every message SHOULD contain:

```text
protocol_version
schema_version
message_id
timestamp
source
destination
payload
checksum/hash where appropriate
```

The receiver MUST reject or quarantine:

```text
unknown protocol version
invalid schema
duplicate command
expired command
future-dated message
clock anomaly
invalid payload
unknown source
```

---

# 10. FAULT TOLERANCE

The system must not equate:

```text
BOT OFFLINE
```

with:

```text
SYSTEM CRASH
```

Instead:

```text
ADAPTER FAILURE
    ->
DATA QUALITY / CAPABILITY ENGINE
    ->
DEGRADE ONLY THE DEPENDENT CAPABILITIES
```

Example:

```text
M1 OFFLINE
    -> M1 microstructure unavailable
    -> M1-dependent features disabled
    -> M15/H4 structural state may continue

M5 OFFLINE
    -> local execution-context features unavailable
    -> execution confidence may be reduced

M15 OFFLINE
    -> M15 primary setup engine unavailable
    -> M15-triggered decisions blocked

H4 OFFLINE
    -> H4 structural authority unavailable
    -> H4-dependent strategies blocked

TICK OFFLINE
    -> quote freshness / execution state unknown
    -> new execution proposals blocked
```

The correct response is capability degradation, not blind continuation.

---

# 11. DATA QUALITY STATES

Every data source should have one of:

```text
VALID
DEGRADED
INVALID
UNKNOWN
STALE
MISSING
OUT_OF_ORDER
DUPLICATE
INCOMPLETE
```

These states have explicit policy consequences.

For example:

```text
VALID
    -> feature may be used

DEGRADED
    -> only features explicitly allowed under degraded policy

INVALID
    -> dependent decisions blocked

UNKNOWN
    -> must not be interpreted as SAFE or NEUTRAL

STALE
    -> execution-dependent operations blocked

MISSING
    -> dependent feature unavailable
```

Unknown is not neutral.

Unknown is uncertainty.

---

# 12. DATA MODEL

## 12.1 Tick

```text
Tick {
    event_id
    symbol
    source
    source_timestamp
    receive_timestamp
    sequence_id
    bid
    ask
    last
    volume
    flags
    quality_state
}
```

## 12.2 Bar

```text
Bar {
    bar_id
    symbol
    timeframe
    open_time
    close_time
    open
    high
    low
    close
    tick_volume
    real_volume_if_available
    source
    broker_id
    completeness_state
    ingestion_time
}
```

## 12.3 Symbol specification

```text
SymbolSpec {
    broker
    server
    symbol
    digits
    point
    tick_size
    tick_value
    contract_size
    volume_min
    volume_max
    volume_step
    volume_limit
    stops_level
    freeze_level
    trade_mode
    execution_mode
    filling_mode
    margin_mode
    sessions
    swap_rules
}
```

## 12.4 Decision

```text
Decision {
    decision_id
    symbol
    trigger_time
    trigger_timeframe
    closed_bar_id

    data_state
    structural_state
    regime_state
    eligibility_state

    signal
    score
    confidence
    probability

    macro_state
    event_state
    market_quality

    risk_proposal
    execution_proposal

    system_mode
    strategy_version
    configuration_version
}
```

---

# 13. DATA VALIDATION FIREWALL

Before a structural feature is calculated, validate:

```text
symbol identity
timestamp monotonicity
duplicate bars
OHLC validity
bar completeness
expected interval
known market closures
history sufficiency
broker identity
data source
```

OHLC invariants:

```text
HIGH >= max(OPEN, CLOSE)
LOW  <= min(OPEN, CLOSE)
HIGH >= LOW
```

Invalid data must not be silently repaired.

If repair is performed for research, the original and repaired records must both be retained and the repair policy must be explicit.

---

# 14. TICK QUALITY AND MICROSTRUCTURE

The system must never assume:

```text
one OnTick callback = one source tick
```

Where high-fidelity tick analysis matters, use the available tick history/reconciliation mechanisms and retain:

```text
source timestamp
receive timestamp
sequence
bid
ask
last
volume
flags
```

Important derived measures may include:

```text
spread
spread/ATR
tick arrival rate
quote freshness
price jump
short-horizon realized volatility
directional efficiency
microstructure imbalance where available
```

Do not label tick activity as exchange liquidity.

Do not equate broker DOM with global gold liquidity.

Do not equate tick volume with exchange-traded volume.

---

# 15. FEATURE ENGINE

The feature engine is divided into layers.

## Layer A — Measurement

```text
returns
TR
ATR
standard deviation
realized volatility
range
spread
tick activity
price displacement
```

## Layer B — Normalization

```text
NATR
Z-score
percentile
ATR ratios
spread/ATR
relative activity
volatility percentile
```

## Layer C — Classification

```text
ADX
DI
EMA structure
EMA slope
linear regression slope
R²
efficiency ratio
compression
expansion
trend state
range state
transition state
post-shock state
```

## Layer D — Structure

```text
swing highs
swing lows
channels
support/resistance
breakout boundaries
pullback structure
structural displacement
range boundaries
```

## Layer E — Decision features

```text
trend continuation
breakout
mean reversion
momentum
pullback
structure confirmation
signal confluence
```

No feature is automatically an alpha signal.

Every feature is a measurable input to a hypothesis.

---

# 16. MARKET REGIME ENGINE

The regime engine answers:

> What type of market is this?

It does NOT answer:

> Should I place an order?

Canonical regime vocabulary may include:

```text
TREND_UP
TREND_DOWN
RANGE
COMPRESSION
EXPANSION_UP
EXPANSION_DOWN
TRANSITION
POST_SHOCK
UNKNOWN
DATA_INSUFFICIENT
```

Regime classification should use multiple observations rather than one candle whenever practical.

Persistence and hysteresis should be used where necessary to prevent unstable state flipping.

Example:

```text
candidate regime
    ->
persistence test
    ->
confirmed regime
```

The exact thresholds remain empirical research parameters.

---

# 17. STRATEGY ELIGIBILITY ENGINE

The eligibility engine answers:

> Which strategy families are permitted to search for a setup?

Examples:

## Trend following

Allowed when:

```text
valid trend
directional structure
sufficient trend quality
acceptable volatility
acceptable market quality
no event lock
```

Blocked by:

```text
range
transition
post-shock
unknown
```

## Mean reversion

Allowed when:

```text
stable range
range boundary
controlled volatility
no event lock
acceptable liquidity
```

Blocked by:

```text
strong trend
expansion
post-shock
transition
unknown
```

## Breakout

Allowed when:

```text
compression
valid range
confirmed boundary
boundary proximity
expansion initiation
acceptable execution environment
```

Blocked by:

```text
unknown
transition
poor liquidity
abnormal spread
unresolved shock
```

The system MUST keep:

```text
REGIME
ELIGIBILITY
SIGNAL
```

as separate states.

---

# 18. SIGNAL ENGINE

The signal engine answers:

> Is there an actual setup?

A signal must contain:

```text
signal_id
decision_id
direction
strategy_family
setup_type
trigger
entry_reference
invalidating_condition
timestamp
timeframe
feature_snapshot
```

A signal is immutable once published.

Later information must not rewrite the historical signal.

If the signal expires:

```text
SIGNAL_EXPIRED
```

It must not be silently refreshed.

---

# 19. MULTI-TIMEFRAME CONFLUENCE

Multi-timeframe confluence is allowed, but must be causal.

For decision time `t`:

```text
Feature_TF(t)
```

may only use information that was available at or before `t`.

Example:

```text
H4 completed structure
+
H1 confirmed context
+
M15 valid setup
+
M5 pullback context
+
M1 acceptable execution
```

is a valid multi-timeframe snapshot if every component was actually available at the decision timestamp.

A future H4/M15/M5 close must never leak backward into an earlier decision.

---

# 20. SCORE ENGINE

The project may use a deterministic score such as:

```text
SignalScore = f(
    structural agreement,
    regime quality,
    setup quality,
    momentum,
    volatility,
    market quality,
    macro state,
    execution quality
)
```

However:

```text
SIGNAL SCORE != PROBABILITY
```

The score is an ordinal or engineered strength measure until empirically calibrated.

Do not write:

```text
78% probability of winning
```

merely because:

```text
Score = 78
```

Instead:

```text
SIGNAL SCORE: 78 / 100
CONFIDENCE: HIGH
CALIBRATED PROBABILITY: N/A
```

until proper calibration exists.

---

# 21. CONFIDENCE MODEL

Confidence should represent the quality/completeness of the information and conditions supporting the setup.

Possible dimensions:

```text
data completeness
timeframe agreement
regime certainty
signal quality
market quality
macro consistency
execution quality
model validity
```

Example:

```text
Confidence = HIGH
```

does not mean:

```text
Trade will win
```

It means the system's required evidence is comparatively strong under the current model.

---

# 22. CALIBRATED PROBABILITY

A probability output is allowed only after defining:

```text
event/outcome
horizon
entry rule
exit rule
cost model
dataset
training period
validation period
calibration method
out-of-sample test
```

Example:

```text
P(
    +R outcome before -R outcome
    within H bars
    | setup state
)
```

must be estimated from historical observations.

Calibration must be tested with appropriate methods and held-out data.

Until then:

```text
probability = NULL
```

not an invented percentage.

---

# 23. MACRO AND EVENT ENGINE

Macro data must be point-in-time.

For every event:

```text
event_id
event_type
observation_period
scheduled_time
publication_time
provider_observed_time
collector_received_time
usable_time
revision_time
value
previous_value
forecast
dispersion
revision
source
source_version
```

Fundamental invariant:

```text
usable_time <= decision_time
```

A later revision cannot silently replace the originally available value in historical replay.

Potential event domains:

```text
CPI
PCE
NFP
FOMC
Fed communication
rates
real yields
USD
liquidity
inflation expectations
central-bank activity
ETF flows
COT/positioning
geopolitical events
major risk events
```

Event handling must remain descriptive and state-based.

The system must not hard-code simplistic rules such as:

```text
CPI high -> GOLD SELL
```

without empirical research.

---

# 24. EVENT CLUSTERS AND SHOCK STATES

A single event may be less informative than a cluster.

The engine may classify:

```text
NORMAL
EVENT_APPROACHING
EVENT_ACTIVE
POST_EVENT
SHOCK
POST_SHOCK
UNRESOLVED
```

Potential event-cluster features:

```text
time distance
importance
surprise
forecast dispersion
prior event
event overlap
market regime
volatility state
spread state
```

During unresolved shock conditions, the system may block or downgrade new setups according to an explicit policy.

---

# 25. MARKET QUALITY ENGINE

Market quality is separate from signal quality.

Possible states:

```text
GOOD
ACCEPTABLE
DEGRADED
POOR
UNKNOWN
```

Inputs:

```text
spread
spread/ATR
quote freshness
tick activity
short-horizon volatility
gap state
execution latency
session
broker state
```

A beautiful signal in a poor execution environment remains a poor candidate for execution.

---

# 26. RISK ENGINE

The risk engine answers:

> How much exposure is acceptable?

It does not decide whether the signal exists.

Canonical conceptual risk:

```text
RiskBudget =
Equity
*
RiskFraction
*
DrawdownThrottle
*
VolatilityThrottle
```

Position size is then derived from monetary loss under the proposed stop and broker economics.

Do not size positions from:

```text
desired lot size
```

Size from:

```text
allowed monetary risk
```

subject to:

```text
broker volume limits
margin
portfolio exposure
account mode
symbol economics
```

---

# 27. STOP MODEL

A protective stop may combine:

```text
structural invalidation
ATR distance
volatility state
gap reserve
broker minimum stop distance
```

Conceptually:

```text
SL = f(
    structural distance,
    ATR,
    volatility,
    gap reserve
)
```

The exact formula and multipliers are research hypotheses.

A position should not intentionally be opened without tested protective logic.

If the broker rejects the protected entry:

```text
DO NOT CONTINUE WITH UNPROTECTED EXPOSURE
```

unless a specific broker-tested emergency procedure exists.

---

# 28. PORTFOLIO RISK

Even though the asset scope is XAUUSD-only, multiple strategies and timeframes may create overlapping exposure.

Therefore the system must distinguish:

```text
strategy exposure
directional exposure
gross exposure
net exposure
correlated signal exposure
existing position exposure
pending simulated exposure
```

For the initial SHADOW system, the portfolio engine still runs even though no real order is sent.

This allows the research ledger to answer:

```text
Would multiple signals have stacked?
Would risk have exceeded policy?
Would simultaneous strategies conflict?
```

---

# 29. EXECUTION GATE

The execution gate is independent of the signal.

Canonical conditions:

```text
DataHealthy
AND
SessionAllowed
AND
EventPolicyAllowed
AND
LiquidityAcceptable
AND
SpreadAcceptable
AND
QuoteFresh
AND
RegimeValid
AND
StrategyEligible
AND
SignalValid
AND
RiskApproved
AND
PortfolioApproved
AND
SystemHealthy
```

If any mandatory condition is false:

```text
ExecutionAllowed = FALSE
```

The system should preserve the reason.

Example:

```text
REJECT_REASON =
SPREAD_TOO_WIDE
```

not merely:

```text
NO_TRADE
```

---

# 30. SHADOW MODE — CORE PROJECT MODE

SHADOW is the default operational mode.

It computes:

```text
signal
risk
position size
entry proposal
SL
TP
order type
execution conditions
simulated fill
simulated slippage
simulated costs
position lifecycle
exit
outcome
```

It does NOT send a real order.

The purpose is to reproduce the entire decision-to-outcome chain without financial exposure.

---

# 31. SHADOW MODE MUST NOT BE A FAKE ORDER MODE

Weak implementation:

```text
SIGNAL
+
TELEGRAM
```

Strong implementation:

```text
DECISION
    ->
RISK
    ->
ORDER PROPOSAL
    ->
MARKET SNAPSHOT
    ->
SIMULATED FILL
    ->
SLIPPAGE MODEL
    ->
COMMISSION
    ->
SWAP
    ->
PARTIAL FILL MODEL
    ->
POSITION
    ->
MANAGEMENT
    ->
EXIT
    ->
OUTCOME
```

This makes SHADOW a genuine experimental environment.

---

# 32. SHADOW EXECUTION MODEL

The simulator should record:

```text
decision_price
requested_price
simulated_fill_price
spread
estimated_slippage
latency
commission
swap
fill probability assumptions
partial fill assumptions
rejection assumptions
```

Every assumption must be labeled:

```text
OBSERVED
ESTIMATED
ASSUMED
MODELLED
UNKNOWN
```

Do not disguise assumptions as observations.

---

# 33. SHADOW LEDGER

Canonical table:

```text
shadow_decisions
```

Suggested fields:

```text
decision_id
signal_id
timestamp
symbol

trigger_timeframe
regime
strategy_family
direction

signal_score
confidence
calibrated_probability

entry_proposal
sl
tp
volume

spread
quote_age
estimated_slippage

execution_state
simulated_fill

position_open_time
position_close_time

MFE
MAE

gross_pnl
transaction_cost
net_pnl

exit_reason

model_version
strategy_version
configuration_version

data_quality_state
macro_state
event_state

status
```

The ledger is immutable for historical facts.

Corrections should be represented as versioned records or explicit correction events, not silent overwrites.

---

# 34. POSITION MANAGEMENT IN SHADOW

Once a simulated position exists:

```text
HEALTH
    ->
MFE / MAE
    ->
BREAKEVEN POLICY
    ->
PARTIAL EXIT
    ->
STRUCTURAL TRAILING
    ->
ATR TRAILING
    ->
REGIME INVALIDATION
    ->
EMERGENCY POLICY
    ->
EXIT
```

The management rules must be evaluated using only information available at each simulated timestamp.

---

# 35. SNIPER OPPORTUNITY ENGINE

The Sniper Engine is not a second strategy.

It is a notification policy layered on top of the canonical decision.

Example:

```text
IF
    SignalScore >= SniperScoreThreshold
AND
    Confidence >= RequiredConfidence
AND
    DataState acceptable
AND
    Regime valid
AND
    Event policy allowed
AND
    Market quality acceptable
AND
    Risk approved
AND
    System healthy
THEN
    SNIPER_CANDIDATE
```

Important:

```text
SNIPER THRESHOLD != SAFETY THRESHOLD
```

An 80/100 score is not a guarantee and not a probability.

---

# 36. TELEGRAM ARCHITECTURE

Telegram is the human observation layer.

It should report:

```text
market state
system state
signal state
risk proposal
shadow execution state
data health
alerts
errors
recovery events
research summaries
```

Telegram MUST NOT be the authoritative state store.

Telegram MUST NOT be required for the core engine to remain safe.

If Telegram is offline:

```text
CORE CONTINUES
LEDGER CONTINUES
ALERTS ARE QUEUED
```

where technically possible.

---

# 37. TELEGRAM MESSAGE TYPES

## Periodic market report

```text
XAUUSD SHADOW MARKET REPORT

H4:
TREND_UP

M15:
PULLBACK

M5:
LOCAL_DOWN

M1:
VALID

Signal:
NO_ACTIVE_SETUP

Score:
64 / 100

Confidence:
MEDIUM

Calibrated Probability:
N/A

Market Quality:
ACCEPTABLE

System:
HEALTHY

Mode:
SHADOW
```

## Sniper candidate

```text
XAUUSD — SNIPER CANDIDATE

Direction:
BUY

Strategy:
H4 Trend Continuation

Trigger:
M15 Close

Signal Score:
86 / 100

Confidence:
HIGH

Calibrated Probability:
N/A / not validated

Higher-Timeframe Context:
W1 BULL
D1 BULL
H4 BULL
H1 BULL

Operational Context:
M15 BREAKOUT
M5 PULLBACK
M1 VALID

Execution Quality:
ACCEPTABLE

Risk Proposal:
0.xx%

SL:
...

TP:
...

Mode:
SHADOW

Reason:
All mandatory gates passed.
```

## System warning

```text
SYSTEM WARNING

Adapter:
M5

State:
OFFLINE

Impact:
M5 execution context unavailable

Action:
M5-dependent execution filters disabled

H4/M15:
Still operational

Mode:
DEGRADED
```

---

# 38. ALERT DEDUPLICATION

Telegram alerts must be idempotent.

The same decision should not generate unlimited messages.

Use:

```text
alert_id
decision_id
alert_type
state_transition
sent_at
delivery_status
```

Examples:

```text
SNIPER_CANDIDATE_CREATED
SNIPER_CANDIDATE_INVALIDATED
DATA_DEGRADED
SYSTEM_RECOVERY
SHADOW_POSITION_OPENED
SHADOW_POSITION_CLOSED
CRITICAL_ERROR
```

Only meaningful state transitions should generate high-priority notifications.

---

# 39. SYSTEM MODES

Canonical modes:

```text
STARTING
RECOVERY
NORMAL
DEGRADED
SHADOW
PAUSED
MANUAL
EMERGENCY
HALTED
```

SHADOW is the default research operating mode.

Mode transitions must be:

```text
explicit
validated
logged
timestamped
auditable
```

---

# 40. EMERGENCY MODEL

Emergency conditions include:

```text
critical data mismatch
unexpected real exposure
reconciliation failure
broker state unknown
critical persistence failure
unprotected position
clock anomaly
corrupted state
```

Emergency behavior:

```text
1. disable new exposure
2. verify connectivity
3. verify account
4. verify positions
5. verify protective state
6. reconcile
7. preserve safety
8. remain halted until recovery criteria are satisfied
```

Because the initial project is SHADOW, emergency handling mainly protects the system and prevents accidental transition into live execution.

---

# 41. RECONCILIATION

If live execution is ever enabled, the canonical chain is:

```text
DecisionID
    ->
CommandID
    ->
Broker Order ID
    ->
Deal ID(s)
    ->
Position ID
```

Never assume:

```text
OrderSend success = filled
```

Never assume transaction events arrive in a simplistic linear order.

After restart or connection loss:

```text
CONNECT
    ->
QUERY ACCOUNT
    ->
QUERY ORDERS
    ->
QUERY DEALS
    ->
QUERY POSITIONS
    ->
RECONSTRUCT STATE
    ->
COMPARE PERSISTED STATE
    ->
RESOLVE DIFFERENCES
    ->
ONLY THEN ARM
```

Blind resend is prohibited.

---

# 42. NETTING VS HEDGING

At startup detect:

```text
NETTING
or
HEDGING
```

The position model must branch accordingly.

Netting:

```text
one net position per symbol
multiple deals may contribute
```

Hedging:

```text
multiple positions may coexist
```

The shadow simulator should be capable of reproducing both policy models if the research environment requires it.

---

# 43. PERSISTENCE

Critical state must survive process restart.

Persist at minimum:

```text
adapter health
last processed bar per timeframe
decision IDs
signals
scores
confidence
risk proposals
shadow positions
simulated fills
outcomes
configuration version
strategy version
system mode
errors
alerts
research experiment IDs
```

Persistence failure is a safety event.

---

# 44. DATABASE DOMAINS

Recommended logical domains:

```text
raw_ticks
raw_bars
symbol_specifications
spread_history
execution_events
orders
deals
positions
account_state
market_regime
signals
risk_decisions
strategy_decisions
system_events
data_quality
latency
performance
macro_events
macro_revisions
shadow_decisions
shadow_positions
shadow_outcomes
telegram_alerts
experiments
model_versions
configuration_versions
```

The exact physical database technology may vary.

The conceptual separation should remain.

---

# 45. POINT-IN-TIME RESEARCH

Every historical decision must be reconstructable from the information set available at that time.

For any feature:

```text
FEATURE(t)
```

must satisfy:

```text
uses only information <= t
```

Future outcome data:

```text
OUTCOME(t + 1 ... t + k)
```

is permitted only as an evaluation label.

This distinction is mandatory.

---

# 46. REPLAY ENGINE

The system should eventually support deterministic replay:

```text
RAW DATA
    ->
EVENT ORDERING
    ->
STATE RECONSTRUCTION
    ->
FEATURES
    ->
REGIME
    ->
SIGNAL
    ->
RISK
    ->
SHADOW EXECUTION
    ->
OUTCOME
```

Given:

```text
same raw data
+
same broker specification
+
same configuration
+
same model versions
+
same event-order policy
```

the result should be reproducible.

---

# 47. RESEARCH EXPERIMENT FRAMEWORK

Every research question must be represented as:

```text
RESEARCH_ID
QUESTION
HYPOTHESIS
DATA_REQUIREMENTS
TIME_WINDOW
POINT_IN_TIME_RULE
FEATURES
EXPERIMENT
METRIC
BASELINE
RESULT
FAILURE_CASES
ARCHITECTURE_IMPACT
DECISION
STATUS
```

Allowed statuses:

```text
UNKNOWN
REQUIRES_EXPERIMENT
DATA_INSUFFICIENT
DATA_COMPLETENESS_UNPROVEN
POINT_IN_TIME_UNKNOWN
SUPPORTED
NOT_SUPPORTED
INCONCLUSIVE
ARCHIVED
```

---

# 48. RESEARCH EXIT CONDITION

Every research item must end with:

```text
QUESTION
    ->
EVIDENCE
    ->
ARCHITECTURE IMPACT
    ->
DECISION
    ->
EXPERIMENT
    ->
VALIDATION
```

No research item should remain indefinitely as generic theory.

If the answer changes architecture, the architecture version must change.

If the answer does not change architecture and the evidence is sufficient, archive it.

---

# 49. RESEARCH GOVERNANCE

Do not optimize only one historical equity curve.

Optimize for robustness across:

```text
time
regimes
costs
brokers
parameter perturbations
execution assumptions
```

Do not erase failures.

Failures are research evidence.

Do not promote a backtest directly to production.

Do not allow alpha hypotheses to override safety.

Do not treat unknown as neutral.

---

# 50. BACKTESTING

Backtests must distinguish:

```text
structural signal validation
from
execution validation
```

For structural logic:

```text
completed bars may be sufficient
```

For execution-sensitive logic:

```text
real ticks are preferred when available
```

Execution tests should model:

```text
spread
slippage
commission
swap
latency
partial fills
rejections
gaps
intrabar ambiguity
```

If the historical data cannot establish the event sequence:

```text
AMBIGUOUS
```

must be recorded rather than silently choosing a favorable sequence.

---

# 51. TESTING PYRAMID

Mandatory test families:

```text
UNIT
INTEGRATION
PROPERTY
HISTORICAL
REPLAY
LOOKAHEAD
NO-REPAINT
FAULT INJECTION
RESTART
NETWORK FAILURE
BROKER CONSTRAINT
EXECUTION
RISK
RECONCILIATION
```

Property examples:

```text
future data cannot alter a historical decision
duplicate events cannot create duplicate decisions
duplicate commands cannot create duplicate executions
risk rounding cannot exceed policy
stop modification cannot loosen protection unintentionally
intrabar movement cannot change a completed higher-timeframe signal
```

---

# 52. STARTUP CONTRACT

On startup:

```text
LOAD CONFIG
    ->
VALIDATE CONFIG
    ->
CONNECT ADAPTERS
    ->
IDENTIFY BROKER
    ->
IDENTIFY SYMBOL
    ->
LOAD SYMBOL SPEC
    ->
LOAD PERSISTENT STATE
    ->
CHECK CLOCK
    ->
CHECK HISTORY
    ->
CHECK DATA HEALTH
    ->
RECONCILE STATE
    ->
INITIALIZE FEATURES
    ->
INITIALIZE REGIME
    ->
INITIALIZE SHADOW ENGINE
    ->
INITIALIZE TELEGRAM
    ->
ENTER APPROPRIATE MODE
```

The system must not silently enter an armed state when critical state is unknown.

---

# 53. CONFIGURATION MANAGEMENT

Configuration must be versioned.

A configuration includes:

```text
thresholds
weights
risk limits
strategy enablement
event policies
execution assumptions
alert thresholds
simulation parameters
```

Every decision must store:

```text
configuration_version
strategy_version
model_version
```

Changing a parameter creates a new version.

Historical decisions must remain attributable to the configuration that produced them.

---

# 54. NO SILENT CONFIGURATION MUTATION

Research experiments must never silently change production configuration.

Use:

```text
RESEARCH CONFIG
```

and:

```text
RUNTIME CONFIG
```

as distinct artifacts.

Promotion requires an explicit process.

---

# 55. MODEL CONTRACT

If ML or statistical models are later introduced, each model must have:

```text
model_id
version
training_period
feature_schema
label_definition
training_data_version
validation_data_version
calibration_status
deployment_status
```

An unvalidated ML prediction is not directional authority merely because it produces a number.

ML may be introduced as:

```text
feature
classifier
ranking model
probability estimator
anomaly detector
```

but must remain subject to the same point-in-time, OOS, and reproducibility requirements.

---

# 56. PERFORMANCE METRICS

The system should report multiple dimensions rather than one headline number.

## Signal metrics

```text
signal frequency
directional hit rate
conditional outcome distribution
MAE
MFE
time to target
time to invalidation
```

## Execution metrics

```text
spread
slippage
latency
fill quality
rejection rate
```

## Risk metrics

```text
risk per decision
max drawdown
exposure
risk concentration
loss clustering
```

## System metrics

```text
uptime
adapter availability
data gaps
message latency
reconciliation failures
restart recovery
alert delivery
```

A high signal hit rate with poor execution may still produce poor outcomes.

---

# 57. SCORE AND OUTCOME SEPARATION

The research database must allow:

```text
SCORE
CONFIDENCE
PROBABILITY
OUTCOME
```

to be analyzed separately.

Example:

```text
Score 80-90
```

may later prove to have:

```text
lower
same
or higher
```

outcome quality than another band.

This must be discovered empirically.

Never encode the conclusion before testing it.

---

# 58. MULTI-TIMEFRAME CONFLICT HANDLING

The system must never silently resolve:

```text
H4 BUY
+
M15 SELL
```

as one direction.

Instead represent:

```text
H4 = BULLISH
M15 = BEARISH
CONFLICT = TRUE
```

Then apply an explicit policy:

```text
BLOCK
WAIT
ALLOW COUNTERTREND STRATEGY
DOWNGRADE CONFIDENCE
```

The selected policy must be configuration-controlled and experimentally validated.

---

# 59. STRATEGY FAMILIES

The initial strategy library may contain:

```text
TREND_CONTINUATION
TREND_PULLBACK
BREAKOUT
MEAN_REVERSION
RANGE_BOUNDARY
MOMENTUM
STRUCTURAL_REVERSAL
POST_SHOCK_REACTION
```

Each strategy must define:

```text
required regime
required timeframe state
entry condition
invalidation condition
risk model
execution requirements
exit model
cooldown
research status
```

No strategy may bypass:

```text
risk
execution gate
system health
```

---

# 60. STRATEGY INDEPENDENCE

Strategies should be independently enableable.

Example:

```text
TREND_CONTINUATION = ENABLED
BREAKOUT = ENABLED
MEAN_REVERSION = DISABLED
POST_SHOCK = RESEARCH_ONLY
```

This makes experimentation controlled.

---

# 61. COOLDOWN AND DUPLICATE SIGNALS

A signal should have an explicit lifecycle:

```text
CREATED
VALIDATED
ACTIVE
TRIGGERED
SIMULATED
EXPIRED
INVALIDATED
CLOSED
```

Repeated evaluations of the same setup must not create unlimited independent decisions.

Use deterministic identity:

```text
DecisionID =
Hash(
    symbol,
    trigger timeframe,
    closed bar ID,
    strategy version,
    configuration version
)
```

---

# 62. IDEMPOTENCY

Idempotency is mandatory at every boundary.

Examples:

```text
duplicate tick
duplicate bar
duplicate event
duplicate command
duplicate alert
duplicate simulated fill
duplicate partial close
```

must not create duplicate state transitions.

---

# 63. EXECUTION SIMULATION QUALITY

The simulator should avoid optimistic assumptions.

At minimum evaluate:

```text
bid/ask
spread
entry side
exit side
latency
slippage
stop distance
gap behavior
commission
swap
partial fill
rejection
```

The simulation must record which assumptions are:

```text
observed
estimated
modelled
unknown
```

---

# 64. POSITION COST MODEL

For each shadow position:

```text
gross outcome
+
commission
+
spread effect
+
slippage
+
swap
=
net outcome
```

The exact accounting depends on broker conditions and instrument specification.

The system must not claim that a gross theoretical price move equals realizable PnL.

---

# 65. MARKET SESSION MODEL

Session state is a first-class input.

Record:

```text
broker session
server time
UTC time
day of week
rollover proximity
market closure
holiday state if known
```

Session policies must be explicit.

---

# 66. WEEKEND AND GAP MODEL

The system must identify:

```text
weekend gap
session gap
data gap
quote gap
execution gap
```

These are not interchangeable.

Gap behavior should be recorded separately and used in research.

---

# 67. LATENCY MODEL

Store at least:

```text
source timestamp
collector receive timestamp
core processing timestamp
decision timestamp
telegram timestamp
simulated execution timestamp
```

This allows:

```text
source -> decision latency
decision -> simulation latency
```

to be measured.

---

# 68. CLOCK SAFETY

Clock anomalies can corrupt point-in-time logic.

Detect:

```text
clock backwards
future timestamp
large clock drift
inconsistent server/local time
```

If time cannot be trusted:

```text
new decisions = BLOCKED
```

until recovery.

---

# 69. HEALTH MONITOR

Health state should summarize:

```text
adapter connectivity
data freshness
database health
event queue health
C++ process health
Telegram health
clock health
configuration integrity
replay consistency
```

Example:

```text
SYSTEM HEALTH = HEALTHY
```

is allowed only if mandatory components satisfy their health policy.

---

# 70. WATCHDOG

The watchdog should detect:

```text
stalled event loop
dead adapter
queue overload
database failure
unresponsive Telegram worker
unexpected state transition
```

The watchdog itself must not silently mutate strategy state.

It can:

```text
raise alert
enter degraded mode
request recovery
halt new decisions
```

according to explicit policy.

---

# 71. THREADING / C++ CORE PRINCIPLE

The C++ system should separate:

```text
INGESTION
STATE
FEATURES
STRATEGY
RISK
SIMULATION
PERSISTENCE
NOTIFICATION
WATCHDOG
```

Avoid one giant event handler containing all logic.

Prefer deterministic state transitions.

Conceptually:

```text
Event
 ->
Validated Event
 ->
State Update
 ->
Feature Snapshot
 ->
Decision Function
 ->
Persist Decision
 ->
Simulation
 ->
Persist Outcome
 ->
Notify
```

---

# 72. MODULE CONTRACTS

Recommended modules:

```text
AdapterManager
DataBus
DataValidator
BarFinalizer
TimeframeStateStore
FeatureEngine
StructureEngine
RegimeEngine
EligibilityEngine
SignalEngine
ScoreEngine
ConfidenceEngine
ProbabilityEngine
MacroContextEngine
MarketQualityEngine
RiskEngine
PortfolioRiskEngine
ShadowExecutionEngine
PositionSimulator
ReconciliationEngine
PersistenceEngine
ResearchEngine
ReplayEngine
AlertEngine
TelegramGateway
HealthMonitor
Watchdog
ConfigurationRegistry
AuditEngine
```

No module should silently bypass another.

---

# 73. SEPARATION OF RESPONSIBILITIES

The canonical questions are:

```text
Data Validator:
"Is the data trustworthy enough?"

Feature Engine:
"What can be measured?"

Structure Engine:
"What structure exists?"

Regime Engine:
"What type of market is this?"

Eligibility Engine:
"Which strategies may search?"

Signal Engine:
"Is there a setup?"

Score Engine:
"How strong is the engineered evidence?"

Confidence Engine:
"How complete/reliable is the evidence?"

Probability Engine:
"What does calibrated historical evidence imply?"

Risk Engine:
"How much exposure is allowed?"

Execution Gate:
"Can this be safely simulated/executed now?"

Shadow Engine:
"What would execution and lifecycle have looked like?"

Reconciliation:
"What actually happened?"

Research Engine:
"What does the evidence support?"

Telegram:
"What should the human know now?"
```

---

# 74. HUMAN-IN-THE-LOOP DESIGN

The initial project is a decision-support system.

The human receives:

```text
context
signal
score
confidence
risk proposal
market quality
reason
uncertainty
```

The system should make uncertainty visible.

Examples:

```text
CALIBRATED PROBABILITY: NOT AVAILABLE
M1 DATA: DEGRADED
MACRO DATA: POINT-IN-TIME UNPROVEN
EXECUTION MODEL: ESTIMATED
```

This is more useful than pretending the system knows more than it does.

---

# 75. TELEGRAM COMMAND INTERFACE

If command functionality is later added, commands must be restricted.

Possible safe commands:

```text
/status
/market
/decision <id>
/shadow <id>
/health
/research <id>
/pause
/resume
```

Any command capable of changing trading behavior must require:

```text
authentication
authorization
audit logging
explicit state transition
```

No Telegram message should directly execute a broker order without passing the same core safety gates.

---

# 76. RESEARCH DASHBOARD CONCEPT

The system should eventually expose:

```text
Current regime
Timeframe matrix
Active signals
Score distribution
Confidence state
Data quality
Macro state
Shadow positions
Recent outcomes
System health
Research status
```

Telegram is the fast interface.

The database is the source of truth.

The C++ core is the runtime authority.

---

# 77. DATA RETENTION

Raw data should be preserved wherever practical.

Prefer:

```text
RAW
PROCESSED
FEATURE
DECISION
OUTCOME
```

as distinct layers.

Never overwrite raw observations merely because a later parser or model changed.

---

# 78. VERSIONING

Version:

```text
raw schema
processed schema
feature schema
strategy
configuration
model
execution simulator
broker profile
research experiment
```

Every important record should be traceable to versions.

---

# 79. BROKER PROFILE

Each broker environment should have a profile:

```text
broker_id
server
symbol
timezone
contract_size
tick_size
tick_value
volume rules
margin rules
sessions
spread behavior
execution behavior
history coverage
known quirks
```

Do not assume one broker's XAUUSD behavior transfers automatically to another.

---

# 80. RESEARCH QUESTIONS FOR BROKER BEHAVIOR

Examples:

```text
Does spread widening cluster around specific sessions?

Does spread/ATR predict poor execution quality?

Does tick arrival rate predict realized volatility?

How stable are symbol specifications?

How often do data gaps occur?

How often does OnTick delivery differ from stored tick history?

How frequently do execution requests reject?

How large is realized slippage?

Does broker behavior change around news?
```

These are research questions, not conclusions.

---

# 81. M15 PRIMARY OPERATING LOOP

On each completed M15 bar:

```text
CAPTURE M15 CLOSE
    ->
VALIDATE M15
    ->
UPDATE H4 CONTEXT
    ->
UPDATE HIGHER-TF CONTEXT
    ->
CALCULATE M15 FEATURES
    ->
UPDATE REGIME
    ->
CHECK ELIGIBILITY
    ->
GENERATE SIGNAL
    ->
SCORE
    ->
CONFIDENCE
    ->
MACRO/EVENT GATE
    ->
MARKET QUALITY GATE
    ->
RISK PROPOSAL
    ->
SHADOW EXECUTION PROPOSAL
    ->
PERSIST
    ->
SNIPER EVALUATION
    ->
TELEGRAM
```

This is the primary operational cycle.

---

# 82. H4 PRIMARY STRUCTURAL LOOP

On each completed H4 bar:

```text
CAPTURE H4 CLOSE
    ->
VALIDATE
    ->
CALCULATE STRUCTURE
    ->
CALCULATE VOLATILITY
    ->
CLASSIFY REGIME
    ->
APPLY PERSISTENCE/HYSTERESIS
    ->
UPDATE STRUCTURAL STATE
    ->
PERSIST
    ->
PUBLISH CONTEXT TO M15 ENGINE
```

H4 does not need to produce an independent order.

Its primary job is to define the structural environment.

---

# 83. HIGHER TIMEFRAME UPDATE LOOP

On:

```text
MN1 close
W1 close
D1 close
H4 close
H1 close
M30 close
```

update the relevant context snapshot.

Do not force a new trade on every higher-timeframe close.

The higher-timeframe state becomes part of the next valid M15 decision snapshot.

---

# 84. M5/M1 EXECUTION LOOP

During an active M15 setup:

```text
M5 close
M1 close
tick
```

may update:

```text
execution quality
pullback state
spread
volatility
quote freshness
entry timing
```

They may reject or delay a candidate if policy permits.

They must not silently rewrite the original M15 signal.

---

# 85. IMMUTABLE DECISION SNAPSHOT

When a decision is generated, freeze:

```text
market state
features
regime
eligibility
signal
score
confidence
macro state
risk proposal
execution state
configuration version
```

This becomes:

```text
IMMUTABLE DECISION SNAPSHOT
```

Later data produces a new state, not a rewritten old decision.

---

# 86. DECISION IDENTITY

Recommended:

```text
DecisionID =
Hash(
    symbol,
    trigger_timeframe,
    closed_bar_id,
    strategy_version,
    configuration_version
)
```

If the same decision already exists:

```text
DO NOT DUPLICATE
```

A new decision requires a new causal trigger or version.

---

# 87. OPPORTUNITY LIFECYCLE

```text
DETECTED
    ->
VALIDATING
    ->
QUALIFIED
    ->
SNIPER_CANDIDATE
    ->
SHADOW_ENTRY_PROPOSED
    ->
SHADOW_OPEN
    ->
MANAGING
    ->
CLOSED
```

Alternative terminal states:

```text
REJECTED
EXPIRED
INVALIDATED
DATA_INVALID
RISK_BLOCKED
EXECUTION_BLOCKED
SYSTEM_BLOCKED
```

Every terminal state needs a reason.

---

# 88. EXPLAINABILITY CONTRACT

For every signal the system must answer:

```text
WHY THIS DIRECTION?
WHY THIS STRATEGY?
WHY NOW?
WHY THIS REGIME?
WHY THIS SCORE?
WHY THIS CONFIDENCE?
WHY THIS RISK?
WHY THIS ENTRY?
WHY THIS STOP?
WHY WAS EXECUTION ALLOWED/BLOCKED?
```

The explanation should be generated from stored facts.

Do not generate post-hoc narratives from future outcomes.

---

# 89. FAILURE-FIRST DESIGN

Every important module should define:

```text
NORMAL
DEGRADED
FAILURE
RECOVERY
```

Example:

```text
Database:
NORMAL -> write
DEGRADED -> local spool
FAILURE -> block new decisions
RECOVERY -> reconcile spool

Telegram:
NORMAL -> send
DEGRADED -> queue
FAILURE -> core continues
RECOVERY -> flush queued alerts
```

---

# 90. DURABLE EVENT SPOOL

If persistence is temporarily unavailable, high-value events should be written to a durable spool where possible.

The spool must preserve:

```text
event_id
sequence
timestamp
payload
checksum
```

On recovery:

```text
SPOOL
    ->
VALIDATE
    ->
DEDUPE
    ->
REPLAY
    ->
COMMIT
```

---

# 91. DATA COMPLETENESS

A missing adapter should not be hidden.

Record:

```text
missing_timeframe
missing_interval
expected_data
received_data
gap_duration
impact
```

Example:

```text
DATA_COMPLETENESS_UNPROVEN
```

means the system does not have enough evidence to assert completeness.

---

# 92. COST-AWARE RESEARCH

A setup may appear attractive before costs and unattractive after costs.

Therefore evaluate:

```text
gross outcome
net outcome
spread-adjusted outcome
slippage-adjusted outcome
commission-adjusted outcome
swap-adjusted outcome
```

For high-volatility XAUUSD periods, cost sensitivity is especially important.

---

# 93. PARAMETER STATUS

Every numeric parameter must carry a status:

```text
ENGINEERING_DEFAULT
RESEARCH_PRIOR
EMPIRICALLY_ESTIMATED
OOS_VALIDATED
PRODUCTION_APPROVED
```

A parameter in a source document is not automatically a proven optimal value.

Examples:

```text
ADX threshold
ATR multiplier
score weight
sniper threshold
spread limit
risk fraction
cooldown
```

remain research hypotheses until evidence supports them.

---

# 94. PARAMETER STABILITY

Do not optimize one exact number if a broad stable region exists.

Research:

```text
parameter perturbation
sensitivity
walk-forward stability
regime stability
broker stability
cost stability
```

A parameter that works only at:

```text
ADX = 23.17
```

but fails at 23 or 24 may be suspicious.

This is a research principle, not a guaranteed statistical conclusion.

---

# 95. MULTIPLE TESTING CONTROL

The project will inevitably test many:

```text
features
thresholds
strategies
timeframes
regimes
filters
models
```

Therefore research records should track:

```text
experiment count
parameter searches
datasets used
validation status
```

Do not treat the best historical result from a large search as automatically independent evidence.

---

# 96. OUT-OF-SAMPLE

The final OOS dataset must remain locked until:

```text
model
strategy
parameters
execution assumptions
```

are frozen.

OOS should be used as a genuine validation stage, not an optimization playground.

---

# 97. WALK-FORWARD

A robust workflow:

```text
TRAIN / RESEARCH WINDOW
        ->
VALIDATION WINDOW
        ->
FORWARD WINDOW
        ->
ROLL
```

Measure stability across multiple periods.

---

# 98. MONTE CARLO AND STRESS

Where appropriate, evaluate:

```text
trade sequence perturbation
slippage perturbation
spread perturbation
execution delay
parameter perturbation
loss clustering
gap shocks
```

The objective is not to prove certainty.

The objective is to understand fragility.

---

# 99. RESEARCH PROMOTION PIPELINE

Canonical promotion:

```text
RESEARCH
    ->
DATA VALIDITY
    ->
HISTORICAL TEST
    ->
COST TEST
    ->
WALK-FORWARD
    ->
OOS
    ->
SHADOW
    ->
PAPER / DEMO
    ->
CONTROLLED LIVE TEST
```

No stage should be skipped merely because results look attractive.

---

# 100. PRODUCTION BOUNDARY

Production is NOT the current goal.

If the system eventually reaches production, it must pass:

```text
data validation
no-lookahead
no-repaint
restart recovery
network recovery
reconciliation
risk
broker constraints
execution realism
OOS
walk-forward
stress
shadow
demo
controlled live
```

The production system remains conceptually separate from the research laboratory.

---

# 101. RESEARCH LAB VS PRODUCTION SYSTEM

The architecture should maintain:

```text
PROJECT A = RESEARCH LAB
PROJECT B = PRODUCTION TRADING SYSTEM
```

The research lab may:

```text
experiment
replay
simulate
mutate parameters
test hypotheses
run models
```

Production must be:

```text
frozen
versioned
audited
restricted
```

Research configuration cannot silently alter production.

---

# 102. PROMOTION PACKAGE

A candidate strategy should be promoted only with a package containing:

```text
strategy definition
data definition
PIT rules
feature schema
parameter version
backtest results
OOS results
walk-forward results
cost model
stress results
failure tests
shadow results
known limitations
broker profile
deployment configuration
rollback plan
```

---

# 103. CORE SAFETY INVARIANTS

The following are hard invariants:

```text
1. No future information.

2. No incomplete structural bar for structural decisions.

3. No duplicate decision for the same decision identity.

4. Signal != risk approval.

5. Risk approval != execution approval.

6. Execution approval != fill.

7. Order/request != deal.

8. Deal != position.

9. Unknown state != safe state.

10. Invalid data blocks dependent decisions.

11. Lower timeframe cannot silently rewrite higher-timeframe structural history.

12. Strategy cannot bypass risk.

13. Risk cannot bypass safety.

14. Execution cannot bypass system health.

15. Telegram cannot bypass the core.

16. Persistence failure is visible.

17. Restart requires state reconstruction.

18. Connection recovery requires reconciliation.

19. Raw data is never silently overwritten.

20. All important decisions are versioned.

21. Scores are not probabilities unless calibrated.

22. High confidence is not a guarantee.

23. Sniper threshold is not a safety guarantee.

24. Profitability is never assumed from architecture.

25. Research claims require evidence.

26. Broker-specific behavior is measured, not assumed.

27. Existing protective state must be preserved during strategy outages.

28. Unknown execution result must not trigger blind resend.

29. No risk increase merely to satisfy broker minimum volume.

30. All critical state transitions are auditable.
```

---

# 104. CANONICAL DECISION EQUATION

For the shadow decision:

```text
DecisionCandidate_t =
    DataValid_t
    AND
    RequiredTimeframesAvailable_t
    AND
    StructuralStateValid_t
    AND
    RegimeValid_t
    AND
    StrategyEligible_t
    AND
    SignalValid_t
    AND
    MacroPolicyAllowed_t
    AND
    MarketQualityAllowed_t
    AND
    RiskApproved_t
    AND
    SystemHealthy_t
```

Then:

```text
ShadowExecutionAllowed_t =
    DecisionCandidate_t
    AND
    QuoteFresh_t
    AND
    ExecutionModelValid_t
```

Then:

```text
SniperCandidate_t =
    ShadowExecutionAllowed_t
    AND
    SignalScore_t >= SniperThreshold
    AND
    Confidence_t >= RequiredConfidence
```

A Sniper Candidate is still a research output, not a promise.

---

# 105. CANONICAL RISK EQUATION

Conceptual:

```text
RiskBudget_t =
Equity_t
*
BaseRiskFraction
*
DrawdownThrottle_t
*
VolatilityThrottle_t
*
StrategyThrottle_t
```

Then:

```text
AllowedVolume =
min(
    RiskVolume,
    MarginVolume,
    PortfolioVolume,
    BrokerVolume
)
```

subject to:

```text
volume_min
volume_max
volume_step
```

If broker minimum volume would require exceeding the risk budget:

```text
REJECT
```

Do not increase risk simply to satisfy the broker minimum.

---

# 106. CANONICAL STOP EQUATION

Conceptual:

```text
StopDistance =
max(
    StructuralInvalidationDistance,
    ATRDistance,
    MinimumBrokerDistance
)
+
GapReserve
```

The final formula remains a research hypothesis.

---

# 107. CANONICAL IDENTITY EQUATION

For eventual live execution:

```text
Decision
    ->
Command
    ->
Order
    ->
Deal(s)
    ->
Position
```

For current SHADOW:

```text
Decision
    ->
ShadowCommand
    ->
SimulatedFill
    ->
ShadowPosition
    ->
ShadowExit
    ->
Outcome
```

The two chains must be structurally comparable.

---

# 108. CURRENT PROJECT STATE

The correct current state should be understood as:

```text
ARCHITECTURE:
DEFINED

IMPLEMENTATION:
TO BE BUILT

BROKER EMPIRICAL VALIDATION:
REQUIRED

NUMERICAL PARAMETERS:
RESEARCH HYPOTHESES

PROBABILITY CALIBRATION:
NOT ESTABLISHED

PROFITABILITY:
UNPROVEN

SHADOW ENGINE:
CORE TARGET

TELEGRAM:
OBSERVATION / ALERT LAYER

LIVE AUTOMATION:
NOT THE INITIAL OBJECTIVE
```

---

# 109. IMPLEMENTATION PHASES

## Phase 1 — Infrastructure

```text
C++ process
configuration
logging
database
IPC
health monitor
adapter manager
```

## Phase 2 — Data

```text
9 adapters
bar capture
tick capture
symbol specification
data validation
gap detection
health states
```

## Phase 3 — Timeframe State

```text
M1
M5
M15
M30
H1
H4
D1
W1
MN1
```

## Phase 4 — Feature and Regime

```text
feature engine
structure engine
regime engine
eligibility engine
```

## Phase 5 — Signal

```text
strategy families
signal engine
score
confidence
```

## Phase 6 — Macro

```text
event database
PIT architecture
event state
macro gate
```

## Phase 7 — Shadow

```text
risk
position sizing
execution simulator
position manager
outcomes
```

## Phase 8 — Telegram

```text
status
periodic reports
sniper alerts
system warnings
research reports
```

## Phase 9 — Replay / Research

```text
historical replay
walk-forward
OOS
parameter experiments
stress
```

## Phase 10 — Optional Demo/Live

Only after evidence and safety gates.

---

# 110. MINIMUM VIABLE SYSTEM

The first implementation should NOT attempt every strategy.

MVP:

```text
XAUUSD
M15
H4
M5
M1
basic higher-timeframe context
data validation
regime
one strategy family
risk proposal
shadow execution
shadow ledger
Telegram
health monitor
replay
```

Then expand.

This prevents the nine-timeframe architecture from becoming nine unfinished systems.

---

# 111. EXPANSION ORDER

Recommended engineering order:

```text
M15
    ->
H4
    ->
M5/M1
    ->
H1/M30
    ->
D1
    ->
W1
    ->
MN1
```

The adapters may be implemented earlier, but analytical authority should be added incrementally.

---

# 112. OBSERVABILITY

Every important subsystem should expose:

```text
state
last event
last successful update
last error
latency
queue depth
data age
version
```

Example:

```text
H4:
VALID
last_close = ...
age = ...
regime = TREND_UP

M15:
VALID
last_close = ...
signal = NONE

M1:
DEGRADED
age = ...
reason = adapter timeout
```

---

# 113. ERROR TAXONOMY

Use structured errors:

```text
DATA_ERROR
SCHEMA_ERROR
CLOCK_ERROR
CONNECTION_ERROR
BROKER_ERROR
RISK_ERROR
EXECUTION_ERROR
RECONCILIATION_ERROR
PERSISTENCE_ERROR
MODEL_ERROR
CONFIG_ERROR
TELEGRAM_ERROR
RECOVERY_ERROR
```

Each error should contain:

```text
error_id
component
severity
timestamp
state
message
context
recovery_action
```

---

# 114. AUDIT LOG

Every decision must be reproducible from stored state.

Audit record should include:

```text
decision_id
symbol
timestamp
timeframe state
features
regime
eligibility
signal
score
confidence
macro
market quality
risk
execution proposal
configuration version
strategy version
model version
system mode
```

The audit record is a technical record, not a marketing explanation.

---

# 115. RESEARCH NOTEBOOK MODEL

For every major architectural decision, maintain:

```text
DECISION_ID
QUESTION
OPTIONS
EVIDENCE
ASSUMPTIONS
SELECTED DESIGN
REJECTED DESIGNS
RISKS
TEST REQUIRED
STATUS
```

This creates a permanent engineering memory for the project.

---

# 116. WHAT THE SYSTEM MUST NEVER CLAIM

It must never claim:

```text
"80% win probability"
```

from a score alone.

It must never claim:

```text
"guaranteed setup"
```

It must never claim:

```text
"safe trade"
```

because a threshold was crossed.

It must never claim:

```text
"profitable system"
```

from architecture.

It must never claim:

```text
"global liquidity"
```

from broker DOM.

It must never claim:

```text
"one tick per OnTick"
```

without appropriate data verification.

---

# 117. WHAT THE SYSTEM SHOULD SAY INSTEAD

Use:

```text
SIGNAL SCORE
CONFIDENCE
CALIBRATED PROBABILITY
DATA QUALITY
EXECUTION QUALITY
RISK STATUS
RESEARCH STATUS
```

Examples:

```text
SIGNAL SCORE: 84/100
CONFIDENCE: HIGH
CALIBRATED PROBABILITY: NOT VALIDATED
DATA QUALITY: VALID
EXECUTION QUALITY: ACCEPTABLE
RISK: APPROVED FOR SHADOW
```

This language is precise.

---

# 118. FINAL OPERATING LOOP

The complete runtime loop is:

```text
                    START
                      |
                      v
               ENVIRONMENT CHECK
                      |
                      v
                 LOAD CONFIG
                      |
                      v
              CONNECT 9 ADAPTERS
                      |
                      v
              DATA VALIDATION
                      |
                      v
              TIMEFRAME STATE
                      |
          +-----------+-----------+
          |                       |
          v                       v
     STRUCTURAL CLOCK       EXECUTION CLOCK
          |                       |
          v                       v
      FEATURES                QUOTES/TICKS
          |                       |
          v                       |
      STRUCTURE                   |
          |                       |
          v                       |
       REGIME <-------------------+
          |
          v
     ELIGIBILITY
          |
          v
       SIGNAL
          |
          v
    SCORE / CONFIDENCE
          |
          v
    MACRO / EVENT GATE
          |
          v
    MARKET QUALITY
          |
          v
        RISK
          |
          v
   SHADOW EXECUTION
          |
          v
   SHADOW POSITION
          |
          v
       OUTCOME
          |
          v
   RESEARCH / AUDIT
          |
          v
      TELEGRAM
```

---

# 119. FINAL PROJECT PRINCIPLES

The project should be built around these principles:

```text
1. DATA BEFORE SIGNAL.

2. CAUSAL TIME BEFORE PERFORMANCE.

3. STRUCTURE BEFORE ENTRY.

4. REGIME BEFORE STRATEGY.

5. STRATEGY BEFORE SIGNAL.

6. SIGNAL BEFORE RISK.

7. RISK BEFORE EXECUTION.

8. EXECUTION BEFORE OUTCOME.

9. RECONCILIATION BEFORE TRUST.

10. RESEARCH BEFORE PROMOTION.

11. SHADOW BEFORE LIVE.

12. EVIDENCE BEFORE CLAIM.

13. UNKNOWN IS NOT SAFE.

14. BROKER REALITY BEATS ABSTRACT ASSUMPTIONS.

15. RAW DATA MUST REMAIN TRACEABLE.

16. DECISIONS MUST BE REPRODUCIBLE.

17. CONFIGURATION MUST BE VERSIONED.

18. FAILURES ARE RESEARCH DATA.

19. TELEGRAM IS AN INTERFACE, NOT THE BRAIN.

20. SCORE IS NOT PROBABILITY.

21. CONFIDENCE IS NOT GUARANTEE.

22. HIGH SCORE IS NOT SAFETY.

23. NO FUTURE INFORMATION.

24. NO SILENT REPAINTING.

25. NO BLIND RESEND.

26. NO UNPROTECTED EXPOSURE.

27. NO ARCHITECTURAL ASSUMPTION IS ALPHA.

28. EVERY HYPOTHESIS MUST HAVE AN EXIT CONDITION.
```

---

# 120. CANONICAL END STATE

The intended end state is:

```text
                  XAUUSD
                     |
          +----------+----------+
          |                     |
       MARKET                 MACRO
        DATA                  DATA
          |                     |
          +----------+----------+
                     |
              C++ DATA BUS
                     |
              DATA VALIDATION
                     |
              TIMEFRAME STORE
                     |
          +----------+----------+
          |                     |
     STRUCTURAL CORE       EXECUTION CORE
          |                     |
     MN1/W1/D1/H4/H1       M30/M15/M5/M1/TICK
          |                     |
          +----------+----------+
                     |
              REGIME ENGINE
                     |
          STRATEGY ELIGIBILITY
                     |
               SIGNAL ENGINE
                     |
             SCORE/CONFIDENCE
                     |
            MACRO/EVENT GATE
                     |
                RISK ENGINE
                     |
          SHADOW EXECUTION ENGINE
                     |
             SHADOW LEDGER
                     |
              RESEARCH ENGINE
                     |
          +----------+----------+
          |                     |
       AUDIT                TELEGRAM
```

The final conceptual equation is:

```text
SYSTEM QUALITY
=
DATA INTEGRITY
*
CAUSAL TIMING
*
STATE CONSISTENCY
*
REPRODUCIBILITY
*
RISK DISCIPLINE
*
EXECUTION REALISM
*
RESEARCH VALIDITY
```

This is a conceptual architecture equation, not a statistical claim.

The project succeeds when it can reliably answer:

```text
WHAT DID WE KNOW?
WHEN DID WE KNOW IT?
WHAT DID WE DECIDE?
WHY DID WE DECIDE IT?
WHAT WOULD WE HAVE EXECUTED?
WHAT WOULD IT HAVE COST?
WHAT HAPPENED NEXT?
WAS THE HYPOTHESIS SUPPORTED?
```

That is the purpose of the system.

---

# 121. SOURCE-CORPUS COVERAGE

This canonical document consolidates the major concepts from the nine supplied source documents:

```text
M1_MASTER_CONSOLIDATED_AI_READABLE.txt
XAUUSD_D1_Institutional_Algorithmic_Trading_Core_—_Complete_Master.md
XAUUSD_H4_Institutional_Algorithmic_Trading_Core_—_Complete_Master.md
XAUUSD_M5_Quantitative_Trading_Core_—_Production_Oriented_Master.md
XAUUSD_MN1_Sovereign_Macro_Trading_Core_—_Exhaustive_Institutional.md
XAUUSD_W1_Sovereign_Grade_Algorithmic_Trading_Core_—_Complete_Master.md
FINAL_V2_—_XAUUSD_H1_Institutional_Grade_Algorithmic_Trading_Core.md
Institutional_Grade_XAUUSD_M1–M5_Algorithmic_Trading_Core_—_Master.md
Institutional_Grade_XAUUSD_M30_Algorithmic_Trading_Core_—_Complete.md
```

The consolidation intentionally changes several concepts into canonical project-wide rules:

```text
H4 = structural authority
M15 = primary operational trigger
M5/M1 = execution context
TICK = execution/broker state

SCORE != PROBABILITY
CONFIDENCE != GUARANTEE
SNIPER THRESHOLD != SAFETY THRESHOLD

SHADOW = full simulated lifecycle
not merely signal display

ADAPTER FAILURE = capability degradation
not automatic application failure

TELEGRAM = interface
not source of truth

RESEARCH = hypothesis + evidence + experiment + exit condition
```

These changes are deliberate architectural normalization intended to make the project internally coherent.

---

# 122. IMPLEMENTATION CHECKLIST

## Infrastructure

```text
[ ] C++ core repository
[ ] configuration registry
[ ] structured logging
[ ] persistent database
[ ] event bus
[ ] IPC protocol
[ ] watchdog
[ ] health monitor
```

## Adapters

```text
[ ] M1 adapter
[ ] M5 adapter
[ ] M15 adapter
[ ] M30 adapter
[ ] H1 adapter
[ ] H4 adapter
[ ] D1 adapter
[ ] W1 adapter
[ ] MN1 adapter
```

## Data

```text
[ ] raw ticks
[ ] bars
[ ] symbol specifications
[ ] spread history
[ ] gaps
[ ] quality states
[ ] timestamps
[ ] sequence IDs
```

## Core

```text
[ ] timeframe state store
[ ] feature engine
[ ] structure engine
[ ] regime engine
[ ] eligibility engine
[ ] signal engine
[ ] score engine
[ ] confidence engine
[ ] macro engine
[ ] risk engine
[ ] market quality engine
```

## Shadow

```text
[ ] order proposal
[ ] fill simulator
[ ] slippage
[ ] commission
[ ] swap
[ ] partial fills
[ ] position manager
[ ] MAE/MFE
[ ] outcome ledger
```

## Research

```text
[ ] replay
[ ] historical tests
[ ] walk-forward
[ ] OOS
[ ] parameter sensitivity
[ ] Monte Carlo
[ ] execution-cost tests
[ ] failure injection
```

## Telegram

```text
[ ] system status
[ ] market status
[ ] periodic report
[ ] sniper alert
[ ] data warning
[ ] recovery warning
[ ] shadow position report
[ ] research report
[ ] alert deduplication
```

## Safety

```text
[ ] no-lookahead test
[ ] no-repaint test
[ ] duplicate-event test
[ ] duplicate-decision test
[ ] restart test
[ ] network failure test
[ ] persistence failure test
[ ] broker constraint test
[ ] risk test
[ ] reconciliation test
[ ] clock test
```

---

# 123. MASTER RESEARCH EXIT CONDITIONS

The following questions remain explicitly empirical:

```text
RQ-001
Which H4 regime definitions are stable on the actual broker XAUUSD feed?

RQ-002
Does H4 regime context improve M15 setup selection?

RQ-003
Does M15 setup quality improve when H1/M30 context agrees?

RQ-004
Does M5/M1 execution context improve realized shadow execution?

RQ-005
Does spread/ATR predict poor execution outcomes?

RQ-006
Does quote freshness materially affect simulated entry quality?

RQ-007
Does event state alter setup outcome distributions?

RQ-008
Does score monotonicity exist?

RQ-009
Can score be calibrated into a reliable probability?

RQ-010
Which strategy families survive cost-adjusted OOS testing?

RQ-011
Which parameters remain stable under perturbation?

RQ-012
Does the shadow execution model resemble demo execution?

RQ-013
How much does broker-specific behavior alter results?

RQ-014
How much signal overlap exists across timeframes?

RQ-015
What risk fraction survives drawdown and execution stress?

RQ-016
Which conditions should disable entries?

RQ-017
Which Telegram alerts provide useful information without alert fatigue?

RQ-018
Does the multi-timeframe architecture improve robustness versus a single-timeframe baseline?
```

Each research question must eventually become:

```text
QUESTION
    ->
DATA
    ->
EXPERIMENT
    ->
RESULT
    ->
ARCHITECTURE IMPACT
    ->
DECISION
    ->
STATUS
```

---

# 124. FINAL STATUS

```text
DOCUMENT STATUS:
CANONICAL MASTER REFERENCE

ARCHITECTURE:
DEFINED

ENGINEERING DESIGN:
DEFINED AT SYSTEM LEVEL

IMPLEMENTATION:
NOT YET COMPLETE

EMPIRICAL VALIDATION:
REQUIRED

PROBABILITY CALIBRATION:
NOT ESTABLISHED

PROFITABILITY:
NOT CLAIMED

PRIMARY MODE:
SHADOW

PRIMARY OPERATIONAL TIMEFRAME:
M15

PRIMARY STRUCTURAL AUTHORITY:
H4

LONG-HORIZON CONTEXT:
MN1 / W1 / D1

INTERMEDIATE CONTEXT:
H1 / M30

EXECUTION CONTEXT:
M5 / M1 / TICK

HUMAN INTERFACE:
TELEGRAM

RUNTIME CORE:
C++

DATA/EXECUTION BOUNDARY:
MT5/MQL5 ADAPTERS

SOURCE OF TRUTH:
PERSISTENT EVENT/DECISION LEDGER

RESEARCH TRUTH:
POINT-IN-TIME REPLAY + OUT-OF-SAMPLE EVIDENCE
```

---

# 125. FINAL PRINCIPLE

The project is not being built to answer:

```text
"Can we predict gold?"
```

It is being built to answer a more rigorous sequence:

```text
Can we collect trustworthy broker-specific XAUUSD data?

Can we prove when information became available?

Can we construct deterministic multi-timeframe state?

Can we classify market structure without lookahead?

Can we determine which strategy families are eligible?

Can we generate immutable setups?

Can we distinguish score from probability?

Can we estimate risk without violating account constraints?

Can we simulate realistic execution?

Can we reproduce every decision?

Can we measure what happened afterward?

Can we determine which hypotheses survive out-of-sample evidence?

Can we safely promote only what deserves promotion?
```

That is the project.

The system should be treated as a **research laboratory, deterministic decision engine, shadow execution simulator, and human intelligence interface** first.

Any future live trading capability is downstream of evidence.



# APPENDIX A — SOURCE HEADING COVERAGE

The following headings were detected in the nine supplied source documents and were used to ensure the canonical reference covers their major domains. The original files remain available separately as source artifacts.


## M1_MASTER_CONSOLIDATED_AI_READABLE.txt

- MASTER XAUUSD M15 ALGORITHMIC TRADING SYSTEM SPECIFICATION
- 0. DOCUMENT IDENTITY
- 0.1 Normative language
- 0.2 Canonical separation of responsibilities
- 1. CANONICAL END-TO-END ARCHITECTURE
- 2. GLOBAL PRODUCTION INVARIANTS
- 3. SOURCE CONSOLIDATION MAP
- 3.1 CONTENT MAP
- 4. DATA, TIME, BAR FINALIZATION, AND DATA QUALITY
- DOCUMENT PURPOSE
- DESIGN PRINCIPLES
- 1.1 Deterministic
- 1.2 Point-in-Time Safe
- 1.3 Closed-Bar Signal Locking
- 1.4 Broker-Aware
- 1.5 Auditable
- 1.6 No Hidden Precedence
- SYSTEM BOUNDARY
- TIME AND M15 BAR SEMANTICS
- 3.1 M15 Duration
- 2 Forming Bar vs Finalized Bar
- 3 OnTick Is Not a Historical Tick Database
- 4 Bar Finalization State Machine
- RAW DATA CONTRACT
- 4.1 Bar Data
- 4.2 Tick Data
- DATA INTEGRITY VALIDATION
- 5.1 OHLC Constraints
- 5.2 Temporal Consistency
- 5.3 Duplicate Detection
- 5.4 Tick Integrity
- 4.1 Production Data-Layer Extensions
- MARKET DATA LAYER
- TICK DATA MODEL
- TICK-CALLBACK VS TRUE-TICK DATA
- M15 BAR CONSTRUCTION
- 5. CANONICAL MATHEMATICAL FEATURE LIBRARY — VOLATILITY AND REGIME MEASURES
- TRUE RANGE
- Research result
- AVERAGE TRUE RANGE — ATR
- Implementation
- Research result
- NORMALIZED ATR — NATR
- Research result
- PRICE VARIANCE
- Mandatory implementation decision
- RETURN VOLATILITY
- Research result
- Z-SCORE
- PERCENTILE RANK
- Research result
- VOLATILITY RATIO
- REALIZED VARIANCE
- Research result
- BIPOWER VARIATION
- PARKINSON VOLATILITY
- GARMAN-KLASS VOLATILITY
- ROGERS-SATCHELL VOLATILITY
- Implementation rule
- EWMA VOLATILITY
- GARCH(1,1)
- Research result
- BOLLINGER BANDS
- Implementation
- Boolean examples
- DIRECTIONAL MOVEMENT — DM
- WILDER SMOOTHING
- DIRECTIONAL INDICATORS
- DX
- ADX
- LINEAR REGRESSION SLOPE
- LINEAR REGRESSION \(R^2\)
- EFFICIENCY RATIO
- REGIME VECTOR
- EXAMPLE MULTI-DIMENSIONAL REGIME RULE
- VOLATILITY STATE
- 5.1 Technical Indicator and Signal Math
- SIMPLE MOVING AVERAGE — SMA
- WEIGHTED MOVING AVERAGE — WMA
- EXPONENTIAL MOVING AVERAGE — EMA

## XAUUSD_D1_Institutional_Algorithmic_Trading_Core_—_Complete_Master.md

- XAUUSD D1 INSTITUTIONAL ALGORITHMIC TRADING CORE
- Complete Quantitative Research, Mathematical Specification, System Architecture, Execution, Risk, Validation and Production Blueprint
- 1. SYSTEM MISSION
- 2. NON-NEGOTIABLE ARCHITECTURAL INVARIANTS
- INV-001 — CLOSED D1 ONLY
- INV-002 — ONE D1 DECISION PER CLOSED BAR
- INV-003 — NO LOWER-TIMEFRAME SIGNAL OVERRIDE
- INV-004 — SIGNAL ≠ ORDER
- INV-005 — ORDER ACCEPTANCE ≠ FILL
- 3. BROKER-SPECIFIC D1 DEFINITION
- 4. DATA QUALITY ENGINE
- 5. MINIMUM HISTORY
- 6. D1 SNAPSHOT OBJECT
- 7. PRICE RETURN MODEL
- 8. TRUE RANGE
- 9. ATR ENGINE
- 10. NORMALIZED ATR
- 11. ATR TERM STRUCTURE
- 12. STANDARD DEVIATION
- 13. VOLATILITY TERM STRUCTURE
- 14. VOLATILITY PERCENTILE
- 15. VOLATILITY STATE
- 16. VOLATILITY SHOCK
- 17. VOLATILITY EXPANSION VS VOLATILITY SHOCK
- 18. ADX ENGINE
- 19. EMA STRUCTURE
- 20. EMA SLOPE
- 21. LONG-TERM TREND SCORE
- 22. STRUCTURAL SWING ENGINE
- 23. STRUCTURAL SEQUENCES
- 24. STRUCTURAL BREAK
- 25. CHANNEL ENGINE
- 26. CHANNEL EXPANSION
- 27. MACRO REGIME TAXONOMY
- 28. BULL REGIME
- 29. BEAR REGIME
- 30. CONSOLIDATION REGIME
- 31. VOLATILITY EXPANSION REGIME
- 32. EXTREME UNSTABLE REGIME
- 33. REGIME CONFIDENCE
- 34. REGIME PERSISTENCE
- 35. REGIME HYSTERESIS
- 36. REGIME TRANSITION MATRIX
- 37. STRATEGY ELIGIBILITY MATRIX
- 38. ELIGIBILITY FUNCTION
- 39. DAILY CLOSE EVENT
- 40. SIGNAL FAMILY 1 — STRUCTURAL BREAKOUT
- 41. SIGNAL FAMILY 2 — EMA TREND TRANSITION
- 42. SIGNAL FAMILY 3 — MULTI-DAY CHANNEL BREAK
- 43. BREAKOUT QUALITY
- 44. EXTENSION FILTER
- 45. EXTREME CANDLE FILTER
- 46. SIGNAL SCORE
- 47. SIGNAL CONFLICT RESOLUTION
- 48. SIGNAL COOLDOWN
- 49. FAILED BREAKOUT
- 50. SIGNAL LIFECYCLE
- 51. RISK ENGINE
- 52. ACCOUNT RISK BUDGET
- 53. STOP DISTANCE
- 54. LONG STOP
- 55. STOP BUFFER
- 56. DYNAMIC ATR STOP
- 57. MONETARY LOSS PER LOT
- 58. MQL5 PROFIT/MARGIN CALCULATION
- 59. VOLUME NORMALIZATION
- 60. PORTFOLIO RISK
- 61. PORTFOLIO HEAT
- 62. DRAW-DOWN SCALING
- 63. CONSECUTIVE LOSS CONTROL
- 64. MARGIN CONSTRAINT
- 65. FINAL POSITION SIZE
- 66. TAKE-PROFIT MODEL
- 67. PARTIAL EXIT
- 68. BREAKEVEN
- 69. ATR TRAILING
- 70. STRUCTURAL TRAILING
- 71. TRAILING PRIORITY
- 72. EXIT HIERARCHY
- 73. TIME EXIT

## XAUUSD_H4_Institutional_Algorithmic_Trading_Core_—_Complete_Master.md

- XAUUSD H4 INSTITUTIONAL ALGORITHMIC TRADING CORE
- COMPLETE MASTER RESEARCH, MATHEMATICAL SPECIFICATION, PROGRAMMATIC ARCHITECTURE, RISK ENGINE, EXECUTION ENGINE, FAIL-SAFE DESIGN, AND VALIDATION FRAMEWORK
- 1. EXECUTIVE SYSTEM DEFINITION
- 1.1 Mission
- 2. IMPORTANT MODELING PRINCIPLE
- 2.1 H4 is the decision clock
- 3. BROKER-SPECIFIC XAUUSD PRINCIPLE
- 4. COMPLETE SYSTEM ARCHITECTURE
- 5. ARCHITECTURAL LAYER SEPARATION
- 6. DATA CONTRACT
- 6.1 H4 bar
- 7. H4 DATA FIREWALL
- 8. HISTORY REQUIREMENTS
- 9. H4 CLOSE EVENT ENGINE
- 10. PRIMARY VOLATILITY ENGINE
- 10.1 True Range
- 10.2 Wilder ATR
- 10.3 Volatility Ratio
- 11. RETURN VOLATILITY
- 12. VOLATILITY PERCENTILE
- 13. VOLATILITY-OF-VOLATILITY
- 14. EMA ENGINE
- 15. MACRO TREND DISTANCE
- 16. EMA SLOPE
- 17. ADX ENGINE
- 18. ADX SLOPE
- 19. CANDLE QUALITY ENGINE
- 20. CLOSE LOCATION
- 21. STRUCTURAL SWING ENGINE
- 22. STRUCTURAL SWING SIGNIFICANCE
- 23. BULLISH STRUCTURE
- 24. BEARISH STRUCTURE
- 25. STRUCTURAL CHANNEL
- 26. TREND PERSISTENCE SCORE
- 27. EXPANSION SCORE
- 28. TREND SCORE
- 29. CONSOLIDATION SCORE
- 30. MACRO REGIME STATES
- 31. EXPANSION-UP HARD GATE
- 32. EXPANSION-DOWN HARD GATE
- 33. TREND-UP HARD GATE
- 34. TREND-DOWN HARD GATE
- 35. CONSOLIDATION HARD GATE
- 36. TRANSITION
- 37. REGIME PRIORITY
- 38. REGIME CONFIDENCE
- 39. REGIME HYSTERESIS
- 40. TREND EXIT HYSTERESIS
- 41. REGIME TRANSITION MATRIX
- 42. STRATEGY FAMILIES
- 43. ELIGIBILITY MATRIX
- 44. EXPANSION AGE CONTROL
- 45. SIGNAL PRECEDENCE
- 46. STRUCTURAL BREAKOUT LONG
- 47. STRUCTURAL BREAKOUT SHORT
- 48. BREAKOUT FALSE-BREAK FILTER
- 49. BREAKOUT EXHAUSTION FILTER
- 50. TREND CONTINUATION LONG
- 51. TREND CONTINUATION SHORT
- 52. STRUCTURAL RETRACEMENT LONG
- 53. STRUCTURAL RETRACEMENT SHORT
- 54. CANDLE-CLOSE INTEGRITY
- 55. INTRABAR INVARIANCE PROPERTY
- 56. SIGNAL EXPIRATION
- 57. PRICE DRIFT PROTECTION
- 58. OPTIONAL MACRO-CONTEXT ENGINE
- 59. POINT-IN-TIME MACRO DATA CONTRACT
- 60. MACRO EVENT FILTER
- 61. MACRO CONTEXT CONFLICT
- 62. SESSION ENGINE
- 63. TIME MODEL
- 64. PREFERRED EXECUTION WINDOW
- 65. FRIDAY GUARD
- 66. MONDAY GAP ENGINE
- 67. SPREAD ENGINE
- 68. SPREAD PERSISTENCE BREAKER
- 69. QUOTE FRESHNESS
- 70. POSITION RISK MODEL
- 71. VOLATILITY RISK SCALER
- 72. DRAWDOWN RISK SCALER

## XAUUSD_M5_Quantitative_Trading_Core_—_Production_Oriented_Master.md

- XAUUSD M5 QUANTITATIVE TRADING CORE
- Production-Oriented Master Blueprint
- Market Regime → Strategy Gating → Close-Confirmed Signal → Dynamic Risk → Smart Execution → Position Management → Fail-Safe
- 0. SYSTEM OBJECTIVE
- 1. CORE ENGINE PRINCIPLES
- 1.1 Separation of Responsibilities
- 2. TIME AND DATA MODEL
- 2.1 Primary Trading Timeframe
- 3. TICK INGESTION
- 3.1 Executable Price
- 3.2 Important Tick Event Rule
- 4. M1 MICRO-NOISE FILTER
- 5. M5 FEATURE ENGINE
- 5.1 True Range
- 5.2 Wilder ATR
- 6. NORMALIZED VOLATILITY
- 6.1 ATR Volatility Ratio
- 7. ATR Z-SCORE
- 8. M5 STANDARD DEVIATION
- 9. ADX / DIRECTIONAL STRENGTH
- 10. EFFICIENCY RATIO
- 11. EMA STRUCTURE
- 12. M5 DONCHIAN STRUCTURE
- 13. CANDLE QUALITY FEATURES
- 14. MARKET REGIME STATE MACHINE
- 15. EXPANSION DETECTION
- 15.1 Expansion Up
- 15.2 Expansion Down
- 16. TREND DETECTION
- 16.1 Trend Up
- 16.2 Trend Down
- 17. RANGE DETECTION
- 18. REGIME PRIORITY
- 19. REGIME HYSTERESIS
- Trend retention
- Range retention
- Expansion retention
- 20. MASTER ELIGIBILITY BOOLEAN
- 21. STRATEGY ELIGIBILITY MATRIX
- 22. SESSION FILTER
- 23. SPREAD GUARD
- 24. OPTIONAL NEWS LOCK
- 25. DYNAMIC RSI THRESHOLDS
- 26. MOMENTUM SCALP SIGNAL
- 26.1 Long
- 26.2 Short
- 27. PULLBACK CONTINUATION SIGNAL
- 27.1 Long
- 27.2 Short
- 28. BREAKOUT-RETEST SIGNAL
- Long retest
- Short retest
- 29. RANGE-FADE SIGNAL
- Long
- Short
- 30. ENTRY CONFIRMATION RULE
- 31. DECISION ID
- 32. ENTRY CHASE CONTROL
- 33. RISK ENGINE
- 34. BASE RISK
- 35. VOLATILITY RISK THROTTLE
- 36. DRAWDOWN THROTTLE
- 37. EFFECTIVE RISK
- 38. DAILY LOSS LOCK
- 39. POSITION LIMIT
- 40. STOP-LOSS DISTANCE
- 41. MAXIMUM STOP DISTANCE
- 42. STOP PRICE
- 43. TICK-SIZE QUANTIZATION
- 44. POSITION SIZE
- 45. SLIPPAGE RISK BUFFER
- 46. RAW LOT SIZE
- 47. VOLUME CONSTRAINTS
- 48. MARGIN VALIDATION
- 49. TAKE-PROFIT STRUCTURE
- 50. NET REWARD FILTER
- 51. BREAKEVEN SYSTEM
- 52. TRAILING STOP
- 53. TRAILING UPDATE FILTER
- 54. PARTIAL TAKE-PROFIT

## XAUUSD_MN1_Sovereign_Macro_Trading_Core_—_Exhaustive_Institutional.md

- XAUUSD MN1 SOVEREIGN MACRO TRADING CORE
- Exhaustive Institutional Research, Mathematical Specification, System Architecture, Validation Framework, and Implementation Contract
- 0. EXECUTIVE DEFINITION
- 1. SYSTEM SCOPE
- 1.1 Primary Use Cases
- 2. CORE DESIGN PRINCIPLES
- 2.1 Completed-Bar Principle
- 2.2 Broker-Specific Price Reality
- 3. DATA ARCHITECTURE
- 3.1 Primary Market Dataset
- 4. POINT-IN-TIME MACRO DATA ARCHITECTURE
- 4.1 Important Macro Domains
- Monetary Policy
- Rates
- Inflation
- USD
- Liquidity
- Fiscal
- Gold-Specific Fundamentals
- Positioning
- Risk Environment
- 5. MACRO DATA NORMALIZATION
- 6. STANDARDIZATION
- 7. MACRO FACTOR DIRECTION
- 8. MACRO FACTOR ORTHOGONALIZATION
- 9. LONG-HORIZON VOLATILITY ENGINE
- 9.1 Monthly True Range
- 9.2 Wilder ATR
- 10. NORMALIZED VOLATILITY
- 11. MONTHLY RETURN VOLATILITY
- 12. EWMA VOLATILITY
- 13. VOLATILITY TERM STRUCTURE
- 14. VOLATILITY PERCENTILE
- 15. VOLATILITY CLUSTERING
- 16. VOL-OF-VOL
- 17. EXTREME TAIL ENGINE
- 18. ADX / DIRECTIONAL MOVEMENT ENGINE
- 19. TREND STRENGTH SCORE
- 20. MOVING-AVERAGE STRUCTURE
- 21. STRUCTURAL EMA FEATURES
- 22. SECULAR BULL REGIME
- 23. SECULAR BEAR REGIME
- 24. CONSOLIDATION REGIME
- 25. SOVEREIGN CONSOLIDATION BASE
- 26. REGIME TRANSITION DETECTION
- 27. HYSTERESIS
- 28. MARKOV / HMM REGIME ENGINE
- 29. BAYESIAN REGIME MODEL
- 30. CHANGE-POINT ENGINE
- 31. FINAL REGIME OBJECT
- 32. STRATEGY REGISTRY
- S01 — Secular Trend Following
- S02 — Multi-Year Macro Swing
- S03 — Sovereign Breakout
- S04 — Regime Transition
- S05 — Structural Retest
- S06 — Defensive Exit / De-Risking
- 33. STRATEGY ELIGIBILITY MATRIX
- 34. BOOLEAN ELIGIBILITY CONTRACT
- 35. MACRO COMPATIBILITY SCORE
- 36. MACRO FILTER STATES
- 37. EVENT ARCHITECTURE
- 38. EVENT CLUSTERS
- 39. REVISION-AWARE MACRO MODEL
- 40. MN1 STRUCTURAL SIGNAL LIBRARY
- 41. MULTI-YEAR CHANNEL
- 42. BREAKOUT BUFFER
- 43. MONTHLY BREAKOUT QUALITY
- 44. BREAKOUT + RETEST
- 45. FAILED BREAKOUT
- 46. SIGNAL STATE MACHINE
- 47. MONTH-END DECISION PROCESS
- 48. DECISION SNAPSHOT
- 49. RISK MANAGEMENT ARCHITECTURE
- 50. PRIMARY RISK BUDGET
- 51. STOP DISTANCE
- 52. STOP LOCATION
- 53. POSITION SIZING
- 54. FINAL VOLUME
- 55. MARGIN CONTROL

## XAUUSD_W1_Sovereign_Grade_Algorithmic_Trading_Core_—_Complete_Master.md

- XAUUSD W1 SOVEREIGN-GRADE ALGORITHMIC TRADING CORE
- Complete Master Research, Mathematical Formulation, Software Architecture, Execution Specification, Risk Engine, Validation Framework, and Fail-Safe Design
- 1. EXECUTIVE SYSTEM DEFINITION
- 2. DESIGN PRINCIPLES
- 3. CRITICAL MT5 WEEKLY-BAR SEMANTICS
- 4. W1 MARKET DATA MODEL
- 5. HISTORY REQUIREMENT
- 6. DATA INTEGRITY ENGINE
- 7. RAW DATA VS DERIVED DATA
- 8. TRUE RANGE
- 9. WILDER ATR
- 10. STRUCTURAL ATR
- 11. LOG RETURN VOLATILITY
- 12. ROBUST VOLATILITY ESTIMATORS
- 13. VOLATILITY-REGIME RATIO
- 14. VOLATILITY EXPANSION
- 15. VOLATILITY HYSTERESIS
- 16. ADX
- 17. EMA ENGINE
- 18. EMA STRUCTURAL GAP
- 19. STRUCTURAL DIRECTION STATES
- 20. SECULAR BULL RAW STATE
- 21. SECULAR BEAR RAW STATE
- 22. STRUCTURAL BASE
- 23. TRANSITION STATES
- 24. SECULARIZATION REQUIREMENT
- 25. DIRECTIONAL REVERSAL CONFIRMATION
- 26. NOISE-SUPPRESSION ARCHITECTURE
- 27. ROBUST FEATURE FILTER
- 28. IMPORTANT LIMITATION
- 29. STRATEGY FAMILIES
- 30. STRATEGY ELIGIBILITY MATRIX
- 31. SIGNAL ENGINE STRUCTURAL INPUTS
- 32. 26-WEEK BREAKOUT
- 33. 52-WEEK BREAKOUT
- 34. STRUCTURAL CLOSE DISPLACEMENT
- 35. MAJOR SWING MODEL
- 36. CHANNEL WIDTH
- 37. EMA STRUCTURAL CONFIRMATION
- 38. HIGH-CONVICTION SIGNAL SCORE
- 39. WEEKLY CLOSE CONFIRMATION
- 40. SIGNAL IMMUTABILITY
- 41. SIGNAL EXPIRATION
- 42. ENTRY DRIFT
- 43. MACRO DATA ARCHITECTURE
- 44. POINT-IN-TIME PRINCIPLE
- 45. MACRO EVENT TYPES
- 46. EVENT SURPRISE
- 47. FORECAST DISPERSION
- 48. EVENT CLUSTERING
- 49. MACRO SHOCK DETECTOR
- 50. CROSS-MARKET MACRO FILTERS
- 51. LIQUIDITY ENGINE
- 52. WEEKLY OPEN EXECUTION WINDOW
- 53. LIQUIDITY-WINDOW CLASSIFICATION
- 54. RISK BUDGET
- 55. DRAWDOWN THROTTLE
- 56. VOLATILITY SCALING
- 57. STRATEGY RISK MULTIPLIER
- 58. STRUCTURAL STOP MODEL
- 59. BROKER STOP-DISTANCE CONSTRAINT
- 60. GAP-RISK RESERVE
- 61. ACCOUNT-CURRENCY LOSS PER LOT
- 62. MARGIN CAPACITY
- 63. FINAL LOT SIZE
- 64. ORDER PRECHECK
- 65. ORDER TRANSMISSION
- 66. IDEMPOTENT DECISION ID
- 67. COMMAND ID
- 68. ORDER RETRY LOGIC
- 69. DUPLICATE ORDER DEFENSE
- 70. MT5 TRANSACTION MODEL
- 71. TRANSACTION LEDGER
- 72. POSITION RECONCILIATION
- 73. NETTING VS HEDGING
- 74. POSITION AGGREGATION
- 75. FOREIGN / MANUAL POSITION DETECTION
- 76. POSITION MANAGEMENT MODES
- 77. BREAKEVEN
- 78. ATR TRAILING STOP

## FINAL_V2_—_XAUUSD_H1_Institutional_Grade_Algorithmic_Trading_Core.md

- XAUUSD H1 INSTITUTIONAL-GRADE ALGORITHMIC TRADING CORE
- FINAL V2 — COMPLETE RESEARCH, MATHEMATICAL, STRATEGY, RISK, EXECUTION, SOFTWARE, TESTING, BACKTESTING, AND DEPLOYMENT SPECIFICATION
- 0. DOCUMENT STATUS
- 1. PURPOSE
- 2. CORE PRINCIPLE
- 3. FUNDAMENTAL SAFETY INVARIANTS
- 4. SCOPE
- 4.1 Included
- 4.2 Excluded from directional authority
- 5. TIMEFRAME AUTHORITY
- 6. DATA MODEL
- 6.1 Tick
- 6.2 H1 Bar
- 6.3 Symbol Specification
- 7. DATA INTEGRITY ENGINE
- 8. DUPLICATE DATA
- 9. GAP DETECTION
- 10. STALE TICK PROTECTION
- 11. EVENT CLOCK
- 12. H1 FINALIZATION CONTRACT
- 13. FEATURE ENGINE
- 14. TRUE RANGE
- 15. ATR
- 16. NATR
- 17. LOG RETURNS
- 18. REALIZED VOLATILITY
- 19. VOLATILITY RATIO
- 20. VOLATILITY PERCENTILE
- 21. ADX
- 22. EMA
- 23. EMA SLOPE
- 24. CANDLE FEATURES
- 25. MARKET STRUCTURE ENGINE
- 26. PIVOT CONFIRMATION
- 27. HH / HL / LH / LL
- 28. BULLISH STRUCTURE
- 29. BEARISH STRUCTURE
- 30. BREAK OF STRUCTURE
- 31. STRUCTURAL BUFFER
- 32. RANGE ENGINE
- 33. REGIME ENGINE
- 34. BULL TREND RULE
- 35. BEAR TREND RULE
- 36. RANGE RULE
- 37. STRUCTURAL EXPANSION
- 38. REGIME PRIORITY
- 39. REGIME HYSTERESIS
- 40. REGIME PERSISTENCE
- 41. REGIME TRANSITION RECORD
- 42. STRATEGY ENGINE
- 43. STRATEGY ELIGIBILITY
- 44. TREND PULLBACK SPECIFICATION
- 45. TREND PULLBACK ENTRY
- 46. MACRO BREAKOUT SPECIFICATION
- 47. INSTITUTIONAL SWING CONTINUATION
- 48. RANGE STRATEGY
- 49. RANGE INVALIDATION
- 50. SIGNAL QUALITY
- 51. SIGNAL IDENTIFIER
- 52. SIGNAL DEDUPLICATION
- 53. MACRO EVENT ENGINE
- 54. EVENT TYPES
- 55. EVENT STATE
- 56. EVENT GATING
- 57. SESSION ENGINE
- 58. SPREAD ENGINE
- 59. MASTER ENTRY GATE
- 60. RISK ENGINE
- 61. ATR STOP
- 62. STRUCTURAL STOP
- 63. FINAL STOP
- 64. RISK PER LOT
- 65. COST-ADJUSTED LOSS
- 66. POSITION SIZE
- 67. BROKER LIMITS
- 68. MARGIN
- 69. PORTFOLIO RISK
- 70. THESIS AGGREGATION
- 71. DAILY LOSS
- 72. ORDER BUILDER

## Institutional_Grade_XAUUSD_M1–M5_Algorithmic_Trading_Core_—_Master.md

- INSTITUTIONAL-GRADE XAUUSD M1–M5 ALGORITHMIC TRADING CORE
- Master Quantitative, Mathematical, Execution, Risk, and Software Architecture Specification
- 0. CORE DESIGN PRINCIPLES
- 1. SYSTEM OBJECTIVE
- 2. SYSTEM STATES
- 3. DATA ARCHITECTURE
- 3.1 Required Data Domains
- 4. TICK DATA MODEL
- 5. DATA QUALITY ENGINE
- 6. STALE DATA DETECTION
- 7. M1 BAR CONSTRUCTION
- 8. MULTI-BAR CONTEXT
- 9. MICROSTRUCTURE VOLATILITY MODEL
- 9.1 Tick Return
- 10. MICRO-ATR
- 11. VOLATILITY NORMALIZATION
- 12. REALIZED TICK VOLATILITY
- 13. EMA SLOPE
- 14. MARKET REGIME CLASSIFICATION
- 15. HIGH-VOLATILITY EXPANSION
- 16. MICRO-TREND
- 17. CHOPPY NOISE
- 18. NOISE FILTER
- 19. REGIME HYSTERESIS
- 20. MARKET QUALITY ENGINE
- 21. SPREAD FILTER
- 22. LIQUIDITY VOID DETECTION
- 23. SESSION FILTER
- 24. EVENT / NEWS FILTER
- 25. STRATEGY ELIGIBILITY ENGINE
- 26. ELIGIBILITY MATRIX
- 27. BOOLEAN ELIGIBILITY
- 28. SIGNAL GENERATION
- 29. FAST EMA SIGNAL
- 30. MICRO-CHANNEL BREAKOUT
- 31. MOMENTUM CONFIRMATION
- 32. MULTI-BAR CONFIRMATION
- 33. ANTI-REPAINTING RULE
- 34. SIGNAL STATES
- 35. SIGNAL EXPIRATION
- 36. STOP-LOSS MODEL
- 37. STOP-LOSS VALIDATION
- 38. TAKE-PROFIT MODEL
- 39. RISK PER TRADE
- 40. POSITION SIZE
- 41. VOLUME NORMALIZATION
- 42. SLIPPAGE RISK BUFFER
- 43. MAXIMUM PORTFOLIO RISK
- 44. LOSS-STREAK PROTECTION
- 45. DAILY LOSS LIMIT
- 46. DRAWDOWN CONTROL
- 47. EXECUTION ENGINE
- 48. ORDER DECISION ID
- 49. IDEMPOTENCY
- 50. PRE-TRADE VALIDATION
- 51. ORDER TYPE
- 52. MAX SLIPPAGE
- 53. EXECUTION QUALITY
- 54. LATENCY MODEL
- 55. POSITION STATE MACHINE
- 56. INITIAL POSITION PROTECTION
- 57. SERVER-SIDE SAFETY
- 58. BREAKEVEN ENGINE
- 59. TRAILING ENGINE
- 60. PARTIAL TAKE PROFIT
- 61. EXIT CONDITIONS
- 62. TIME-BASED EXIT
- 63. REGIME INVALIDATION EXIT
- 64. CIRCUIT BREAKER
- 65. NETWORK FAILURE
- 66. STARTUP RECONCILIATION
- 67. POSITION RECONCILIATION
- 68. ACCOUNT MODE
- 69. DUPLICATE POSITION PROTECTION
- 70. SYSTEM HEALTH SCORE
- 71. COMPLETE ENTRY DECISION TREE
- 72. COMPLETE POSITION MANAGEMENT TREE
- 73. MASTER ENTRY FORMULA
- 74. STRATEGY ARCHITECTURE
- 75. FEATURE ENGINE

## Institutional_Grade_XAUUSD_M30_Algorithmic_Trading_Core_—_Complete.md

- INSTITUTIONAL-GRADE XAUUSD M30 ALGORITHMIC TRADING CORE
- Complete Mathematical, Structural, Risk, Execution, Safety, Backtesting, and Software Architecture Specification
- 0. SYSTEM PURPOSE
- 1. DESIGN PRINCIPLES
- 1.1 Primary Principles
- 2. NON-GOALS
- 3. HIGH-LEVEL SYSTEM ARCHITECTURE
- 4. DATA MODEL
- 5. BROKER-SPECIFICATION DISCOVERY
- 6. DATA INTEGRITY ENGINE
- 7. M30 BAR ENGINE
- 8. MULTI-TIMEFRAME ARCHITECTURE
- 8.1 M30
- 8.2 M5
- 8.3 M1
- 9. TRUE RANGE
- 10. ATR ENGINE
- 11. STANDARD DEVIATION
- 12. ADX ENGINE
- 13. MARKET REGIME MODEL
- 14. ESTABLISHED TREND
- 15. BROAD RANGE
- 16. VOLATILITY EXPANSION
- 17. REGIME HYSTERESIS
- 18. REGIME CONFIDENCE
- 19. REGIME STATE MACHINE
- 20. MARKET STRUCTURE ENGINE
- 21. SWING DETECTION
- 22. BREAK OF STRUCTURE
- 23. BREAKOUT VALIDATION
- 24. FALSE BREAKOUT FILTER
- 25. STRUCTURAL CHANNEL
- 26. SESSION FILTER
- 27. SPREAD FILTER
- 28. SPREAD SHOCK GUARD
- 29. STRATEGY FAMILIES
- 30. STRATEGY ELIGIBILITY MATRIX
- 31. MASTER ELIGIBILITY FUNCTION
- 32. TREND PULLBACK STRATEGY
- 33. PULLBACK DEFINITION
- 34. TREND CONTINUATION CONFIRMATION
- 35. STRUCTURAL BREAKOUT STRATEGY
- 36. BREAKOUT RETEST MODEL
- 37. RANGE MEAN REVERSION
- 38. MEAN-REVERSION INVALIDATION
- 39. VOLATILITY EXPANSION STRATEGY
- 40. CANDLE-CLOSE CONFIRMATION
- 41. SIGNAL SCORE
- 42. SIGNAL OBJECT
- 43. SIGNAL STATE MACHINE
- 44. STOP-LOSS MODEL
- 45. STOP BUFFER
- 46. TAKE-PROFIT MODEL
- 47. POSITION RISK
- 48. POSITION SIZE
- 49. SLIPPAGE BUFFER
- 50. VOLUME NORMALIZATION
- 51. ACCOUNT-LEVEL RISK
- 52. DAILY LOSS CIRCUIT BREAKER
- 53. CONSECUTIVE LOSS PROTECTION
- 54. MARGIN SAFETY
- 55. CORRELATED EXPOSURE
- 56. EXECUTION ENGINE
- 57. EXECUTION PRICE
- 58. SLIPPAGE CONTROL
- 59. EXECUTION MODES
- 60. ORDER STATE MACHINE
- 61. UNKNOWN EXECUTION STATE
- 62. COMMAND IDEMPOTENCY
- 63. TRADE TRACEABILITY
- 64. POSITION MANAGER
- 65. BREAKEVEN MODEL
- 66. ATR TRAILING STOP
- 67. STRUCTURAL TRAILING
- 68. PARTIAL TAKE PROFIT
- 69. POSITION EXIT CONDITIONS
- 70. STRUCTURAL INVALIDATION
- 71. CIRCUIT BREAKER ARCHITECTURE
- 72. NORMAL
- 73. DEGRADED


# APPENDIX B — CANONICAL SOURCE-TO-MASTER MAPPING


```text
M1/M15 mathematical foundation
    -> measurement, normalization, regime, signal, risk, execution-cost concepts

M1-M5 execution master
    -> tick quality, microstructure, execution context, market quality, low-latency state

M30 master
    -> structural M30 context and deterministic lower-timeframe separation

H1 master
    -> directional authority separation, data integrity, no-lookahead, broker-aware execution

H4 master
    -> structural authority, regime engine, H4 close event, strategy eligibility, execution gates

D1 master
    -> higher structural context, risk, reconciliation, testing, promotion gates

W1 master
    -> long-horizon structure, three-clock model, risk, execution, promotion evidence

MN1 master
    -> macro regime, PIT macro architecture, secular context, research governance

M1 consolidated mathematical master
    -> canonical mathematical foundation, score/risk/execution cost separation
```

The master reference deliberately turns these into one project-wide architecture rather than nine competing strategy specifications.

---

# 42. SELF-EVOLUTION RESEARCH RECORD — PRESERVED SOURCE MATERIAL

The following is the full research record used to derive the evolution architecture. It is preserved as source-derived material; architectural decisions in this master are the integrated synthesis.

# Executive Summary

النتيجة المركزية للبحث هي:

> **أفضل Architecture ليست نظامًا يستطيع تعديل نفسه، بل نظامًا يستطيع تطوير "نسخ مرشحة" لنفسه داخل بيئة بحث معزولة، وإثبات أو تفنيد الفرضيات تجريبيًا، ثم تقديم الأدلة إلى بوابة حوكمة بشرية؛ بينما يبقى Production immutable من منظور الـResearch Engine.**

هذه النتيجة تظهر من تقاطع عدة خطوط بحثية:

- **AlphaEvolve / FunSearch** أثبتا أن LLM + evolutionary search + deterministic evaluators يمكن أن يولد نسخًا برمجية ويطورها تكراريًا. AlphaEvolve يستخدم قاعدة برامج وتقييمًا آليًا ويُنشئ تطورًا عبر اختيار الحلول الواعدة.
- **Darwin Gödel Machine** يذهب خطوة أعمق: النظام يعدّل كوده، يحافظ على archive شجري من النسخ، ويستكشف عدة مسارات تطورية بالتوازي؛ لكنه بحثي وتجريبي، وليس نموذجًا جاهزًا لمنح نظام إنتاجي سلطة ذاتية.
- **The AI Scientist / AI Scientist-v2 / AI co-scientist** تنقل الفكرة من "تحسين parameter" إلى دورة بحثية كاملة: فرضيات → تجارب → تحليل → مراجعة، مع بحث تفرعي وتطور للأفكار.
- **TFX/TFMA** و**Kayenta/Spinnaker** يقدمان الجانب الذي ينقص كثيرًا من أنظمة self-evolution: مقارنة candidate مع baseline، gates، canary، manual judgment، promotion/abort/rollback.
- **MLflow / ML Metadata / W3C PROV / SLSA** تقدم أساسًا عمليًا للـlineage والـprovenance وربط artifact بالـexecution والـdataset والـversion.
- **PBO / DSR / White Reality Check / SPA / CPCV** تجعل مشكلة "النظام جرّب كثيرًا حتى وجد نتيجة جميلة" مشكلة إحصائية صريحة، وليست مجرد تحذير عام من overfitting.
- والأهم: **Reusable Holdout** يبين أن الـholdout نفسه يمكن أن يتعرض للـoverfitting عندما تصبح عملية البحث adaptive وتستخدم نتائج اختبارات سابقة لتقرير الاختبار التالي. هذه النقطة شديدة الأهمية لنظام self-evolution.
- أبحاث 2026 حول **reward hacking** توضح أن agent قد يحقق score مرتفعًا باستغلال الـevaluator أو البيئة بدل تحقيق الهدف الحقيقي؛ لذلك الـevaluator نفسه يجب أن يكون جزءًا من الـsecurity boundary.

إذًا التصميم المقترح ليس:

`AI → يغير نفسه → Production`

بل:

`Production → Observation → Failure → Diagnosis → Hypothesis → Candidate → Sandbox → Validation Firewall → Robustness → Human Gate → Shadow → Human-approved Promotion → Monitoring → Learning`

وهذا الفرق هو جوهر المشروع.

---

# 1. Existing Research

## 1.1 أقرب الأنظمة الموجودة

### AlphaEvolve

AlphaEvolve هو أقرب مثال عملي لفكرة:

**generate → evaluate → retain → evolve**

يستخدم نماذج LLM لتوليد برامج، ثم evaluators آليين لقياس الصحة/الجودة، ثم يحتفظ بالبرامج في قاعدة ويستخدم البرامج الواعدة في الأجيال التالية. DeepMind تصفه كنظام يجمع إبداع LLM مع evolutionary framework وautomated evaluators.

**ما نأخذه:**

`Candidate Generator + Evaluator + Program Archive + Evolutionary Selection`

**ما لا نأخذه كما هو:**

عدم إعطاء candidate حق تغيير Production أو evaluator نفسه.

---

### FunSearch

FunSearch أكثر أهمية مما يبدو، لأنه يفصل بوضوح بين:

**Generator**

و

**Evaluator**

النظام يولد برامج، لكن البرنامج لا يصبح "مكتشفًا ناجحًا" لأنه مقنع للـLLM؛ بل لأنه يجتاز evaluator. كما يستخدم islands للمحافظة على diversity بدل الانجراف إلى فرع واحد.

هذه الفكرة تصلح مباشرة لـ:

`Hypothesis/Candidate Population`

بدل Candidate واحد يتحسن بطريقة greedy.

**فكرة مهمة جدًا:**

لا تجعل النظام يحتفظ فقط بـ"أفضل Candidate".

احتفظ بـ**population of diverse candidates**.

هذا يمنع collapse إلى منطقة بحث واحدة.

---

### Darwin Gödel Machine

DGM يمثل أقرب شيء إلى "self-evolution الحقيقي" وجدته.

النظام:

1. يختار agent موجودًا من archive.
2. ينشئ نسخة جديدة.
3. يعدل الكود.
4. يشغل benchmarks.
5. يحتفظ بالنسخ الناجحة.
6. يبني شجرة تطور متعددة المسارات.

الدراسة أظهرت تحسينًا كبيرًا على SWE-bench وPolyglot، وتذكر صراحة استخدام sandboxing وhuman oversight كاحتياطات.

**القيمة المعمارية لنا:**

بدل:

`A → B → C`

نستخدم:

`A → {B, C}`

ثم:

`B → {D, E}`

وهكذا.

هذا هو أصل فكرة **Evolution Graph** في مشروعك.

لكن DGM أيضًا يوضح الخطر:

> كلما أصبح النظام قادرًا على تعديل نفسه، يصبح أيضًا قادرًا على تعديل **طريقة تعديله لنفسه**.

وهنا تبدأ طبقة **Meta-Evolution**.

---

### The AI Scientist

The AI Scientist ينفذ تقريبًا:

`Idea → Code → Experiment → Results → Paper → Review`

وهو من أوضح الأمثلة على تحويل البحث العلمي إلى pipeline قابل للأتمتة. المشروع نفسه يوضح أنه ينفذ LLM-written code ولذلك يحتاج sandboxing وتقييدًا للـweb/process access.

نأخذ منه:

- automated idea generation
- experiment execution
- result analysis
- automated review
- iterative research

ولا نأخذ منه فكرة "fully autonomous production".

---

### AI Scientist-v2

الإصدار الثاني أضاف **progressive agentic tree search** بدل مسار بحث خطي، واستخدم Experiment Manager لإدارة الفروع التجريبية. والأهم أن repo نفسه يذكر أن النسخة الأوسع استكشافًا قد تكون أقل نجاحًا من النسخة المقيدة بقالب جيد؛ أي أن **اتساع الاستكشاف ليس مجانيًا**.

وهذه نقطة مباشرة لمشروعك:

> Search space أوسع ≠ نظام أفضل.

كل توسع في search space يرفع statistical burden.

---

### AI co-scientist

Google تصف النظام كـmulti-agent scientific collaborator يستخدم:

`generate → debate → evolve`

لتطوير hypotheses، مع tournament evolution للأفكار.

أهم شيء نأخذه منه:

بدل أن يكون:

`Hypothesis Engine = LLM واحد`

يمكن أن يكون:

`Generator Agents`
→ `Critic`
→ `Evidence Agent`
→ `Alternative Hypothesis Agent`
→ `Tournament`
→ `Experiment Planner`

وهذا أفضل بكثير من جعل النموذج يقترح تعديلًا رقميًا مباشرًا.

---

# 2. Existing Systems

| النظامآلية التطورما يغيره تلقائيًاHuman roleValidationما نأخذه |                                  |                         |                          |                                  |                          |
| -------------------------------------------------------------- | -------------------------------- | ----------------------- | ------------------------ | -------------------------------- | ------------------------ |
| AlphaEvolve                                                    | evolutionary code search         | programs/algorithms     | بحثي                     | automated evaluators             | evaluator + archive      |
| FunSearch                                                      | program evolution + islands      | functions/programs      | domain feedback          | systematic evaluator             | diversity                |
| DGM                                                            | self-modifying agents            | agent code              | oversight/sandbox        | coding benchmarks                | evolution tree           |
| AI Scientist                                                   | scientific loop                  | experiments/code/papers | محدود/بحثي               | reviewer + experiments           | full research loop       |
| AI Scientist-v2                                                | tree search                      | hypotheses/code         | بحثي                     | experiment manager + reviewer    | branch-based research    |
| AI co-scientist                                                | multi-agent hypothesis evolution | hypotheses              | scientist guidance       | evidence/experimental validation | debate + tournament      |
| AutoML-Zero                                                    | evolutionary search              | ML algorithms           | experiment setup         | benchmark                        | structural evolution     |
| TFX/TFMA                                                       | candidate/baseline validation    | model versions          | engineering governance   | threshold gates                  | promotion gate           |
| Kayenta                                                        | baseline vs canary               | release                 | manual judgment possible | statistical canary               | shadow/canary            |
| MLflow/MLMD                                                    | lifecycle/lineage                | versions/artifacts      | governance               | recorded metadata                | registry + lineage       |
| SRE Postmortems                                                | incident learning                | processes/fixes         | human ownership          | postmortem review                | failure memory           |
| CausalRCA / MicroRCA                                           | diagnosis                        | root-cause hypotheses   | operator                 | telemetry/causal analysis        | Diagnose before Optimize |

المصادر: AlphaEvolve/FunSearch/DGM ، AI Scientist systems ، TFX/Kayenta ، MLflow/MLMD ، RCA .

---

# 3. Self-Evolution Approaches

## A. Parameter Optimization

مثال:

`threshold 0.70 → 0.65`

هذه أقل طبقات التطور تعقيدًا.

**مزايا:**

- سهلة التقييم
- search space محدد
- سهلة rollback

**المخاطر:**

- massive multiple testing
- local optimization
- قد تعالج symptom لا cause

**الحكم:**

ضرورية كبداية، لكنها لا تمثل Self-Evolution الكامل.

---

## B. Rule Evolution

تغيير:

`IF condition A AND B THEN action C`

هذه أكثر خطورة لأن النظام يغير logic.

يجب أن تدخل تحت:

`Rule Schema + Static Validation + Regression Suite`

ولا تسمح للنظام بتغيير safety/risk rules.

---

## C. Feature Evolution

النظام يكتشف أو يولد feature جديدة.

المشكلة هنا ليست فقط overfitting؛ بل **data leakage**.

أي feature جديدة يجب أن تكون مرتبطة بـ:

`feature_version`
\+
`data lineage`
\+
`availability timestamp`
\+
`transformation version`

---

## D. Strategy Evolution

هنا يتغير structure نفسه.

هذا أول مستوى يصبح فيه مفهوم `Candidate Strategy` مهمًا فعليًا.

نحتاج:

`Parent Strategy`
→ `Mutation`
→ `Candidate`

وليس:

`LLM writes new strategy`

---

## E. Architecture Evolution

تغيير بنية النظام.

مثل:

`single model`
→ `ensemble`

أو:

`rule engine`
→ `rule + model + regime router`

هذه الطبقة أخطر لأن architecture change قد يغير behavior كله.

لذلك:

**Architecture Evolution لا يدخل Production في النسخة الأولى إطلاقًا.**

---

## F. Research-Method Evolution

هذه فكرة قوية جدًا ومهملة غالبًا.

النظام لا يقول:

> "غيّر strategy"

بل:

> "طريقة اختبار هذه الفرضيات غير كافية."

مثلاً:

`Backtest method A`

قد يقترح النظام:

`validation method B`

لكن هنا يظهر خطر خطير:

> النظام قد يغيّر evaluator لكي يسجل نتيجة أفضل.

لذلك Research-Method Evolution يجب أن يكون أبطأ وأكثر تقييدًا من Strategy Evolution.

---

## G. Meta-Evolution

أعلى مستوى:

النظام يطور:

`طريقة توليد الفرضيات`

أو:

`طريقة اختيار التجارب`

أو:

`طريقة توزيع compute`

أو:

`طريقة البحث نفسها`

وهنا يصبح النظام قادرًا على تطوير **optimizer of optimizer**.

هذه المنطقة ما زالت بحثية أكثر من كونها production engineering.

---

# 4. الترتيب المقترح

الترتيب العملي:

`Parameter`
↓
`Rule / Feature`
↓
`Strategy`
↓
`Research Method`
↓
`Architecture`
↓
`Meta-Evolution`

لكن يوجد بعد ثانٍ مستقل تمامًا:

> **Evolution Depth ≠ Permission Level**

وهذه من أهم توصيات البحث.

لا تقل:

> Level 5 يعني يستطيع تعديل architecture.

بل افصل:

**Axis 1 — ماذا يستطيع تطويره؟**

و

**Axis 2 — أين يستطيع تنفيذ ذلك؟**

---

# 5. Human Governance

مصطلحات HITL / HOTL / Human-in-Command ليست معيارًا واحدًا متفقًا عليه تمامًا؛ لذلك الأفضل تحويلها إلى تعريفات تشغيلية داخل architecture.

أعمال Parasuraman/Sheridan/Wickens تقترح النظر إلى automation عبر وظائف مثل information acquisition, analysis, decision/action selection, action implementation، وعلى مستويات automation مختلفة.

كما أن NIST AI RMF يجعل **Govern** وظيفة مستمرة عبر دورة حياة النظام، ويضع Govern/Map/Measure/Manage ضمن إطار متكرر لإدارة المخاطر.

## Human-in-the-Loop

الإنسان يشارك داخل العملية قبل التنفيذ أو أثناءه.

مثال:

`Candidate → Human Approve → Deploy`

---

## Human-on-the-Loop

النظام يعمل تلقائيًا، والإنسان يراقب ويستطيع التدخل.

مثال:

`Monitoring → Autonomous rollback → Human review`

---

## Human-in-Command

الإنسان يملك:

- الهدف
- السياسات
- حدود الصلاحيات
- authority

بينما النظام يدير التفاصيل داخل الحدود.

---

## النموذج الذي أوصي به

ليس HITL دائمًا.

بل:

### Human-in-the-Command

للحوكمة العليا.

### Human-on-the-Loop

للتشغيل المراقب.

### Human-in-the-Loop

للتغييرات عالية الأثر.

وهذا متوافق مع فكرة meaningful human control التي تربط التحكم بالقدرة الفعلية على التدخل وتتبع المسؤولية، لا بمجرد وجود شخص "في الغرفة".

---

# 6. ماذا يفعل النظام بدون موافقة؟

## مسموح

- Observation
- anomaly detection
- degradation detection
- telemetry analysis
- Root Cause hypotheses
- hypothesis generation
- experiment planning
- sandbox experiments
- backtests
- stress tests
- regression tests
- candidate generation
- candidate rejection
- candidate clustering
- duplicate experiment detection
- automatic ranking
- lineage construction
- failure memory
- automatic rollback إلى version مصادق عليها مسبقًا عند invariant violation

## يحتاج Approval

- اعتماد Strategy
- تغيير Risk Rules
- تغيير evaluator
- تغيير datasets الأساسية
- تغيير data source
- تغيير Production configuration
- Promotion
- رفع صلاحيات candidate
- Architecture change
- Research-method change الذي يؤثر على final validation
- تعديل metric definitions

NIST يدعم فكرة وجود سياسات وإدارة مخاطر مستمرة وتوثيق قرارات المعالجة، بينما TFX/Kayenta يقدمان عمليًا نموذجًا يكون فيه evaluation gate قبل promotion، مع إمكانية وجود human judgment قبل التوسع.

---

# 7. Autonomy Levels

التقسيم الأولي 0–6 جيد، لكن أرى أنه يحتاج إعادة صياغة.

## L0 — Observation

Read-only.

لا تعديل ولا تجارب.

---

## L1 — Diagnosis

يستطيع:

`Observe → Detect → Diagnose`

لكن لا يولد تغييرًا.

---

## L2 — Research Sandbox

يستطيع إنشاء hypotheses وتشغيل experiments.

لا يستطيع لمس Candidate production state.

---

## L3 — Candidate Evolution

يستطيع إنشاء:

`Candidate A`
`Candidate B`
`Candidate C`

ضمن search space محدد.

---

## L4 — Autonomous Validation

يمكنه:

- run validation
- stress
- regression
- reject
- rank

لكن لا يمكنه promotion.

---

## L5 — Shadow

يعمل candidate بالتوازي مع control.

لا authority لتنفيذ action حقيقي.

---

## L6 — Human-Gated Promotion

الإنسان يقرر:

`Promote / Reject`

---

## L7 — Guarded Runtime Recovery

بعد promotion:

يمكن للنظام تنفيذ rollback تلقائيًا **فقط إلى version known-good ومصرح بها مسبقًا** عند invariant violation.

هذا ليس "self-evolution".

بل:

**self-preservation.**

---

## L8 — Meta-Evolution

يُسمح له بتجريب تحسينات على:

- hypothesis generator
- experiment planner
- candidate generator
- search policy

لكن:

**Research Lab only.**

ليس Production.

---

# 8. Evolution Loop

الـloop الأصلي عندك قوي، لكن أضيف مراحل:

```text
OBSERVE
   ↓
DETECT
   ↓
CLASSIFY
   ↓
DIAGNOSE
   ↓
ROOT-CAUSE HYPOTHESES
   ↓
HYPOTHESIS FORMULATION
   ↓
EXPERIMENT DESIGN
   ↓
EXPERIMENT BUDGET CHECK
   ↓
CANDIDATE GENERATION
   ↓
SANDBOX EXECUTION
   ↓
DATA-LEAKAGE CHECK
   ↓
VALIDATION FIREWALL
   ↓
ROBUSTNESS / STRESS
   ↓
CONTROL vs CANDIDATE
   ↓
REGRESSION
   ↓
STATISTICAL CORRECTION
   ↓
CANDIDATE REVIEW
   ↓
HUMAN GATE
   ↓
SHADOW
   ↓
PROMOTION GATE
   ↓
CONTROLLED DEPLOYMENT
   ↓
MONITORING
   ↓
INCIDENT / ROLLBACK
   ↓
POSTMORTEM
   ↓
FAILURE MEMORY
   ↓
LEARN
   ↓
OBSERVE

```

---

# 9. لماذا Diagnose قبل Optimize؟

أبحاث RCA توضح أن anomaly detection وroot-cause localization يمكن ربطهما باستخدام logs وmetrics وtraces وcausal structures بدل الاكتفاء برؤية symptom. CausalRCA مثلًا يستخدم causal structure لتحديد root-cause metrics، مع وجود حدود منهجية يجب عدم تجاهلها.

والنقطة المهمة:

`Performance ↓`

ليس معناها:

`change parameter`

قد يكون السبب:

`Data Failure`

أو:

`Distribution Shift`

أو:

`Execution Failure`

أو:

`Latency Failure`

أو:

`Regime Failure`

أو:

`Feature Failure`

لذلك:

> **Failure → Root Cause → Hypothesis**

يجب أن يسبق:

> **Parameter Search**

---

# 10. Hypothesis Engine

أقترح أن كل hypothesis تتبع schema ثابتًا:

```text
Hypothesis ID
Observed Failure
Context / Regime
Root Cause Belief
Mechanism
Prediction
Proposed Change
Expected Direction
Primary Metric
Secondary Metrics
Safety Constraints
Falsification Condition
Experiment Design
Required Data
Budget

```

مثال:

```text
OBSERVATION:
Performance degradation in regime X

HYPOTHESIS:
The degradation is caused by dependency on feature X
whose information quality deteriorates in regime X.

PREDICTION:
Reducing dependency on X in regime X will improve
robustness without materially degrading behavior elsewhere.

EXPERIMENT:
Compare Control vs Candidate under identical conditions.

```

هذه أفضل بكثير من:

```text
threshold = 0.65

```

لأنها تجعل النظام **يختبر تفسيرًا**، وليس مجرد رقم.

---

# 11. Automated Experimentation

Katib وAutoML وغيرها تظهر أن experiment orchestration يمكن أن يصبح آليًا، مع search algorithms متعددة مثل Bayesian optimization وTPE وCMA-ES وHyperBand وPBT.

لكن لمشروعك لا أوصي بـ:

`unbounded optimization`

بل:

```text
Hypothesis
    ↓
Define Search Space
    ↓
Budget Allocation
    ↓
Trials
    ↓
Early Reject
    ↓
Validation

```

والـSearch Space نفسه يجب أن يكون versioned.

---

# 12. Critical Discovery: Evolution Budget

الـbudget يجب ألا يكون مجرد CPU/GPU.

يجب أن يكون هناك أربع ميزانيات:

### Compute Budget

كم compute؟

### Experiment Budget

كم تجربة؟

### Statistical Budget

كم adaptive decisions؟

### Evidence Budget

كم مرة يستطيع النظام الاقتراب من datasets الحساسة؟

يمكن أيضًا إضافة:

### Structural Budget

كم mutation architecture/rule مسموح؟

---

## لماذا؟

إذا سمحت للنظام:

`100,000 trials`

فحتى لو لم يكن optimizer "سيئًا"، يصبح احتمال العثور على نتيجة تبدو ممتازة بالصدفة أكبر.

DSR يربط selection bias بعدد trials، وPBO صمم تحديدًا لدراسة احتمال أن تكون أفضل strategy ناتجة عن overfitting للبحث التاريخي.

إذن:

> **Number of trials يجب أن يصبح جزءًا من Evidence.**

وليس مجرد رقم تقني مخفي.

---

# 13. Validation & Overfitting Protection

هذه أهم طبقة في المشروع.

## White Reality Check

White أوضح أن data snooping يحدث عندما تستخدم نفس البيانات أكثر من مرة للاستدلال أو اختيار النموذج، وأن النتيجة الممتازة قد تكون مجرد أثر لإعادة استخدام البيانات.

---

## Superior Predictive Ability

Hansen قدم SPA كاختبار للتعامل مع model-selection/data-snooping في مجموعة من النماذج.

---

## PBO / CSCV

Bailey et al. قدموا Probability of Backtest Overfitting واستخدموا CSCV لتقدير احتمال أن أفضل نتيجة in-sample لا تمثل out-of-sample.

---

## Deflated Sharpe Ratio

DSR صمم لتصحيح selection bias تحت multiple testing بالإضافة إلى non-normality.

---

## Purged CV + Embargo

في financial ML، K-fold التقليدي قد يكون غير مناسب بسبب overlap والاعتماد الزمني. أدبيات López de Prado تقترح purging وembargo كآليتين للتحكم في أنواع محددة من leakage، وهناك بحث تجريبي أحدث وجد نتائج قوية لـCPCV في بيئات صناعية محكومة.

---

# 14. لا تستخدم تقنية واحدة فقط

الـValidation Firewall عندي يجب أن يكون:

```text
Candidate
   ↓
Leakage Tests
   ↓
Purged / Time-Aware Validation
   ↓
Walk-Forward
   ↓
CPCV / CSCV
   ↓
PBO
   ↓
DSR
   ↓
SPA / Reality Check where applicable
   ↓
Stress / Monte Carlo
   ↓
Locked OOS
   ↓
Human Review

```

ليس كل اختبار يصلح لكل candidate.

الـValidation Planner يحدد protocol مسبقًا.

وهذا مهم جدًا:

> **Candidate لا يختار الاختبار الذي يناسب نتيجته.**

---

# 15. Evidence Firewall

هذه من أقوى أفكار المشروع.

أقترح أربع طبقات:

```text
E0 — Exploration
        ↓
E1 — Development / Validation
        ↓
E2 — Out-of-Sample
        ↓
E3 — Locked Holdout
        ↓
HUMAN

```

لكن هناك تعديل مهم:

## الـFinal Holdout لا يكون ملفًا

بل:

> **Validation Service**

النظام لا يحصل على data.

يرسل:

```text
Candidate Hash
Experiment Definition
Code Artifact Hash
Evaluator Version

```

والـservice ينفذ الاختبار في بيئة معزولة ويعيد:

```text
Approved Metrics Only

```

---

# 16. لماذا هذا مهم؟

لأن reusable-holdout literature تثبت أن analyst يمكن أن يبدأ overfitting للـholdout نفسه عندما يجعل نتيجة الاختبار جزءًا من قرار الاختبار القادم.

لذلك أقترح:

### Rule A

Candidate engine لا يرى raw holdout.

### Rule B

Holdout Service تعيد نتائج pre-defined فقط.

### Rule C

كل access له سجل immutable.

### Rule D

هناك Query Budget.

### Rule E

بعد الوصول إلى Final Holdout:

`Candidate = contaminated`

ولا يسمح له بالعودة إلى optimization loop على نفس الـholdout.

### Rule F

أي إعادة استخدام للتقييم النهائي تبدأ Evidence Cycle جديدة.

### Rule G

الإنسان نفسه عندما يرى Final Holdout لا يرسل الأرقام back إلى Research Engine.

---

# 17. Reusable Holdout — فكرة مستقبلية

أبحاث Dwork وغيرها تقدم طرقًا نظرية لاستخدام holdout في adaptive analysis باستخدام أفكار من differential privacy، بهدف الحفاظ على generalization تحت إعادة الاستخدام التكيفي.

لكنني **لا أوصي** بأن تجعل هذا أساس الإصدار الأول.

الأفضل:

`One-shot / limited-access Holdout`

ثم لاحقًا:

`Privacy-preserving adaptive holdout`

كمسار بحثي مستقل.

---

# 18. Control vs Candidate

هذه النقطة توجد لها analogues قوية في production ML.

TFX Evaluator يستطيع مقارنة:

`candidate`

مقابل:

`baseline`

واستخدام thresholds لتحديد ما إذا كان candidate "good enough".

نحوّل ذلك إلى:

```text
CONTROL
Current Approved Version

vs

CANDIDATE
New Version

```

مع:

- نفس data slice
- نفس execution assumptions
- نفس evaluator
- نفس environment
- نفس time window
- نفس transaction assumptions
- same random seeds حيث يلزم

ثم نحسب:

```text
Absolute Difference
Relative Difference
Regression Set
Regime-specific Difference
Tail Behavior
Robustness Difference

```

---

# 19. لا تسمح بـSingle Metric Optimization

Reward hacking research في 2026 مهم جدًا هنا.

RHB وجد حالات يستطيع فيها agent استغلال طريقة التقييم بدل حل المهمة، وأبحاث أخرى وجدت أن reward hacking قد يظهر حتى عندما يبدو الـreward مرتفعًا.

إذًا:

```text
Score = 0.7 * metricA + 0.3 * metricB

```

ليس كافيًا.

أوصي بـ:

```text
HARD SAFETY CONSTRAINTS
        +
STATISTICAL VALIDITY
        +
ROBUSTNESS
        +
QUALITY OBJECTIVES

```

أي:

> **Constraints are vetoes. Metrics are optimizers.**

---

# 20. Evaluator Firewall

هذه فكرة جديدة مهمة للمشروع.

Candidate لا يجوز له تعديل:

- evaluator
- metric definitions
- validation code
- holdout service
- experiment budget
- audit logger

بل يجب أن تكون هذه في:

```text
TRUSTED CONTROL PLANE

```

والـcandidate في:

```text
UNTRUSTED RESEARCH PLANE

```

هذا يشبه الفصل بين build/provenance والartifact في SLSA، حيث يتم تتبع كيف ومتى وبأي inputs تم إنتاج artifact.

---

# 21. Candidate Version

الschema المقترح:

```text
Candidate ID
Parent Version
Parent Hash

Campaign ID
Hypothesis ID
Failure ID
RCA ID

Reason
Change Type
Changed Components

Dataset Version(s)
Feature Version(s)
Parameter Set

Code Commit
Environment Image
Dependency Lock

Experiment Definition
Search Space
Trial Number
Random Seed Policy

Evaluator Version
Metric Registry Version

Control Version

Validation Results
OOS Results
Stress Results
Regression Results

PBO / DSR / Statistical Results
Risk Analysis

Known Limitations
Unexpected Effects

Contamination Status

Human Decision
Human Reviewer
Approval Timestamp

Deployment Status
Shadow Status
Promotion Status

Rollback Target
Rollback Procedure

Provenance / Artifact Digest

```

وهذا قريب جدًا من فلسفة MLMD التي تسجل:

`Artifacts + Executions + Contexts + Events`

وتتيح تتبع أي artifact إلى الـexecutions والinputs التي أنتجته.

---

# 22. Experiment Ledger

Experiment Ledger يجب أن يسجل **الفشل قبل النجاح**.

```text
Experiment ID
Timestamp
Actor
Campaign

Parent Version
Hypothesis
RCA

Dataset
Feature Set
Parameters
Search Space
Number of Trials

Control
Candidate

Metrics
Validation
Stress
Decision

Failure Reason
Rejection Reason
Unexpected Effects

Evaluator Version
Code Version
Environment Version
Seed

Holdout Exposure

```

---

# 23. منع إعادة التجارب

أنشئ:

## Experiment Fingerprint

مثل:

```text
Hash(
  Parent
  +
  Hypothesis
  +
  Dataset
  +
  Features
  +
  Search Space
  +
  Code
  +
  Evaluator
  +
  Environment
)

```

إذا تكرر:

`Exact Duplicate`

يُمنع.

وإذا كان قريبًا:

`Semantic Duplicate`

يظهر تحذير:

> "This experiment is materially similar to experiments E-192, E-241 and E-377."

لكن ليس بالضرورة reject آليًا؛ قد يكون التكرار العلمي المقصود.

---

# 24. Evolution Memory

الذاكرة ليست:

> "أفضل Strategy"

بل:

### Positive Memory

ما الذي نجح؟

### Negative Memory

ما الذي فشل؟

### Causal Memory

لماذا فشل؟

### Search Memory

ما الذي جُرّب؟

### Evidence Memory

ما الذي ثبت وما الذي لم يثبت؟

### Contamination Memory

أي datasets/candidates أصبحت غير صالحة لإعادة التقييم؟

---

# 25. Rollback + Learning

Google SRE يقدم نموذجًا قويًا للـpostmortem: root cause، timeline، impact، lessons learned، action items، owner، tracking، ثم تخزين النتائج بحيث يمكن التعلم من الحوادث السابقة. كما يشير إلى قيمة وجود metadata machine-readable.

لذلك:

```text
Candidate Failed
      ↓
Rollback
      ↓
Incident Created
      ↓
Root Cause
      ↓
Failure Pattern
      ↓
Knowledge Update
      ↓
Future Experiment Filter

```

لكن **لا تمسح candidate**.

---

# 26. Failure Memory

كل failure يجب أن يصبح object:

```text
FailurePattern
{
    type
    context
    trigger
    evidence
    root_cause_hypotheses
    confirmed_root_cause
    false_hypotheses
    affected_versions
    rejected_changes
    recurrence_count
    prevention_actions
}

```

وهذه من أقوى النقاط التي يمكن أخذها من SRE.

Postmortem الجيد لا ينتهي عند:

> "عرفنا ماذا حصل."

بل:

> "ماذا يجب أن يتغير حتى لا يتكرر؟"

---

# 27. Evolution Graph

استخدم Graph حقيقي:

```text
                    A
                  /   \
                 B     C
               /  \     \
              D    E     F
                   \
                    G

```

لكن لا تضع فقط versions.

ضع:

```text
VERSION
  ↓
HYPOTHESIS
  ↓
EXPERIMENT
  ↓
RESULT
  ↓
DECISION

```

وبالتالي graph يصبح:

```text
Version
   ↓
Hypothesis
   ↓
Experiment
   ↓
Candidate
   ↓
Validation
   ↓
Decision
   ↓
Version

```

وهذا أقوى من مجرد Git history.

W3C PROV يعطي نموذجًا عامًا للـprovenance والعلاقات بين entities/activities، بينما MLMD يطبق graph-like lineage عمليًا في ML workflows.

---

# 28. Version Graph + Evidence Graph

أوصي بوجود graphين:

## Graph 1 — Evolution Graph

من ماذا نشأ ماذا؟

## Graph 2 — Evidence Graph

ما الدليل الذي يبرر كل transition؟

مثال:

```text
Version B
   |
   +-- Hypothesis H19
           |
           +-- Experiment E44
                    |
                    +-- Validation V7
                    |
                    +-- OOS O3
                    |
                    +-- Human Decision D12

```

هذا يجعل كل deployment قابلًا لإعادة البناء تاريخيًا.

---

# 29. Shadow Deployment

Google وNetflix وSpinnaker جميعها تستخدم canary/progressive delivery، وKayenta يقارن baseline مع canary ويبحث عن degradation.

TFX أيضًا يقدم online validation بعد offline validation، ويمكن استخدام canary/A-B قبل تعميم version جديدة.

لكن في مشروعك يجب أن يكون:

```text
CURRENT VERSION
        +
CANDIDATE VERSION
        ↓
Parallel Evaluation

```

مع فصل كامل عن execution authority.

---

# 30. Shadow ليس Canary

هذه نقطة مهمة.

### Shadow

Candidate يرى نفس المدخلات لكنه لا يملك authority.

### Canary

Candidate يحصل على جزء من production exposure.

في نظام حساس:

```text
Research
→ Shadow
→ Human Approval
→ Restricted Canary
→ Human Decision

```

أحسن من:

```text
Research
→ Canary

```

لأن الـcandidate لم يثبت بعد امتلاكه robustness كافية.

---

# 31. Rollback

Argo Rollouts وSpinnaker يوضحان عمليًا كيف يمكن ربط analysis بالpromotion والـabort والـrollback، وحتى الاحتفاظ بنوافذ rollback سريعة.

نأخذ:

```text
Current Stable Version
New Candidate
      ↓
Canary
      ↓
Analysis
      ↓
PASS → continue
FAIL → Abort
      ↓
Stable

```

لكن نضيف:

```text
Rollback
→ Incident
→ RCA
→ Failure Memory

```

---

# 32. Forbidden Behaviors

هذه يجب أن تكون enforced خارج الـagent نفسه.

### Absolute Forbidden

1. Self-modifying Production Code
2. Direct production writes from Research Engine
3. Changing evaluator
4. Changing metric definitions during an experiment
5. Deleting failed experiments
6. Editing historical results
7. Accessing raw locked holdout
8. Re-running after seeing holdout result
9. Unlimited experimentation
10. Future-information access
11. Changing timestamps
12. Hidden external data access
13. Unauthorized data-source changes
14. Modifying risk constraints
15. Bypassing approval gates
16. Creating hidden side channels to evaluator
17. Optimizing evaluator instead of objective
18. Suppressing error signals
19. Auto-promoting its own successor
20. Writing itself new credentials/permissions
21. Changing the logging system that records its actions
22. Altering the rollback target
23. Using a candidate to evaluate itself through a mutable evaluator
24. Circular changes where candidate changes the metric that declares it successful

أبحاث reward hacking الحديثة تجعل البنود 16–17–23–24 ليست أفكارًا نظرية فقط؛ exploit evaluation itself أصبح موضوعًا تجريبيًا مباشرًا.

---

# 33. Recommended Architecture

```text
                        ┌──────────────────────────┐
                        │     HUMAN GOVERNANCE     │
                        │ Policies / Approvals     │
                        │ Risk Constraints        │
                        └────────────┬─────────────┘
                                     │
                              Approval / Reject
                                     │
┌────────────────────────────────────▼─────────────────────────────────┐
│                        GOVERNANCE / CONTROL PLANE                     │
│                                                                      │
│ Policy Engine │ Approval Gate │ Metric Registry │ Evaluator Registry │
│ Version Registry │ Audit Ledger │ Evidence Firewall │ Budget Manager │
└───────────────┬───────────────────────────────┬──────────────────────┘
                │                               │
                │                               │
        ┌───────▼───────┐               ┌───────▼────────┐
        │ RESEARCH PLANE│               │ RUNTIME PLANE  │
        │               │               │                │
        │ Observation   │               │ Production     │
        │ Detection     │               │ Control        │
        │ RCA           │               │ Monitoring     │
        │ Hypothesis    │               │ Shadow         │
        │ Experiments   │               │ Canary         │
        │ Candidate Gen │               │ Rollback       │
        └───────┬───────┘               └───────┬────────┘
                │                               │
                └──────────────┬────────────────┘
                               │
                       ┌───────▼────────┐
                       │ EVOLUTION GRAPH│
                       │                │
                       │ Versions       │
                       │ Hypotheses     │
                       │ Experiments    │
                       │ Results        │
                       │ Incidents      │
                       └────────────────┘

```

والـflow الداخلي:

```text
OBSERVE
  ↓
DETECT
  ↓
CLASSIFY
  ↓
DIAGNOSE
  ↓
ROOT-CAUSE HYPOTHESIS
  ↓
FALSIFIABLE HYPOTHESIS
  ↓
EXPERIMENT PLANNER
  ↓
BUDGET CHECK
  ↓
CANDIDATE GENERATOR
  ↓
SANDBOX
  ↓
LEAKAGE CHECK
  ↓
VALIDATION FIREWALL
  ↓
ROBUSTNESS
  ↓
CONTROL vs CANDIDATE
  ↓
REGRESSION
  ↓
STATISTICAL VALIDATION
  ↓
HUMAN REVIEW
  ↓
SHADOW
  ↓
PROMOTION GATE
  ↓
PRODUCTION
  ↓
MONITOR
  ↓
INCIDENT / OUTCOME
  ↓
ROLLBACK if required
  ↓
POSTMORTEM
  ↓
FAILURE MEMORY
  ↓
OBSERVE

```

---

# 34. مكونات النظام النهائية

## Observation Engine

يجمع:

- state
- performance
- regime
- execution observations
- system telemetry
- data quality

---

## Failure Detection Engine

يكتشف:

- anomaly
- degradation
- unexpected behavior
- constraint violations

---

## Root Cause Engine

يجمع:

- statistical evidence
- causal evidence
- logs
- features
- regime context

ولا يقدم:

`Root Cause = truth`

بل:

`Root Cause Hypothesis + Confidence + Evidence`

---

## Hypothesis Engine

ينتج hypotheses قابلة للتفنيد.

---

## Experiment Engine

يبني التجارب ويمنع:

- leakage
- duplicate
- budget overrun
- holdout abuse

---

## Candidate Generator

يعمل مثل AlphaEvolve/FunSearch لكن داخل search boundary مقيد.

---

## Validation Firewall

هو أهم component تقريبًا.

يقرر:

`What data?`

`What evaluator?`

`What protocol?`

`What evidence?`

---

## Overfitting Protection

يطبق:

- PBO
- DSR
- CPCV / Purged validation
- OOS
- multiple-testing controls
- trial accounting

بحسب طبيعة التجربة.

---

## Evolution Budget

يحكم:

```text
Compute
Experiments
Trials
Structural Mutations
Holdout Queries
Validation Runs
Branches

```

---

## Human Governance Layer

يحدد:

```text
Who may approve?
What may be approved?
For how long?
For which candidate?
Under what evidence?

```

---

## Approval Gate

يجب أن يكون خارج agent process نفسه.

---

## Shadow Engine

تشغيل parallel مع:

`NO EXECUTION AUTHORITY`

---

## Promotion Gate

يتحقق:

```text
Approved?
Evidence complete?
No forbidden state?
Version signed?
Rollback ready?
Monitoring ready?

```

---

## Rollback Engine

ينفذ فقط إلى:

`Last Known Good Approved Version`

---

## Evolution Memory

يخزن:

`what worked + what failed + why + under what context`

---

## Experiment Ledger

Append-only.

---

## Version Registry

كل production version:

`immutable ID + digest + lineage`

---

## Audit System

يسجل:

`Who / What / Why / When / Evidence / Decision`

---

## Monitoring Engine

لا يراقب performance فقط.

بل:

```text
Performance
Behavior
Data Quality
Distribution Shift
Regime Shift
Execution
Latency
Constraint Violations
Unexpected Effects

```

---

# 35. Experiment Ledger + MLMD + SLSA

أرى أن أفضل implementation philosophy هي دمج ثلاث أفكار:

### MLflow

للـexperiments/runs/metrics/artifacts.

### MLMD / PROV

لـlineage graph.

### SLSA-style provenance

لإثبات كيف تم بناء artifact ومن أين جاء.

ثم نضيف فوقها:

### Evolution Graph

خاص بالمشروع.

---

# 36. Governance Rules المقترحة

بعد مقارنة الأدبيات، أرى أن هذه القواعد تستحق أن تصبح **Core Invariants**:

### RULE 1

Research Engine has no direct Production write access.

### RULE 2

Every change is represented by an immutable Candidate.

### RULE 3

Every Candidate has exactly one explicit parent.

### RULE 4

Every Candidate belongs to a Hypothesis or explicitly documented exploratory campaign.

### RULE 5

Every Experiment is logged.

### RULE 6

Failed experiments are first-class data.

### RULE 7

Evaluator definitions are immutable inside an experiment family.

### RULE 8

Metric definitions are versioned.

### RULE 9

Final Holdout is a separate trust boundary.

### RULE 10

Holdout access is audited and budgeted.

### RULE 11

No candidate may be optimized on a result that it already observed from the locked holdout.

### RULE 12

Number of trials is part of the evidence record.

### RULE 13

Promotion requires explicit human authority.

### RULE 14

Every deployment is reversible.

### RULE 15

Automatic rollback may occur only within predefined safety boundaries.

### RULE 16

Every rollback creates an Incident.

### RULE 17

Every Incident feeds Failure Memory.

### RULE 18

Production may not modify Research history.

### RULE 19

Research may not rewrite historical evidence.

### RULE 20

The evaluator cannot be evolved by the candidate it evaluates.

هذه الأخيرة مهمة جدًا.

---

# 37. Risk Model

يمكن تقسيم المخاطر إلى خمس طبقات:

## Statistical Risk

- overfitting
- multiple testing
- selection bias
- data snooping
- non-stationarity

## Research Risk

- weak hypothesis
- circular experimentation
- confirmation bias
- duplicate search

## Agent Risk

- reward hacking
- tool misuse
- evaluator manipulation
- long-horizon failure

## Operational Risk

- bad deployment
- bad rollback
- monitoring blind spots

## Governance Risk

- unauthorized changes
- missing approval
- unverifiable decisions
- corrupted history

---

# 38. Research Gaps

هذه المنطقة مهمة لأنها توضح أن مشروعك لا يعيد تنفيذ حل موجود فقط.

## Gap 1 — Adaptive Search + Financial Statistical Validity

لدينا أدوات لكل جزء:

`PBO`
`DSR`
`CPCV`
`Multiple Testing`

لكن لا يوجد حل نهائي ناضج لمشكلة:

> continuously adaptive self-evolution over dependent, non-stationary financial data with evolving search spaces.

هذه ما تزال research problem.

---

## Gap 2 — Self-Evolving Validation

هناك أنظمة تستطيع تغيير الحل.

لكن:

> كيف نسمح للنظام بتطوير طريقة الاختبار دون أن يصبح evaluator نفسه جزءًا من reward hacking loop؟

لا يوجد جواب هندسي شامل ومثبت.

---

## Gap 3 — Root Cause + Self-Evolution

هناك أنظمة RCA قوية، وهناك أنظمة evolution قوية.

لكن دمج:

`RCA → hypothesis → experiment → verified evolution`

مع ضمانات إحصائية قوية ما يزال غير ناضج.

---

## Gap 4 — Human Oversight at Scale

عند 10 candidates:

human review ممكن.

عند:

`10,000 candidates`

يصبح الإنسان نفسه bottleneck.

وهنا تظهر الحاجة إلى:

`Human Review Compression`

بحيث لا تعرض للإنسان 10,000 تجربة، بل:

```text
Clusters
Novel Cases
High-Uncertainty Cases
High-Impact Cases
Policy Violations
Evidence Conflicts

```

---

## Gap 5 — Failure Learning Without Leakage

كيف تجعل system يتعلم من:

`Candidate Failed`

من دون أن يتحول failure data إلى leak للـvalidation?

هذه مشكلة حقيقية.

---

## Gap 6 — Meta-Evolution Safety

إذا سمحت للsystem بتحسين:

`Hypothesis Generator`

ثم:

`Experiment Generator`

ثم:

`Evaluator Strategy`

فأنت تنتقل من self-evolution إلى:

> **self-referential research system**

وهو أكثر بكثير من مجرد AutoML.

DGM وSOAR وAI Scientist تعطي أجزاء مهمة من الصورة، لكن لا يوجد standard production blueprint نهائي لهذا المستوى.

---

# 39. ما الذي يجب أخذه مباشرة؟

## TAKE

### 1. AlphaEvolve

`LLM + evaluator + evolutionary archive`

### 2. FunSearch

`islands + diverse population + evaluator guardian`

### 3. DGM

`evolution tree + archive + multiple paths`

### 4. AI Scientist

`full research loop`

### 5. AI co-scientist

`generate → debate → evolve`

### 6. TFX

`candidate vs baseline + validation gate`

### 7. Kayenta

`baseline vs canary + automated judgment + manual gate`

### 8. MLMD

`artifact/execution/event lineage`

### 9. SRE

`incident → postmortem → action → organizational memory`

### 10. PBO/DSR

`search count becomes evidence`

### 11. Reusable Holdout

`protect evidence from adaptive reuse`

### 12. SLSA

`artifact provenance`

---

# 40. ما الذي يمكن تطويره؟

## AlphaEvolve → ResearchEvolve

بدل:

`Program improvement`

نستخدم:

`Hypothesis-conditioned Candidate evolution`

أي أن كل mutation مرتبط بمشكلة observed.

---

## DGM → Safe Evolution Graph

نحتفظ بالـtree، لكن كل edge يحتاج evidence:

```text
Parent
  ↓
Hypothesis
  ↓
Experiment
  ↓
Evidence
  ↓
Child

```

---

## AI Scientist → Governed Research Scientist

نأخذ research loop، ونضيف:

`budget + provenance + locked evidence + human gates`

---

## Kayenta → Research Canary

نحوّل:

`baseline vs canary`

إلى:

`control vs candidate`

مع behavior-specific metrics.

---

# 41. ما الذي يجب تجنبه؟

### تجنب

`LLM → code → deploy`

### تجنب

`LLM → optimize one score`

### تجنب

`train on failures → test on same data`

### تجنب

`candidate modifies evaluator`

### تجنب

`unlimited experiments`

### تجنب

`latest result replaces older knowledge`

### تجنب

`delete failed branches`

### تجنب

`single scalar notion of "better"`

---

# 42. Ideas Worth Stealing

## 1. Evaluator as a Guardian

من FunSearch/AlphaEvolve:

> الـgenerator ليس الحكم.

والـevaluator ليس مساعدًا للgenerator.

بل:

> **Evaluator = Security Boundary**

---

## 2. Evolution Tree وليس Best-Only

من DGM:

> لا تمحُ المسارات الضعيفة.

الـweak branch قد يحتوي على فكرة تظهر قيمتها لاحقًا.

---

## 3. Hypothesis Tournament

من AI co-scientist:

`Hypothesis A`
`Hypothesis B`
`Hypothesis C`

ثم debate/evidence/evolution.

---

## 4. Failure-First Research

من SRE:

لا تجعل system يسأل:

> "كيف أحسن؟"

بل:

> "ماذا حدث؟ لماذا حدث؟"

ثم:

> "ما الفرضية التي تفسره؟"

---

## 5. Query-Budgeted Holdout

من reusable holdout literature:

الـtest set ليست data warehouse.

هي evidence resource.

---

## 6. Trial Count as Metadata

من PBO/DSR:

`Result = performance + search history`

وليس performance وحده.

---

## 7. Two Trust Planes

```text
RESEARCH PLANE
Untrusted
Experimental
Mutable

CONTROL PLANE
Trusted
Governed
Restricted

```

هذه من أهم التحسينات المقترحة للمشروع.

---

## 8. Evidence Graph

لا تسجل:

> "Candidate B أفضل."

سجل:

```text
B
← Hypothesis H
← Experiment E
← Dataset D
← Evaluator V
← Control C
← Validation V2
← Human Decision D3

```

---

## 9. Contamination State

كل Candidate يجب أن يحمل:

```text
CLEAN
VALIDATED
OOS_EXPOSED
HOLDOUT_EXPOSED
CONTAMINATED
RETIRED

```

بعد رؤية holdout لا يعود candidate طبيعيًا في نفس البحث.

هذه فكرة عملية قوية مبنية على مشكلة adaptive holdout reuse.

---

## 10. Evaluator Immutability

Evaluator Version:

```text
EV-17

```

لا يجوز أن يتغير أثناء campaign.

أي تغيير:

```text
EV-18

```

يبدأ Evidence Family جديدة.

---

## 11. Research Budget كـStatistical Resource

لا تقل فقط:

`CPU hours = 100`

بل:

```text
Experiment Budget = 500
Candidate Budget = 300
Structural Mutation Budget = 20
Holdout Queries = 3
Adaptive Validation Decisions = X

```

---

## 12. Human Review Compression

لا تجعل الإنسان يراجع:

`20,000 candidates`

اجعله يراجع:

```text
Top Evidence Families
Novel Branches
Contradictory Results
High-Risk Candidates
Candidates with Major Structural Change

```

---

## 13. Auto-Rollback ≠ Auto-Evolution

هذه صياغة أعتقد أنها يجب أن تكون مبدأً رسميًا:

> **The system may automatically return to a known-good state, but it may not automatically authorize an unknown future state.**

هذه الجملة تلخص الحدود المعمارية للمشروع.

---

# 43. جدول الخلاصة

| الفكرةالمصدركيف تعملالنضجقابلة للتطبيق؟كيفية الاستخدامالمخاطر |                         |                                    |                              |          |                         |                                   |
| ------------------------------------------------------------- | ----------------------- | ---------------------------------- | ---------------------------- | -------- | ----------------------- | --------------------------------- |
| Evolutionary program search                                   | AlphaEvolve / FunSearch | generate + evaluate + archive      | Research/strong              | نعم      | Candidate Generator     | reward exploitation               |
| Evolution tree                                                | DGM                     | archive + branching                | Research                     | نعم      | Evolution Graph         | uncontrolled self-modification    |
| Automated research loop                                       | AI Scientist            | hypothesis→experiment→review       | Research                     | نعم      | Research Plane          | generated code risks              |
| Hypothesis evolution                                          | AI co-scientist         | generate/debate/evolve             | Research                     | نعم      | Hypothesis Engine       | weak hypotheses                   |
| AutoML search                                                 | Katib/AutoML            | systematic trial search            | Mature                       | نعم      | Parameter/Feature layer | multiple testing                  |
| Candidate vs baseline                                         | TFX                     | compare + threshold gate           | Mature                       | نعم جدًا | Validation Gate         | wrong metrics                     |
| Canary analysis                                               | Kayenta/Spinnaker       | baseline vs canary                 | Production                   | نعم      | Shadow/Canary           | peeking / bad thresholds          |
| Experiment tracking                                           | MLflow                  | runs/metrics/artifacts             | Mature                       | نعم      | Experiment Ledger       | metadata incompleteness           |
| Lineage                                                       | MLMD/PROV               | artifact/execution graph           | Mature                       | نعم جدًا | Evolution Graph         | mutable provenance                |
| Provenance                                                    | SLSA                    | verifiable build history           | Mature                       | نعم      | Candidate provenance    | trusted builder assumptions       |
| Root cause analysis                                           | CausalRCA/MicroRCA      | telemetry + causal/localization    | Research/production variants | نعم      | RCA Engine              | false causality                   |
| Postmortem learning                                           | Google SRE              | incident→actions→memory            | Mature                       | نعم جدًا | Failure Memory          | weak follow-up                    |
| PBO                                                           | Bailey et al.           | estimate backtest overfit          | Research/statistical         | نعم جدًا | Validation Firewall     | assumptions                       |
| DSR                                                           | Bailey & López de Prado | adjusts for trials/non-normality   | Research/statistical         | نعم      | Evidence scoring        | not a universal proof             |
| CPCV                                                          | financial ML literature | purging + combinatorial validation | Research/practical           | نعم      | OOS validation          | implementation assumptions        |
| Reusable holdout                                              | Dwork et al.            | controlled adaptive access         | Research/theoretical         | لاحقًا   | Holdout Service         | finance dependence/generalization |
| Reward-hacking benchmarks                                     | 2026 research           | adversarial evaluation             | Emerging                     | نعم جدًا | Evaluator Red Team      | benchmark coverage                |

المصادر الأساسية لهذه الخلاصة موزعة بين الأعمال البحثية والوثائق الرسمية المذكورة أعلاه.

---

# 44. Final Proposed Blueprint

أرى أن الشكل النهائي الأفضل لمشروعك هو:

```text
                    HUMAN
                      │
              ┌───────▼────────┐
              │ GOVERNANCE CORE │
              │ Policies        │
              │ Approval        │
              │ Risk Limits     │
              │ Metric Registry │
              └───────┬────────┘
                      │
          ┌───────────▼────────────┐
          │    RESEARCH PLANE      │
          │                        │
          │ Observation            │
          │ Failure Detection      │
          │ RCA                    │
          │ Hypothesis             │
          │ Experiment Planning    │
          │ Candidate Generation   │
          │ Evolution Search       │
          └───────────┬────────────┘
                      │
              ┌───────▼────────┐
              │   SANDBOX      │
              │ isolated code  │
              │ isolated data  │
              │ no production  │
              └───────┬────────┘
                      │
              ┌───────▼────────────┐
              │ VALIDATION FIREWALL│
              │ Leakage            │
              │ Purged/CPCV        │
              │ OOS                │
              │ PBO                │
              │ DSR                │
              │ Stress             │
              │ Regression         │
              └───────┬────────────┘
                      │
             ┌────────▼─────────┐
             │ CONTROL vs       │
             │ CANDIDATE        │
             └────────┬─────────┘
                      │
                 HUMAN GATE
                      │
                 ┌────▼────┐
                 │ SHADOW  │
                 └────┬────┘
                      │
                PROMOTION GATE
                      │
                 PRODUCTION
                      │
                MONITORING
                      │
            ┌─────────┴─────────┐
            │                   │
          HEALTH              FAILURE
            │                   │
            │              ┌────▼────┐
            │              │ROLLBACK │
            │              └────┬────┘
            │                   │
            └──────────────┬────▼─────┐
                           │POSTMORTEM│
                           └────┬─────┘
                                │
                         FAILURE MEMORY
                                │
                         EVOLUTION GRAPH
                                │
                             LEARN
                                │
                           OBSERVE

```

---

# 45. الحكم البحثي النهائي

بعد جمع الأدلة، أرى أن **أقوى صياغة علمية للمشروع ليست "Self-Modifying Trading System"**.

بل:

> **Human-Governed Autonomous Research and Evolution Engine**

لأن القيمة الحقيقية ليست أن النظام يستطيع تغيير نفسه.

القيمة هي أن النظام يستطيع:

```text
SEE
UNDERSTAND
EXPLAIN
HYPOTHESIZE
EXPERIMENT
FALSIFY
COMPARE
REMEMBER
PROPOSE

```

بينما يبقى:

```text
AUTHORIZATION
RISK
FINAL EVIDENCE
PRODUCTION STATE

```

خارج سيطرته الذاتية.

وهذا يخلق separation واضحًا بين:

### Intelligence

والـ

### Authority

والـ

### Evidence

والـ

### Execution

وهذه، في رأيي، هي القطعة المعمارية التي تجعل المشروع مختلفًا جذريًا عن مجرد AutoML أو LLM agent.

---

# 46. الخلاصة الأهم

إذا أردنا تلخيص كل البحث في 10 مبادئ:

```text
1. Never let Research directly mutate Production.

2. Every evolution begins as a versioned hypothesis.

3. Every experiment is evidence, including failure.

4. Candidate generation and candidate evaluation are separate trust domains.

5. The evaluator must be harder to manipulate than the candidate.

6. Trial count and adaptive search history are part of the evidence.

7. Final Holdout is a protected service, not ordinary data.

8. Production promotion requires explicit human authority.

9. Automatic rollback is allowed only to pre-approved known-good states.

10. Every failure becomes future research knowledge.

```

وأهم مبدأ على الإطلاق:

> **The engine should be allowed to discover that its own hypothesis is wrong.**

إذا لم يكن قادرًا على ذلك، فهو ليس Self-Evolution Engine بالمعنى العلمي؛ بل مجرد optimizer يبحث عن رقم أفضل.

---

# المصادر الأولية/الرئيسية

- AlphaEvolve — DeepMind + paper
- FunSearch — Nature
- Darwin Gödel Machine — arXiv 2025
- The AI Scientist / v2
- AI co-scientist
- AutoML-Zero
- TFX Evaluator / TFMA
- Netflix Kayenta / Spinnaker
- NIST AI RMF
- ISO/IEC 42001
- MLflow / ML Metadata
- W3C PROV
- SLSA Provenance
- PBO
- Deflated Sharpe Ratio
- White Reality Check / SPA
- CPCV comparison study
- Reusable Holdout
- Google SRE Postmortem/Canary
- Recent reward-hacking research
---

# 43. PRIOR PLANNING NOTES — PRESERVED SOURCE MATERIAL

The following planning material is preserved because it contains the earlier decisions that the desktop application is the primary control center, Telegram is optional, and the system should learn by observing, verifying, proposing, and waiting for human authority.

canvas

svgsvg

Today 4:20 PM

svg

M1_MASTER_CONSOLIDATED_AI_READABLE.txt

Document

svg

XAUUSD_D1_Institutional_Algorithmic_Trading_Core\_—\_Complete_Master.md

File

svg

XAUUSD_H4_Institutional_Algorithmic_Trading_Core\_—\_Complete_Master.md

File

svg

XAUUSD_M5_Quantitative_Trading_Core\_—\_Production_Oriented_Master.md

File

svg

XAUUSD_MN1_Sovereign_Macro_Trading_Core\_—\_Exhaustive_Institutional.md

File

svg

XAUUSD_W1_Sovereign_Grade_Algorithmic_Trading_Core\_—\_Complete_Master.md

File

svg

FINAL_V2\_—\_XAUUSD_H1_Institutional_Grade_Algorithmic_Trading_Core.md

File

svg

Institutional_Grade_XAUUSD_M1–M5_Algorithmic_Trading_Core\_—\_Master.md

File

svg

Institutional_Grade_XAUUSD_M30_Algorithmic_Trading_Core\_—\_Complete.md

File

إي، هلق فهمت الفكرة بدقة. أنت **ما بدك النظام يعدّل نفسه تلقائياً**؛ بدك يعمل دورة **Self-Learning + Human Approval** بحيث النظام يكتشف ويثبت المشكلة، ثم يرفعها لك، وأنت تكون صاحب القرار النهائي.

وفي نقطة مهمة: **بوت الموافقة لازم يكون منفصل تماماً عن بوت التوقعات/التقارير اليومية**.

### الدورة التي تقصدها

```
```

```
LIVE SYSTEM
     │
     ▼
Prediction + Market Observation
     │
     ▼
Research & Prediction Ledger
     │
     ▼
Outcome Evaluation
     │
     ▼
Error / Weakness Detection
     │
     ▼
Repeated Statistical Verification
     │
     ├── غير مؤكد
     │      └── لا تغيير
     │
     └── مؤكد
            │
            ▼
      Change Proposal
            │
            ▼
      Detailed Research Report
            │
            ▼
   ┌───────────────────────────┐
   │ Telegram Approval System  │
   │           ↓               │
   │      YOU REVIEW           │
   └───────────────────────────┘
            │
       ┌────┴────┐
       ▼         ▼
     REJECT    APPROVE
       │         │
       ▼         ▼
  Keep Current  Candidate
                 ↓
          Controlled Deployment
                 ↓
             New Version
```

### التقرير الذي يوصلك

ما بدي يكون مجرد:

> "وجدنا خطأ، نقترح تعديل الاستراتيجية."

بل يكون **Change Investigation Report** كامل، مثلاً:

```
```

```
CHANGE PROPOSAL
────────────────────────────

Proposal ID:
CP-000184

Detected At:
2026-09-24 14:15:03

First Evidence:
2026-08-17 09:45:00

Affected Component:
M15 Signal Validation

Current Version:
Strategy V1.3

Proposed Version:
Strategy V1.4

────────────────────────────
1. WHAT WAS DETECTED?
────────────────────────────

وصف المشكلة بالتفصيل.

────────────────────────────
2. WHEN DID IT FIRST APPEAR?
────────────────────────────

أول ظهور للمشكلة.

────────────────────────────
3. HOW WAS IT DETECTED?
────────────────────────────

عدد الحالات.
عدد العينات.
عدد الحالات المتشابهة.
الفترة الزمنية.

────────────────────────────
4. HOW WAS IT VERIFIED?
────────────────────────────

اختبارات إحصائية.
Historical replay.
Out-of-sample.
Walk-forward.
Robustness tests.

────────────────────────────
5. WHAT IS WRONG?
────────────────────────────

شرح المشكلة الحالية.

────────────────────────────
6. WHAT WILL CHANGE?
────────────────────────────

Current behavior
        ↓
Proposed behavior

────────────────────────────
7. WHY THIS CHANGE?
────────────────────────────

الدليل الذي أدى إلى الاقتراح.

────────────────────────────
8. EXPECTED EFFECT
────────────────────────────

ما الذي يُتوقع أن يتغير.

────────────────────────────
9. NEGATIVE EFFECTS / RISKS
────────────────────────────

ما الذي قد يسوء بسبب التعديل.

────────────────────────────
10. HISTORICAL COMPARISON
────────────────────────────

Current V1.3
vs
Candidate V1.4

────────────────────────────
11. VALIDATION RESULTS
────────────────────────────

Backtest
Walk-forward
OOS
Stress
Regime tests
etc.

────────────────────────────
12. AFFECTED TIME PERIODS
────────────────────────────

كل الفترات والحالات التي أدت
إلى اكتشاف المشكلة.

────────────────────────────
13. FILES / DATA / PREDICTIONS
────────────────────────────

Prediction IDs
Experiment IDs
Dataset IDs
Replay IDs
etc.

────────────────────────────
14. FINAL SYSTEM RECOMMENDATION
────────────────────────────

PROPOSED FOR HUMAN REVIEW

────────────────────────────
15. APPROVAL
────────────────────────────

[ APPROVE ]
[ REJECT ]
[ DETAILS ]
[ COMPARISON ]
```

وهذا بالضبط يخلي **أنت الباحث النهائي**.

النظام يقول لك:

> "أنا اكتشفت هذا السلوك، وهذه الأدلة التي تثبته، وهذا التعديل المقترح، وهذه نتائجه."

وأنت تقوم بالجزء الذي ذكرته:

> ترجع للوقت الذي حصلت فيه المشكلة، تفحص السوق والأحداث والبيانات، تبحث بنفسك عن السبب، وبعدها تقرر إذا التعديل منطقي فعلاً أم لا.

---

## والأهم: لازم نسجل "سبب الرفض" أيضاً

إذا رفضت التعديل، لا نرميه.

مثلاً:

```
```

```
Proposal CP-000184

Status:
REJECTED

Human Reason:
False pattern caused by temporary
high-volatility regime.

Decision By:
Human

Decision Time:
...

Current Version:
V1.3 remains active
```

وبالتالي النظام يتعلم حتى من **قراراتك أنت**، بدون AI.

مع الوقت يصبح عندنا:

```
```

```
Detected Error
      ↓
Verified Error
      ↓
Proposed Change
      ↓
Human Investigation
      ↓
APPROVED / REJECTED
      ↓
Decision History
```

وهذا مهم جداً لأن النظام لا يتعلم فقط:

**"ماذا حدث في السوق؟"**

بل أيضاً:

**"ما هي التعديلات التي اقترحها؟ وما الذي وافق عليه الإنسان أو رفضه؟ ولماذا؟"**

---

## بالنسبة لبوت Telegram الثاني

نعم، فصل البوتين فكرة جيدة جداً:

```
```

```
BOT 1 — OPERATIONS / MARKET BOT
────────────────────────────────
• توقعات M15
• نتائج التوقعات
• تقارير H4
• حالة السوق
• الأخطاء
• الإحصائيات
• التنبيهات
```

وبالمقابل:

```
```

```
BOT 2 — GOVERNANCE / APPROVAL BOT
────────────────────────────────
• Change Proposals
• Research Findings
• Candidate Versions
• Validation Reports
• Approve
• Reject
• Compare
• Rollback
• Audit Trail
```

والـ **Bot 2 لا يملك صلاحية تعديل النظام مباشرة بمجرد أن تصل رسالة**.

الأفضل:

```
```

```
Telegram Approval
       ↓
Approval Service
       ↓
Verify Proposal ID
       ↓
Verify User Authorization
       ↓
Check Candidate Hash/Version
       ↓
Record Approval
       ↓
Promotion Manager
       ↓
Deploy Candidate
```

يعني حتى لو صار خلل في Telegram، ما بيقدر يسبب تغيير عشوائي.

### بخصوص رمز الوصول الذي أرسلته

إذا كان الرمز الذي كتبته **Token حقيقي لبوت Telegram**، اعتبره سرياً ولا تضعه داخل الكود أو Git أو ملفات المشروع العامة. وإذا كان Token فعلياً وقد تم كشفه هنا، الأفضل **تدويره/إلغاؤه وإنشاء Token جديد** من BotFather.

وبالنسبة لخطوة **رقم الهاتف + تأكيد الطلب من تطبيق Telegram**: هذه تبدو أقرب إلى **تفويض حساب Telegram/جلسة مستخدم** وليست آلية Bot API المعتادة. لذلك أنا ما رح أفترض أن هذا هو التصميم الصحيح قبل تحديد نوع الوصول. والأفضل أمنياً أن يبقى **حساب Telegram الشخصي منفصلاً عن صلاحيات النظام** قدر الإمكان.

**الخلاصة:** نعم، التصميم الذي تريده قابل للتنفيذ بدون AI، وأرى أن الاسم الأدق له هو:

> **Self-Learning Statistical System with Human-Governed Change Management**

والـ Self-Learning هنا **يكتشف → يتحقق → يقترح → يوثق → ينتظر قرارك**، وليس **يكتشف → يغير نفسه**.

نسيت قلك امو البوتات مش اساسيات الاساسي هوا تطبيق الاساسي و هوي يلي رح يحوي الواجهات و القرائات و حتى تقرير الاصلاح التلقائي يمكنني الموافقة عليه من تطبيق التطبيق هو يجب ان يحوي كل شيئ و التلجرام ل مساعدين ممكن اعطيه يشتغلو على التلجرام و خلي تطبيق خاص فيني

إي، هيك الصورة صارت أوضح بكثير. **التطبيق هو النظام الأساسي والمرجع الرئيسي، وTelegram مجرد واجهات مساعدة اختيارية**.

أنا أعدل المعمارية السابقة بهذا الشكل:

```
```

```
                    XAUUSD SYSTEM
                         │
              ┌──────────┴──────────┐
              │                     │
              ▼                     ▼
       CORE / ENGINE          RESEARCH ENGINE
              │                     │
              └──────────┬──────────┘
                         │
                         ▼
                DATABASE / LEDGER
                         │
                         ▼
                 DESKTOP APPLICATION
                    ★ PRIMARY ★
                         │
        ┌────────────────┼────────────────┐
        │                │                │
        ▼                ▼                ▼
   Market View      Research View    System Control
        │                │                │
        ▼                ▼                ▼
 Predictions       Self-Learning     Approvals
 H4/M15 Reports    Error Discovery   Versions
 M1/M5 Context     Experiments       Rollback
 Statistics        Candidates        Configuration
 Health            Validation        Audit
```

## التطبيق هو الـ Control Center

كل شيء مهم يجب أن يكون موجوداً داخل التطبيق:

### 1. Market Dashboard

يعرض مثلاً:

- XAUUSD
- H4 Market Compass
- M15 Execution Pulse
- M5/M1 context
- السعر الحالي
- volatility
- spread
- market regime
- structure
- signals
- confidence/confluence
- data health
- connection status

---

### 2. Prediction Center

كل توقع M15 يصبح له سجل كامل:

```
```

```
Prediction ID
Timestamp
Price
Direction
Confidence Score
H4 State
M15 State
M5 State
M1 State
Market Regime
Expected Move
Expected Horizon
Actual Move
MFE
MAE
Result
```

وتقدر ترجع لأي Prediction وتشوف **لماذا النظام اتخذ هذا القرار في تلك اللحظة**.

---

### 3. Research Center

هون قلب الـ Self-Learning:

```
```

```
Historical Predictions
       ↓
Outcome Analysis
       ↓
Pattern Detection
       ↓
Error Detection
       ↓
Verification
       ↓
Research Findings
       ↓
Change Proposal
```

---

# 4. Auto-Repair / Self-Improvement Center

وهذا الجزء مهم جداً حسب فكرتك.

إذا النظام اكتشف مشكلة، **لا يغير نفسه**.

بدلاً من ذلك ينشئ:

```
```

```
CHANGE PROPOSAL #CP-00184
```

والتطبيق يعرض لك:

### المشكلة

متى ظهرت؟

```
```

```
First observed:
2026-08-17 09:45:00
```

### كيف اكتشفها؟

```
```

```
Affected predictions: 247
Failure cases: 163
Similar market conditions: 189
Verification cycles: 7
```

### ما الذي يعتقد النظام أنه خاطئ؟

شرح تقني كامل.

### ما الذي يريد تغييره؟

```
```

```
CURRENT
M15 signal filter = X

PROPOSED
M15 signal filter = Y
```

### لماذا؟

كل الأدلة والإحصائيات.

### ماذا سيحدث لو طبقناه؟

Comparison:

```
```

```
                    CURRENT      CANDIDATE

Sample size             ...          ...
Signals                 ...          ...
Failures                ...          ...
Neutral                 ...          ...
MFE                     ...          ...
MAE                     ...          ...
Drawdown                ...          ...
Regime A                ...          ...
Regime B                ...          ...
Regime C                ...          ...
```

### السلبيات المحتملة

لازم النظام **يعرض أيضاً أسباب عدم تطبيق التغيير**، وليس فقط فوائده.

---

# 5. أنت تعمل Investigation

وهذه نقطة أساسية في التصميم.

النظام يقول:

> "اكتشفت المشكلة في هذه الظروف."

وأنت تفتح التفاصيل وتحقق بنفسك:

```
```

```
2026-08-17
09:45
XAUUSD
H4 = ...
M15 = ...
Spread = ...
Volatility = ...
News/Event = ...
Prediction = ...
Actual = ...
```

وتبحث أنت:

**شو صار بالسوق بهذا الوقت؟**

وبعدها:

```
```

```
                 CHANGE PROPOSAL

                    YOU
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
       APPROVE                REJECT
          │                     │
          ▼                     ▼
 Candidate → Deploy       Keep Current
```

---

# 6. Approval داخل التطبيق نفسه

وهذا أهم تعديل.

**ما لازم تكون محتاج Telegram حتى توافق.**

داخل التطبيق:

```
```

```
┌─────────────────────────────────────────┐
│ CHANGE PROPOSAL CP-00184                │
├─────────────────────────────────────────┤
│                                         │
│ Problem detected                        │
│ ...                                     │
│                                         │
│ Proposed change                         │
│ ...                                     │
│                                         │
│ Validation                              │
│ PASS                                    │
│                                         │
│ Risk / drawbacks                        │
│ ...                                     │
│                                         │
│ [ VIEW EVIDENCE ]                       │
│ [ COMPARE VERSIONS ]                    │
│ [ VIEW AFFECTED TRADES ]                │
│                                         │
│      [ APPROVE ]     [ REJECT ]         │
│                                         │
└─────────────────────────────────────────┘
```

والقرار يدخل مباشرة إلى:

```
```

```
Audit Ledger
```

مثلاً:

```
```

```
Proposal ID: CP-00184
Decision: APPROVED
Decision Time: ...
Approved Version: V1.4
Previous Version: V1.3
```

---

# 7. Telegram يصبح مساعداً فقط

وهذا أفضل فصل:

### التطبيق

**Primary Interface**

كل شيء موجود فيه.

### Telegram Bot 1

**Market Assistant**

مثلاً:

```
```

```
/الحالة
/السوق
/تحليل
/توقعات
/نتائج
/إحصائيات
/أخطاء
/تقرير
```

### Telegram Bot 2

**Research / Governance Assistant**

يستطيع إخبارك:

```
```

```
🔬 New Research Finding

CP-00184

تم اكتشاف مشكلة محتملة في M15...

افتح التطبيق لمراجعة التقرير الكامل.
```

وممكن يعطيك معلومات مختصرة، لكن **التطبيق يبقى المصدر الرئيسي**.

---

# والأهم: لا نعطي Telegram صلاحيات أكبر من التطبيق

أنا أقترح أن تكون الصلاحيات هكذا:

```
```

```
                  APPLICATION
                       │
                PRIMARY AUTHORITY
                       │
       ┌───────────────┼────────────────┐
       │               │                │
       ▼               ▼                ▼
    Research        Approval         Control
       │               │                │
       └───────────────┴────────────────┘
                       │
                       ▼
                  CORE SYSTEM


Telegram
   │
   ├── Read Status
   ├── Receive Alerts
   ├── Request Reports
   └── Limited Commands
```

يعني **Telegram ليس هو النظام**.

إذا Telegram توقف:

```
```

```
Telegram DOWN
     ↓
System continues
     ↓
Research continues
     ↓
Predictions continue
     ↓
Database continues
     ↓
Application continues
```

فقط المساعد الخارجي يتوقف.

---

## وبالنسبة لرمز Telegram الذي أعطيتني إياه

بما أنك تريد Telegram كمساعد وليس كواجهة النظام الأساسية، **ما في داعي أصلاً أن نعتمد على وصول حساب Telegram الشخصي حتى نبني المعمارية الأساسية**.

نقدر نصمم النظام بحيث:

```
```

```
Core
 ↓
Application
 ↓
Database
```

هو الأساس، وبعدها:

```
```

```
                    ┌── Telegram Bot 1
                    │
Core → API/Service ─┼── Telegram Bot 2
                    │
                    └── Future Interfaces
```

وهذا يعطيك أهم ميزة: **إذا قررت لاحقاً إزالة Telegram بالكامل، النظام لا يتأثر.**

وبالتالي الشكل النهائي الذي أراه مناسباً لمشروعك حالياً هو:

> **Desktop Application = Brain's Human Interface + Control Center**
>
> **C++ Core = System Brain**
>
> **Research Engine = Self-Learning Engine**
>
> **SQLite/Research DB = Memory**
>
> **Telegram = Optional Assistants**
>
> **Human = Final Authority for System Changes**
>
> **AI = Completely absent for now**

وهذا أقرب بكثير للفكرة التي عم تحاول تبنيها من البداية.

Ad

شو عندك ليا افكار ل تطوير تلقائي

إي، عندي أفكار كثيرة، والأهم أني ما بشوف **التطوير التلقائي** كـ"تغيير الاستراتيجية كلما خسر صفقة". الأفضل نبني **Evolution Engine** يكتشف فرص التحسين بشكل منظم، لكن كل تغيير يبقى Candidate إلى أن تراجعه أنت.

أنا أقسمه إلى طبقات:

### 1. Automatic Error Discovery

النظام يبحث تلقائياً عن أنماط الفشل:

- أخطاء متكررة في M15.
- أخطاء تظهر فقط أثناء volatility مرتفع.
- أخطاء مرتبطة بـ spread.
- أخطاء في جلسة معينة.
- أخطاء بعد أخبار معينة.
- أخطاء عندما H4 وM15 متعارضان.
- أخطاء في transitions بين regimes.
- أخطاء مرتبطة بوقت معين من اليوم.
- حالات يتأخر فيها الدخول.
- حالات يعطي فيها النظام إشارة صحيحة لكن التنفيذ الافتراضي كان سيئاً.

المهم: **لا يعدّل شيئاً بعد الاكتشاف.**

---

### 2. Automatic Root-Cause Investigation

بدلاً من:

> "Win rate انخفض."

يحاول تحديد **أين المشكلة بالضبط**.

مثلاً:

```
```

```
Total Predictions: 12,840

Failures:
    2,931

Concentration:
    71% occurred during
    EXTREME_VOLATILITY

Further condition:
    H4 = TREND
    M15 = TRANSITION

Further condition:
    Spread > P90
```

فيصبح عندك:

> "هناك مشكلة محتملة في التعامل مع M15 Transition أثناء Extreme Volatility."

وليس مجرد "الاستراتيجية سيئة".

---

### 3. Automatic Regime-Specific Learning

هذه من أهم الأفكار.

لا نحاول إيجاد استراتيجية واحدة لكل السوق.

النظام يراقب:

```
```

```
TREND_UP
TREND_DOWN
RANGE
EXPANSION
CONTRACTION
TRANSITION
EXTREME_VOLATILITY
NEWS_EVENT
```

ثم يكتشف:

```
```

```
Current Strategy
      ↓
Performance by Regime
      ↓
Weak Regimes
      ↓
Candidate Modification
```

مثلاً قد يكتشف أن المشكلة ليست في النظام كله، بل في **Regime واحد فقط**.

---

### 4. Automatic Threshold Optimization

بدلاً من أن تكون كل الحدود ثابتة للأبد:

```
```

```
Spread < 0.50
ATR > X
Confidence > 80
```

النظام يستطيع إنشاء Candidates:

```
```

```
Candidate A → 75
Candidate B → 80
Candidate C → 85
Candidate D → dynamic threshold
```

ويختبرها على بيانات تاريخية منفصلة.

لكن **لا يسمح لنفس البيانات أن تختار وتثبت التعديل**.

---

### 5. Automatic Feature Discovery

النظام يستطيع اكتشاف أن بعض المعلومات الموجودة عنده لها علاقة بنتائج معينة.

مثلاً:

```
```

```
Features:
H4 trend
M15 momentum
spread
volatility
session
distance from structure
M1 impulse
M5 structure
```

ثم يحلل:

```
```

```
Which combinations
are associated with
different outcomes?
```

وقد ينتج:

```
```

```
Candidate Finding:

M15 Momentum + H4 Structure
appears more informative than
M15 Momentum alone.
```

ثم يدخلها كـ **Experiment**، وليس كحقيقة.

---

### 6. Automatic Strategy Mutation

وهذه مرحلة متقدمة جداً.

النظام يأخذ Strategy Version:

```
```

```
V1.3
```

ويولد Candidates:

```
```

```
V1.4-A
V1.4-B
V1.4-C
V1.4-D
```

كل Candidate يغير **شيئاً واحداً أو مجموعة محددة بوضوح**.

مثلاً:

```
```

```
V1.4-A
Change: volatility filter

V1.4-B
Change: M15 confirmation

V1.4-C
Change: spread filter

V1.4-D
Change: regime eligibility
```

ثم يعمل:

```
```

```
Backtest
↓
Walk Forward
↓
OOS
↓
Stress
↓
Stability
```

والـ Candidates الضعيفة يتم رفضها آلياً.

---

### 7. Automatic Experiment Generator

هذه برأيي واحدة من أقوى الأفكار.

النظام نفسه يبني **Research Experiments**.

مثلاً:

```
```

```
EXPERIMENT E-00491

Question:
Does spread filtering improve
M15 prediction quality during
high volatility?

Hypothesis:
...

Dataset:
...

Variables:
...

Control:
...

Candidate:
...

Evaluation:
...

Result:
...

Conclusion:
...
```

وبالتالي Research Lab يبدأ يصبح فعلاً **مختبر آلي**.

---

### 8. Automatic Version Evolution

تقدر تعمل شجرة نسخ:

```
```

```
V1.0
 ├── V1.1-A
 ├── V1.1-B
 └── V1.1-C
          ↓
      Validation
          ↓
       V1.1-C
          ↓
       HUMAN
          ↓
       APPROVE
```

ولا يتم حذف:

```
```

```
V1.0
V1.1-A
V1.1-B
```

حتى لو فشلوا.

لأنهم جزء من **تاريخ تطور النظام**.

---

### 9. Automatic Regression Testing

هاي ضرورية جداً.

كلما اقترح النظام تغييراً، يجب أن يسأل:

> هل أصلحت المشكلة وكسرت شيئاً آخر؟

مثلاً:

```
```

```
Problem:
Reduce false signals

Candidate:
V1.5
```

قد تنخفض false signals، لكن بالمقابل:

```
```

```
Good signals ↓
Missed opportunities ↑
Performance in TREND regime ↓
```

النظام يجب أن يكتشف ذلك قبل أن يوصلك التعديل.

---

### 10. Automatic "Do Nothing" Decision

هذه مهمة جداً.

ليس مطلوباً من النظام أن يجد تعديلاً دائماً.

قد تكون النتيجة:

```
```

```
Finding detected
      ↓
Investigated
      ↓
Evidence insufficient
      ↓
NO CHANGE
```

أو:

```
```

```
Candidate improvement detected
      ↓
Not robust across regimes
      ↓
REJECT
```

وهذا أفضل من إجباره على التطور باستمرار.

---

## 11. Automatic Knowledge Base

مع الوقت يصير عندك شيء مثل:

```
```

```
SYSTEM KNOWLEDGE

Finding #001
────────────────
Condition:
...

Observed:
...

Experiment:
...

Result:
...

Human Decision:
APPROVED

────────────────

Finding #002
Condition:
...

Human Decision:
REJECTED

Reason:
Temporary market regime
```

وهكذا النظام يبني **ذاكرة بحثية تاريخية**.

---

## 12. Human Decision Feedback

وهذه أعتقد أنها مهمة جداً لمشروعك.

كل مرة أنت تعمل:

```
```

```
APPROVE
```

أو:

```
```

```
REJECT
```

يسجل:

```
```

```
What was proposed
Why
What evidence existed
Human decision
Human reason
Final outcome
```

وبعد فترة تستطيع دراسة:

> ما أنواع التعديلات التي كنت أوافق عليها؟
>
> ما أنواع التعديلات التي كنت أرفضها؟
>
> هل كانت قرارات الرفض صحيحة لاحقاً؟
>
> هل بعض أنواع التعديلات تسبب مشاكل؟

**بدون AI**.

---

# 13. Automatic Rollback Monitoring

بعد موافقتك على V1.7:

```
```

```
V1.7 DEPLOYED
```

النظام يراقبها.

إذا ظهرت **شروط فشل مسبقة ومحددة مسبقاً**:

```
```

```
Regression Detected
Safety Threshold Breached
Data Anomaly
Execution Integrity Problem
```

لا يقوم بتغيير الاستراتيجية عشوائياً.

بل:

```
```

```
V1.7
   ↓
Emergency protection / stop candidate
   ↓
Restore V1.6 if policy allows
   ↓
Generate Incident Report
   ↓
Notify Human
```

والـ rollback نفسه يجب أن يكون موثقاً.

---

# 14. Automatic "Research Priority"

إذا عندك 50 مشكلة محتملة، النظام يستطيع ترتيبها **حسب معايير بحثية قابلة للقياس** مثل:

```
```

```
Number of affected predictions
Evidence strength
Reproducibility
Potential impact
Regime coverage
Data quality
Validation confidence
```

لكن بدون أن يقول:

> "هذه أفضل استراتيجية."

بل:

> "هذه المشكلة تستحق التحقيق أولاً وفق المعايير المحددة."

---

# 15. Evolution Graph

وأنا أحب أضيفها للتطبيق نفسه:

```
```

```
                    V1.0
                     │
          ┌──────────┼──────────┐
          │          │          │
        V1.1-A     V1.1-B     V1.1-C
          │                     │
        REJECT                ACCEPT
                                │
                               V1.2
                                │
                     ┌──────────┼─────────┐
                     │          │         │
                   V1.3-A     V1.3-B    V1.3-C
                     │          │
                   REJECT     REJECT
                               
```

وتضغط على أي Version وتشوف:

- لماذا ولدت؟
- ما المشكلة التي حاولت حلها؟
- ما البيانات؟
- ما التجارب؟
- نتائج الاختبارات؟
- لماذا قُبلت؟
- لماذا رُفضت؟
- من وافق؟
- متى؟
- ما الذي تغير؟

هذا سيجعل المشروع فعلاً **قابلاً للتطور عبر السنوات**.

---

## والأهم عندي: 4 مستويات للتطوير

ما بخلي النظام يملك صلاحية واحدة فقط.

```
```

```
LEVEL 0
Observation
↓
يكتشف المشكلة فقط

LEVEL 1
Research
↓
يقترح Experiments

LEVEL 2
Candidate
↓
يبني ويختبر Version جديدة

LEVEL 3
Human Approval
↓
أنت تقرر

LEVEL 4
Deployment
↓
تطبيق النسخة المعتمدة
```

وبالتالي **Self-Learning لا يعني Self-Modification**.

بل:

> **Self-Observation → Self-Analysis → Self-Experimentation → Self-Validation → Human Decision → Controlled Evolution**

وهذا، برأيي، هو الاتجاه الذي يناسب مشروعك أكثر من إدخال AI حالياً، خصوصاً مع رغبتك أن يكون **C++ Core + Desktop Application + Research Database** هو الأساس، وتبقى Telegram مجرد مساعد خارجي.

### Chat paused until usage resets tomorrow at 1:09 AM

You’ve reached the limit for chats that include files or images. Start a new text-only chat or upgrade to continue now.

New chat

Upgrade
---

# 44. UNIFIED DECISION REGISTER

This section converts the entire body of work into explicit project decisions.

| ID | Decision | Status | Consequence |
|---|---|---|---|
| UD-001 | Research cannot write Production directly | CORE | Separate trust planes and permissions |
| UD-002 | Every material evolution is a versioned Candidate | CORE | No invisible mutation |
| UD-003 | Every material change needs a Hypothesis or documented campaign | CORE | Research remains interpretable |
| UD-004 | Candidate generation and evaluation are separate trust domains | CORE | Evaluator protected |
| UD-005 | Final Holdout is a protected evidence boundary | CORE | Limit adaptive leakage |
| UD-006 | Trial history is recorded as evidence | CORE | Results interpreted in search context |
| UD-007 | System may choose NO CHANGE | CORE | Prevent forced over-optimization |
| UD-008 | Human is final authority for production change | CORE | Approval gate mandatory |
| UD-009 | Application is primary control center | CORE | Telegram remains optional |
| UD-010 | Automatic rollback is allowed only to approved known-good states | CORE | Self-preservation without self-authorization |
| UD-011 | Failure records are permanent research objects | CORE | Evolution learns from rejection |
| UD-012 | Evolution depth and permission are independent axes | CORE | Avoid unsafe autonomy coupling |
| UD-013 | Architecture and meta-evolution remain research-only initially | PHASED | High-risk capabilities delayed |
| UD-014 | AI is not required for the first implementation of the evolution loop | PHASED | Deterministic research kernel first |
| UD-015 | Any future AI acts as a research component, not the authority plane | FUTURE | Safe insertion point for LLMs |

---

# 45. RECOMMENDED BUILD ORDER

The project should be implemented in this order:

```text
1. Project foundation and contracts
2. Core infrastructure / persistence / health
3. Market data adapters and validation
4. Timeframe state and point-in-time correctness
5. Features / structure / regime
6. Strategy / signal / score / confidence
7. Macro / event / market-quality / risk
8. Shadow execution and outcome ledger
9. Replay and research harness
10. Observation + failure detection
11. RCA + hypothesis engine
12. Experiment engine + ledger
13. Candidate generator + evolution graph
14. Validation firewall + evidence firewall
15. Human governance + approval package
16. Shadow challenger
17. Promotion / rollback / monitoring
18. Failure memory / evidence graph
19. Broader evolution levels
20. Research-only meta-evolution
```

## 45.1 Explicit first milestone

The first evolution milestone is **not** “automatic strategy improvement”.

It is:

> **The system can detect a recurring weakness, reproduce it, describe it, and produce a complete research case without changing itself.**

This is the foundation required before candidate evolution becomes trustworthy.

## 45.2 Second milestone

> **The system can generate multiple sandbox candidates from a declared hypothesis and automatically reject those that fail predefined validation.**

## 45.3 Third milestone

> **The system can present an auditable evidence package to a human and safely keep the current production version unchanged when the human rejects the proposal.**

## 45.4 Fourth milestone

> **The approved candidate can run in shadow, be monitored, and only then enter a separate promotion process.**

---

# 46. DEFINITION OF DONE FOR THE EVOLUTION ENGINE

The Evolution Engine should not be considered complete until all of the following are true:

```text
[ ] Observation records are versioned and reproducible
[ ] Failures have stable IDs
[ ] RCA stores competing hypotheses
[ ] Hypotheses include falsification conditions
[ ] Experiments are immutable/append-only in the ledger
[ ] Search spaces are versioned
[ ] Candidates have explicit parents
[ ] Candidate artifacts have provenance
[ ] Exact duplicate experiments are prevented
[ ] Near duplicates trigger review
[ ] Evolution budgets are enforced
[ ] Candidate cannot alter evaluator
[ ] Metrics are versioned
[ ] Validation protocol is selected before evaluation
[ ] Holdout boundary is protected
[ ] Holdout access is audited
[ ] Contamination state exists
[ ] NO_CHANGE is supported
[ ] Human decisions are attributable
[ ] Application is primary approval interface
[ ] Telegram cannot bypass the control plane
[ ] Shadow has no execution authority
[ ] Promotion is a separate operation
[ ] Rollback target is known-good and approved
[ ] Rollback generates an incident
[ ] Failure memory stores the incident
[ ] Evolution graph is queryable
[ ] Evidence graph is queryable
[ ] Full audit trail is reproducible
```

---

# 47. RESEARCH QUESTIONS THAT SHOULD REMAIN OPEN

The project should deliberately preserve uncertainty in the following areas until measured:

```text
Which candidate generation method is best?
How large should the candidate population be?
How should search budget scale with regime complexity?
Which statistical controls are needed for each campaign type?
How should hypothesis quality be measured?
How should RCA confidence be calibrated?
How should human review be compressed safely?
When should a candidate enter shadow?
How long should shadow last?
What constitutes sufficient post-deployment evidence?
When should a retired branch be revisited?
How should research memory avoid leakage?
What level of meta-evolution is actually useful?
```

No answer is assumed merely because it sounds architecturally elegant.

---

# 48. SYSTEM LANGUAGE / UI VOCABULARY

The application should consistently use precise terms.

### Preferred

```text
Signal Score
Confidence
Calibrated Probability (only when validated)
Data Quality
Execution Quality
Risk Status
Research Finding
Hypothesis
Candidate
Validation Status
Contamination Status
Human Decision
Shadow Status
Production Version
Known-Good Version
```

### Avoid unsupported claims

```text
Guaranteed setup
Safe trade
Guaranteed probability
Profitable system
Perfect model
Global liquidity
Certain prediction
```

---

# 49. FINAL MASTER ARCHITECTURE

```text
                                         HUMAN
                                           │
                              ┌────────────▼────────────┐
                              │     GOVERNANCE CORE     │
                              │ Policy / Risk / Approval│
                              │ Evaluator / Metrics     │
                              │ Audit / Versioning      │
                              └────────────┬────────────┘
                                           │
                    ┌──────────────────────┼──────────────────────┐
                    │                      │                      │
                    ▼                      ▼                      ▼
             RESEARCH PLANE         VALIDATION PLANE        RUNTIME PLANE
                    │                      │                      │
          Observe / Detect         Leakage / OOS /         Production
          Diagnose / RCA           PBO / DSR / CPCV        Shadow
          Hypothesis               Stress / Regression     Monitoring
          Experiment               Holdout Service         Rollback
          Candidate Gen                                      │
                    │                      │                  │
                    └──────────────┬───────┘                  │
                                   ▼                          │
                              CANDIDATE                       │
                                   │                          │
                              HUMAN GATE                     │
                                   │                          │
                                SHADOW                        │
                                   │                          │
                             PROMOTION                        │
                                   │                          │
                                   ▼                          │
                             PRODUCTION ─────────────────────┘
                                   │
                             OUTCOME / INCIDENT
                                   │
                         POSTMORTEM / FAILURE MEMORY
                                   │
                         EVOLUTION + EVIDENCE GRAPHS
                                   │
                                RESEARCH
                                   │
                                  LOOP
```

---

# 50. FINAL PROJECT PRINCIPLES

```text
1. Research before execution.
2. Observation before optimization.
3. Diagnosis before parameter search.
4. Hypothesis before material evolution.
5. Candidate before change.
6. Experiment before belief.
7. Evidence before promotion.
8. Human authority before production mutation.
9. Known-good rollback before emergency improvisation.
10. Failure memory before forgetting.
11. Lineage before trust.
12. Immutable history before convenience.
13. Diversity before best-only search.
14. Constraints before scalar reward.
15. Trial history before interpreting performance.
16. Do-nothing is a valid scientific result.
17. Evolution depth and permission are different things.
18. Production is not the laboratory.
19. Telegram is not the authority plane.
20. Self-evolution is bounded autonomy, not unrestricted self-modification.
```

---

# 51. FINAL CANONICAL STATEMENT

> **Human-Governed Autonomous Research and Evolution Engine for the AURA Multi-Timeframe Shadow Research System**
>
> The system observes itself, detects weaknesses, investigates causes, formulates falsifiable hypotheses, designs experiments, generates and evaluates candidates inside a sandbox, preserves evidence and failures, presents auditable proposals to the human, runs approved candidates in shadow, promotes only through explicit authority, monitors deployed behavior, rolls back only to known-good approved states, and continuously turns outcomes into research memory — without granting the research engine direct authority to redefine evidence or production state.

---

# 52. PROJECT STATUS AT THE END OF THIS DOCUMENT

```text
ARCHITECTURE:
UNIFIED

RESEARCH BASIS:
DOCUMENTED

EVOLUTION MODEL:
HUMAN-GOVERNED

CORE XAUUSD SYSTEM:
CANONICAL BASELINE PRESERVED

IMPLEMENTATION:
NOT YET COMPLETE

BROKER-SPECIFIC VALIDATION:
REQUIRED

PROBABILITY CALIBRATION:
NOT ESTABLISHED

PROFITABILITY:
UNPROVEN

LIVE AUTHORITY:
RESTRICTED

PRIMARY HUMAN INTERFACE:
DESKTOP APPLICATION

OPTIONAL EXTERNAL ASSISTANTS:
TELEGRAM

AI DEPENDENCY:
NONE FOR INITIAL EVOLUTION KERNEL

FUTURE AI ROLE:
RESEARCH COMPONENT ONLY, SUBJECT TO GOVERNANCE
```

---

# 53. SOURCE-BASED RESEARCH REFERENCES

The research record identifies the following primary/reference families for future detailed verification and citation management:

- AlphaEvolve — DeepMind + associated research paper
- FunSearch — Nature / program search
- Darwin Gödel Machine — research work on self-modifying agent evolution
- The AI Scientist / AI Scientist-v2
- AI co-scientist
- AutoML-Zero
- TFX Evaluator / TFMA
- Netflix Kayenta / Spinnaker
- NIST AI Risk Management Framework
- ISO/IEC 42001
- MLflow / ML Metadata
- W3C PROV
- SLSA provenance
- Probability of Backtest Overfitting
- Deflated Sharpe Ratio
- White Reality Check / Superior Predictive Ability
- CPCV / purged validation literature
- Reusable Holdout
- Google SRE postmortem / canary practices

For the implementation, these should be re-checked against the exact versions/primary sources used when the project moves from planning into verification.

---

# 54. DOCUMENT CHANGE CONTROL

This master should be updated by explicit version increments.

```text
MAJOR
= architectural boundary changed

MINOR
= component / workflow / schema added or changed

PATCH
= wording / formatting / non-semantic correction
```

Every future revision should include:

```text
Version
Date
Changed Sections
Reason
Evidence
Impact
Approval / Decision
```

**End of unified master planning document.**

---

# V2-01. VERSION 2.0 — NORMATIVE UPDATE

## 55.1 Purpose

Version 2.0 formalizes the architectural changes added after the first unified master was produced. These changes do not replace the XAUUSD market core. They add the missing control architecture required for long-lived autonomous research while preserving deterministic operation, human authority, auditability, and bounded resource usage.

The updated system is defined as:

```text
XAUUSD MARKET / BROKER REALITY
            │
            ▼
   DETERMINISTIC RUNTIME CORE
            │
     ┌──────┴───────┐
     │              │
     ▼              ▼
 OBSERVATION     OUTCOMES
     │              │
     └──────┬───────┘
            ▼
      SELF-LEARNING
            │
            ▼
         KNOWLEDGE
            │
            ▼
        HYPOTHESIS
            │
            ▼
        RESEARCH LOOP
            │
            ▼
       CANDIDATE EVOLUTION
            │
            ▼
      VALIDATION FIREWALL
            │
            ▼
     HUMAN GOVERNANCE
            │
     ┌──────┴──────┐
     ▼             ▼
   REJECT         APPROVE
                    │
                    ▼
                  SHADOW
                    │
                    ▼
               PROMOTION GATE
                    │
                    ▼
                CONTROLLED
                 RUNTIME
                    │
                    ▼
               MONITORING
                    │
            ┌───────┴───────┐
            ▼               ▼
          CONTINUE        FAILURE
                            │
                            ▼
                         ROLLBACK
                            │
                            ▼
                       POSTMORTEM
                            │
                            ▼
                      FAILURE MEMORY
                            │
                            └──────► SELF-LEARNING

         GUARDIAN / PROTECTED CORE
         surrounds the entire chain
```

## 55.2 V2 authority rule

> **Autonomy is granted for investigation, not for self-granted authority.**

The system is allowed to become more capable at answering research questions. It is not allowed to redefine what it is permitted to do.

## 55.3 Supremacy order

When two subsystems disagree, the following priority order applies:

```text
1. Emergency / Safety Invariants
2. Guardian / Protected Core
3. Human Governance Policy
4. Runtime Risk Controls
5. Evidence / Validation Rules
6. Production State
7. Research Objectives
8. Evolution Objectives
9. Efficiency / Optimization Objectives
```

Lower layers may not override higher layers.

---

# V2-02. NO-AI ARCHITECTURAL INVARIANT

## 56.1 Current system definition

The current project **does not contain AI as a required architectural component**. This is a deliberate design constraint.

The system may use deterministic mathematics, statistical tests, state machines, rule systems, optimization procedures, search algorithms, version comparison, controlled experimentation, and other explicitly specified algorithms without becoming an AI-dependent system.

The project must not silently introduce an LLM, autonomous AI agent, generative model, or external AI service into the core runtime, research authority, evaluator, approval system, or safety boundary.

## 56.2 Self-learning without AI

Self-learning means: 

```text
OBSERVATION
   +
OUTCOME
   +
CONTEXT
   +
EXPERIMENT HISTORY
   +
FAILURE HISTORY
   +
HUMAN DECISION HISTORY
        │
        ▼
KNOWLEDGE UPDATE
```

It does **not** mean:

```text
AI MODEL → RETRAIN → ALTER PRODUCTION
```

## 56.3 Future AI separation rule

If AI is ever considered in a future research branch, that branch must begin as a separately documented architectural proposal. It must have its own threat model, trust model, sandbox, evidence protocol, and human approval. It must not be introduced merely because a research engine discovers that AI could generate candidates faster.

---

# V2-03. SELF-LEARNING — FORMAL DEFINITION

## 57.1 Learning vs evolution

The project now treats two concepts separately.

### Learning

> The system acquires, structures, tests, updates, scopes, and ages knowledge from observed outcomes and research activity.

### Evolution

> The system uses sufficiently supported knowledge to formulate bounded hypotheses, create candidate changes, test them, and produce an evidence package for governance.

The canonical separation is:

```text
LEARNING
Observation → Comparison → Pattern → Knowledge

EVOLUTION
Knowledge → Hypothesis → Experiment → Candidate → Validation
```

A learned fact does not automatically become a configuration change.

## 57.2 Learning loop

```text
OBSERVE
  ↓
RECORD
  ↓
COMPARE EXPECTED / ACTUAL
  ↓
DETECT REPEATED PATTERN OR DEVIATION
  ↓
CHECK EVIDENCE QUALITY
  ↓
CREATE / UPDATE KNOWLEDGE OBJECT
  ↓
SCOPE BY CONTEXT
  ↓
MEASURE CONFIDENCE
  ↓
CHECK CONTRADICTIONS
  ↓
AGE / REVALIDATE WHEN REQUIRED
  ↓
AVAILABLE TO HYPOTHESIS ENGINE
```

## 57.3 Learning must be reversible

Knowledge updates must be versioned. Historical knowledge must not be silently overwritten. A newer conclusion can supersede a previous conclusion, but the lineage between them must remain visible.

---

# V2-04. SELF-LEARNING SOURCES

The learning engine may use the following evidence streams.

## 58.1 Market observation learning

```text
Ticks
Bars
Timeframe States
Features
Structure
Regime
Market Quality
Macro/Event State
```

The observation record must preserve the information set that was actually available at the decision timestamp.

## 58.2 Outcome learning

For every prediction, signal, shadow position, and relevant system decision, the learning engine can compare:

```text
EXPECTED
   vs
ACTUAL
```

The comparison may produce:

```text
Deviation
MFE
MAE
Timing Error
Direction Error
Magnitude Error
Execution Error
Risk Error
Context Mismatch
```

A deviation alone is not proof that the strategy is wrong. The learning engine must investigate context and recurrence.

## 58.3 Failure learning

Failures become structured records:

```text
Failure ID
Failure Class
First Seen
Last Seen
Affected Versions
Affected Components
Market Context
System Context
Observed Symptom
Evidence
RCA Hypotheses
Confirmed Cause (if established)
Rejected Explanations
Recurrence
Prevention Actions
```

## 58.4 Experiment learning

The system learns from: 

```text
Hypothesis
Experiment Design
Search Space
Trial Count
Control
Candidate
Validation Protocol
Outcome
Rejection
Unexpected Effects
Reproducibility
```

A failed experiment may be highly informative and must remain available for future research.

## 58.5 Context learning

Knowledge must be conditioned on relevant context rather than treated as universally true.

Possible context dimensions include:

```text
Market Regime
Volatility State
Spread State
Session
Time of Day
Event / Shock State
Liquidity Quality
Execution Quality
Trend / Range Structure
Timeframe Interaction
Broker Profile
```

## 58.6 Human-decision learning

When the user approves or rejects a change proposal, the system records:

```text
Proposal
Evidence Snapshot
Human Decision
Decision Reason
Timestamp
Candidate Version
Future Outcome
```

Human decisions become governance history. They do not automatically become hard labels for a learning model or direct rules. The system must avoid assuming that a human rejection means the underlying hypothesis was mathematically false; the rejection may reflect risk, uncertainty, timing, scope, or policy.

## 58.7 Research-process learning

The system may also learn which research procedures tend to produce: 

```text
Robust results
Fragile results
Inconclusive results
Data leakage
Repeated duplicates
High trial burden
Low information gain
```

This creates a future path toward **Research-Method Evolution** without granting the research engine the ability to rewrite the trusted evaluator.

---

# V2-05. KNOWLEDGE OBJECT MODEL

The unit of self-learning is not a raw metric. It is a **Knowledge Object**.

## 59.1 Canonical structure

```text
KnowledgeObject
{
    knowledge_id,
    observation,
    context,
    evidence_refs[],
    hypothesis_refs[],
    experiment_refs[],
    related_failures[],
    conclusion,
    confidence,
    validity_scope,
    status,
    contradictory_refs[],
    first_observed_at,
    last_supported_at,
    revalidation_due_at,
    lineage_refs[],
    human_decision_refs[]
}
```

## 59.2 Example

```text
K-001

Observation:
M15 prediction error rate rises during Regime X.

Context:
High volatility + widened spread.

Evidence:
E12, E19, E27, E31.

Hypothesis:
Feature F loses reliability under this context.

Experiments:
E40, E41, E42.

Conclusion:
Partially supported.

Validity Scope:
Regime X + spread above measured threshold.

Confidence:
Moderate.

Status:
SUPPORTED_FOR_RESEARCH
```

The example is deliberately scoped. It does not claim that Feature F is always bad.

---

# V2-06. KNOWLEDGE LIFECYCLE

A knowledge statement may move through the following states:

```text
OBSERVED
   ↓
SUSPECTED
   ↓
UNDER_INVESTIGATION
   ↓
SUPPORTED
   ↓
VALIDATED
   ↓
OPERATIONAL_KNOWLEDGE
```

Alternative branches:

```text
SUSPECTED ─────────────→ REFUTED
SUPPORTED ─────────────→ CONTRADICTED
VALIDATED ─────────────→ AGING
AGING ─────────────────→ REVALIDATION
```

## 60.1 State semantics

### OBSERVED
Direct observation exists, but no generalization is claimed.

### SUSPECTED
A repeated or meaningful pattern is visible, but not sufficiently verified.

### UNDER_INVESTIGATION
An explicit research effort exists.

### SUPPORTED
Evidence supports the statement under a defined scope.

### VALIDATED
The statement survives the validation protocol applicable to its claim class.

### OPERATIONAL_KNOWLEDGE
The system is permitted to use the knowledge in bounded research prioritization or policy logic, subject to governance.

### REFUTED
Evidence actively contradicts the claim.

### CONTRADICTED
Different evidence sets support incompatible conclusions. Context separation or additional research is required.

### AGING
The evidence may no longer represent current market behavior or current system behavior.

## 60.2 No silent deletion

Refuted or aged knowledge remains in the lineage. It is never erased merely because it is inconvenient.

---

# V2-07. KNOWLEDGE CONTRADICTION ENGINE

Contradiction is a first-class research state.

The engine should detect patterns such as:

```text
Knowledge A:
Feature X is unreliable in Regime R.

Knowledge B:
Feature X is reliable in Regime R under Session S.
```

The correct response is not to overwrite A with B.

The engine creates a contradiction object:

```text
Contradiction ID
Knowledge A
Knowledge B
Shared Scope
Conflict Dimension
Evidence Difference
Potential Context Split
Required Experiment
Status
```

Possible outcomes:

```text
CONTEXT_RESOLVED
EVIDENCE_WEIGHTED
RESEARCH_REQUIRED
TRUE_CONTRADICTION
UNRESOLVED
```

The contradiction engine prevents the system from treating the latest observation as automatically authoritative.

---

# V2-08. KNOWLEDGE CONFIDENCE MODEL

Confidence is a structured evidence assessment. It is not certainty and is not a probability unless separately calibrated.

The evidence record should consider:

```text
Sample Size
Evidence Quality
Replication
Validation Strength
Regime Coverage
Context Coverage
Contradictory Evidence
Recency
Selection Burden
Data Quality
```

A recommended conceptual representation is:

```text
Confidence
   =
Evidence Strength
   + Replication
   + Scope Coverage
   + Validation
   - Contradiction
   - Selection Burden
   - Data Uncertainty
```

This equation is conceptual, not a fixed numerical implementation. Exact calibration remains a research parameter.

## 62.1 Confidence rules

```text
Confidence ≠ Probability
Confidence ≠ Guarantee
Confidence ≠ Production Permission
High Confidence ≠ Automatic Deployment
```

---

# V2-09. KNOWLEDGE DECAY AND REVALIDATION

Market behavior, broker behavior, execution quality, and strategy behavior can change. Therefore operational knowledge must have an aging mechanism.

A knowledge object may define:

```text
Last Supported Time
Validity Horizon
Required Revalidation Trigger
Minimum Refresh Evidence
Context Drift Trigger
```

Revalidation can be triggered by:

```text
Regime Distribution Shift
Large Performance Degradation
Broker Profile Change
Feature Definition Change
Execution Model Change
Long Time Since Validation
Repeated Contradiction
```

Aged knowledge is not erased. It becomes a research item again.

---

# V2-10. LEARNING-TO-EVOLUTION GATE

The system must not perform:

```text
New Knowledge → Immediate Configuration Mutation
```

Instead:

```text
New Knowledge
   ↓
Evidence Sufficiency Check
   ↓
Scope Check
   ↓
Contradiction Check
   ↓
Hypothesis Generation
   ↓
Experiment
   ↓
Validation
   ↓
Candidate
   ↓
Human Governance
```

## 64.1 Evidence sufficiency

A knowledge object may become eligible for hypothesis generation only when its status, evidence quality, and scope satisfy the policy associated with that class of claim.

## 64.2 No automatic escalation of authority

A knowledge object may gain evidence support. It may not gain permissions.

---

# V2-11. HYPOTHESIS ENGINE V2

The Hypothesis Engine converts evidence-backed knowledge into falsifiable research questions.

## 65.1 Problem classification

Before forming a hypothesis, the engine classifies the observed issue:

```text
DATA
SCHEMA
CLOCK
FEATURE
REGIME
ELIGIBILITY
SIGNAL
TIMING
RISK
EXECUTION
RECONCILIATION
PERSISTENCE
RESEARCH_METHOD
UNKNOWN
```

This prevents the engine from treating every problem as a strategy problem.

## 65.2 Hypothesis classes

```text
FEATURE_HYPOTHESIS
PARAMETER_HYPOTHESIS
RULE_HYPOTHESIS
REGIME_HYPOTHESIS
TIMING_HYPOTHESIS
RISK_HYPOTHESIS
EXECUTION_HYPOTHESIS
INTERACTION_HYPOTHESIS
STRUCTURAL_HYPOTHESIS
DATA_QUALITY_HYPOTHESIS
RESEARCH_METHOD_HYPOTHESIS
```

## 65.3 Hypothesis contract

```text
Hypothesis ID
Parent Knowledge IDs
Problem Class
Claim
Affected Component
Context Scope
Expected Effect
Success Criterion
Failure Criterion
No-Effect Criterion
Required Evidence
Experiment Design
Maximum Trials
Maximum Candidates
Known Risks
Related Previous Experiments
```

## 65.4 Falsifiability

Every material hypothesis must state what evidence could make it fail.

Example:

```text
CLAIM:
Filtering signals when spread exceeds S will reduce execution-related degradation.

SUPPORT:
Improved out-of-sample outcome quality without unacceptable loss of valid opportunities.

FAILURE:
No improvement, improvement limited to exploration data, or material regression in protected slices.
```

The engine is not allowed to quietly redefine failure after seeing results.

---

# V2-12. RESEARCH PLANNER V2

The Research Planner transforms hypotheses into bounded campaigns.

## 66.1 Campaign contract

```text
Campaign ID
Hypothesis ID
Research Goal
Control Version
Candidate Search Space
Dataset Versions
Feature Versions
Evaluator Version
Metric Registry Version
Validation Protocol
Trial Budget
Candidate Budget
Holdout Budget
Compute Budget
Expected Completion State
Stop Conditions
```

## 66.2 Required pre-registration

Before material experimentation begins, the planner must record:

```text
What is being tested?
Why?
What data will be used?
What remains locked?
What constitutes success?
What constitutes failure?
What constitutes inconclusive evidence?
How many trials are allowed?
What is the maximum scope of change?
```

## 66.3 Do-not-optimize rule

If the research problem is diagnosis, the planner must not automatically convert it into optimization.

The preferred order remains:

```text
Diagnose → Explain → Hypothesize → Experiment → Optimize
```

---

# V2-13. SELF-EVOLUTION ENGINE V2

## 67.1 Candidate generation

The Evolution Engine may produce:

```text
Parameter Candidate
Rule Candidate
Feature Candidate
Regime Candidate
Strategy Candidate
Research-Method Candidate
```

Architecture changes remain a separately governed research class.

## 67.2 Parent rule

Each candidate must have one explicit parent version. Multiple research branches are represented by separate candidate objects.

## 67.3 Candidate population

When computationally reasonable, the engine should maintain a **diverse population** rather than a single greedy successor. This supports branch diversity and reduces the risk that one early error dominates all future evolution.

## 67.4 Candidate isolation

Candidate code/configuration must run in a sandbox or equivalent restricted execution environment. Candidate processes must not receive arbitrary production credentials or direct production write access.

## 67.5 Candidate trust state

Every candidate begins as:

```text
UNTRUSTED
```

and can progress through: 

```text
GENERATED
→ SANDBOXED
→ VALIDATED
→ REVIEW_READY
→ HUMAN_APPROVED
→ SHADOW
→ PROMOTION_READY
→ DEPLOYED
```

A candidate may also become:

```text
REJECTED
FAILED
CONTAMINATED
ROLLED_BACK
EXPIRED
```

---

# V2-14. EVOLUTION BUDGET V2

The project treats research capacity as both an engineering resource and a statistical resource.

## 68.1 Budget classes

```text
Compute Budget
Experiment Budget
Trial Budget
Candidate Budget
Structural Mutation Budget
Holdout Query Budget
Validation Run Budget
Branch Budget
Research-Window Budget
Network Budget
Storage Budget
```

## 68.2 Budget non-escalation

The research engine cannot modify its own maximum research budget.

If the budget is exhausted:

```text
BUDGET EXCEEDED
      ↓
RESEARCH PAUSED
      ↓
AUDIT
      ↓
WAIT FOR NEXT APPROVED WINDOW / BUDGET
```

## 68.3 Search burden

The system records:

```text
Number of Hypotheses
Number of Experiments
Number of Trials
Number of Candidates
Number of Rejected Candidates
Number of Holdout Queries
Number of Adaptive Decisions
```

These values must be available to the evidence evaluator because repeated search changes the evidentiary context.

---

# V2-15. EXPERIMENT LEDGER V2

The ledger is the scientific memory of the research engine.

## 69.1 Minimum record

```text
Experiment ID
Campaign ID
Hypothesis ID
Failure ID
RCA ID
Actor
Timestamp
Parent Version
Control Version
Candidate Version(s)
Dataset Version(s)
Feature Version(s)
Parameter Set
Search Space
Trial Number
Random Seed Policy
Evaluator Version
Metric Registry Version
Environment Version
Code Digest
Validation Protocol
Holdout Exposure
Metrics
Stress Results
Regression Results
Decision
Rejection Reason
Unexpected Effects
Contamination State
```

## 69.2 Append-only requirement

Historical experiment records are append-only. Corrections create a correction event or new version; they do not silently rewrite the original event.

## 69.3 Experiment fingerprint

The system should compute an experiment fingerprint from the material inputs:

```text
Hash(
    Parent Version
    + Hypothesis
    + Dataset
    + Features
    + Search Space
    + Code Digest
    + Evaluator
    + Environment
)
```

Exact duplicates may be rejected. Near duplicates should produce a warning and a reference to the related experiments. Scientific replication remains allowed when explicitly justified.

---

# V2-16. VALIDATION FIREWALL V2

The Validation Firewall is a trust boundary, not just a collection of metrics.

## 70.1 Evidence layers

```text
E0 — EXPLORATION
E1 — DEVELOPMENT / VALIDATION
E2 — OUT-OF-SAMPLE
E3 — LOCKED HOLDOUT
E4 — SHADOW / BEHAVIORAL VALIDATION
```

The exact test suite depends on the claim and change class.

## 70.2 Candidate does not select its own proof

The candidate does not choose the evaluator version, change metric definitions after the fact, or select a favorable validation partition.

## 70.3 Required validation families where applicable

```text
Leakage / Availability Checks
Point-in-Time Checks
Purged / Time-Aware Validation
Walk-Forward Analysis
CPCV / CSCV where justified
PBO / Multiple-Testing Analysis where justified
Deflated Sharpe Ratio where justified
Reality Check / SPA where applicable
Monte Carlo / Stress Tests
Cost / Spread / Slippage Stress
Regime Coverage
Parameter Sensitivity
Regression Tests
Locked OOS
Shadow Comparison
```

No single statistical test proves robustness for all candidate types.

---

# V2-17. EVIDENCE FIREWALL AND LOCKED HOLDOUT

## 71.1 Final holdout as a service boundary

The preferred architecture is:

```text
RESEARCH ENGINE
    │
    ├── Candidate Hash
    ├── Experiment Definition
    ├── Evaluator Version
    │
    ▼
LOCKED HOLDOUT SERVICE
    │
    └── Approved Metrics Only
```

The raw final holdout should not be copied into the research workspace.

## 71.2 Holdout budget

Every final-holdout query is logged. The system may define a query budget and a contamination rule.

## 71.3 Contamination

Once a candidate has used a protected holdout in a way that can influence subsequent optimization, the candidate is marked contaminated for that evidence cycle.

## 71.4 No feedback loop

The research engine must not receive raw holdout content and then simply adapt until the result improves.

---

# V2-18. EVALUATOR FIREWALL V2

The evaluator is part of the trusted evidence plane.

## 72.1 Protected components

Research / candidate code must not modify:

```text
Evaluator Root
Metric Definitions during a campaign
Validation Rules
Holdout Service
Audit Logger
Experiment Ledger
Approval Gate
Promotion Gate
Rollback Mechanism
Guardian
Emergency Stop
Known-Good Registry
Security Policy
Risk Limits
```

## 72.2 Evaluator versioning

Every evidence result must record the exact evaluator version. Any material evaluator change begins a new evidence family.

## 72.3 Evaluator red-team

Evaluator behavior itself should periodically be tested for obvious exploitation paths, including:

```text
Metric Gaming
Hidden Side Effects
Tolerance Abuse
Boundary Exploitation
Data-Selection Exploitation
Self-Scoring

```

The evaluator must be harder to manipulate than the candidate.

---

# V2-19. CONTROL VS CANDIDATE — V2

Every material candidate should be evaluated against a defined control.

```text
CONTROL
Current Approved Version

CANDIDATE
Research Version
```

They should be compared under matched conditions:

```text
Same Data Slice
Same Execution Assumptions
Same Evaluator
Same Environment where possible
Same Time Window
Same Cost Model
Same Risk Constraints
Same Validation Protocol
```

The comparison should expose: 

```text
Absolute Difference
Relative Difference
Regression Set
Regime-Specific Difference
Tail Behavior
Opportunity Loss / Gain
Cost Difference
Robustness Difference
Unexpected Effects
```

---

# V2-20. REWARD-HACKING / OBJECTIVE-GAMING DEFENSE

Even without AI, any optimization system can optimize an imperfect objective. Therefore the project retains the following rule:

> **Constraints are vetoes. Metrics are objectives.**

A candidate should not be accepted merely because one scalar score improved.

## 74.1 Constraint classes

```text
Safety Constraints
Risk Constraints
Data Integrity Constraints
Leakage Constraints
Execution Constraints
Operational Constraints
Audit Constraints
Governance Constraints
```

## 74.2 Objective classes

Within the allowed constraint envelope, the research engine may study:

```text
Predictive Quality
Robustness
Stability
Efficiency
Opportunity Quality
Execution Quality
Research Efficiency
```

## 74.3 Forbidden objective mutation

The system may not change the definition of “success” after seeing a result.

---

# V2-21. HUMAN GOVERNANCE V2

## 75.1 Human authority

The human owns final authority over production-impacting change.

The system may provide evidence. It may not manufacture authority from confidence.

## 75.2 What the system may do autonomously

```text
Observe
Record
Compare
Detect anomalies
Detect degradation
Classify failures
Run bounded RCA
Generate hypotheses
Design experiments
Run sandbox research
Generate candidate versions
Reject candidates by predefined objective gates
Run validation
Run stress tests
Build reports
Rank research priorities
Enter safe mode
Rollback to known-good state inside predefined policy
Continue observation
Conclude NO CHANGE
```

## 75.3 What requires human approval

At minimum:

```text
Production Strategy Change
Production Risk-Rule Change
Production Configuration Change
Activation of a New Strategy Family
Promotion of a Candidate
Material Architecture Change
Change to Protected Authority
Change to Guardian Policy
Change to Operating Window Policy
Change to Final-Holdout Policy
Change to Evaluator Authority
```

## 75.4 Human decision is not a hidden side effect

Every decision must create an explicit immutable governance record.

---

# V2-22. CHANGE INVESTIGATION REPORT V2

The primary repair / self-improvement proposal presented to the user is a **Change Investigation Report**.

## 76.1 Required report structure

```text
CHANGE PROPOSAL

Proposal ID
Created At
First Evidence At
Current Version
Candidate Version
Affected Component
Problem Class

1. WHAT WAS DETECTED?
2. WHEN DID IT APPEAR?
3. HOW WAS IT DETECTED?
4. HOW WAS IT VERIFIED?
5. WHAT IS THE CURRENT BEHAVIOR?
6. WHAT IS PROPOSED TO CHANGE?
7. WHY IS THIS CHANGE BEING PROPOSED?
8. EXPECTED EFFECT
9. NEGATIVE EFFECTS / RISKS
10. CONTROL VS CANDIDATE
11. VALIDATION RESULTS
12. ROBUSTNESS / STRESS
13. REGIME / CONTEXT IMPACT
14. DATA / PREDICTION / EXPERIMENT REFERENCES
15. UNKNOWN / INCONCLUSIVE AREAS
16. ROLLBACK PLAN
17. REQUIRED HUMAN DECISION
```

## 76.2 Decision states

```text
APPROVE
REJECT
REQUEST_MORE_RESEARCH
MODIFY_PROPOSAL
FREEZE
```

## 76.3 Rejection reason

A rejection is itself a research object. The application must preserve: 

```text
Proposal ID
Candidate
Evidence Snapshot
Human Decision
Reason
Timestamp
Result of Current Version
```

---

# V2-23. DESKTOP APPLICATION — PRIMARY CONTROL CENTER

This is a key Version 2.0 change.

## 77.1 Primary rule

> **The desktop application is the system’s primary human interface and control center.**

It owns the authoritative presentation of:

```text
Market Views
Predictions
Outcomes
Research
Knowledge
Failures
Hypotheses
Experiments
Candidates
Validation
Approvals
Versions
Evolution Graph
Evidence Graph
Incidents
Rollback
Audit
System Health
Operating Window
Resource Budgets
Configuration
```

## 77.2 Application as source of control

The application communicates with the trusted control plane. It does not become authoritative merely because it has a UI. The source of truth remains the protected services and persistent ledger behind it.

## 77.3 Required application domains

```text
HOME / SYSTEM STATUS
MARKET
PREDICTIONS
OUTCOMES
RESEARCH
KNOWLEDGE
FAILURES / RCA
HYPOTHESES
EXPERIMENTS
CANDIDATES
VALIDATION
CHANGE PROPOSALS
APPROVAL CENTER
EVOLUTION GRAPH
EVIDENCE GRAPH
VERSIONS
SHADOW / CHALLENGER
MONITORING
INCIDENTS
ROLLBACK
AUDIT
OPERATING WINDOW
RESOURCE BUDGETS
SYSTEM SETTINGS
```

## 77.4 One application, one story

A user should not need Telegram to understand why the system changed, what it learned, or what it wants approved. Telegram may notify or assist; the application contains the complete context.

---

# V2-24. TELEGRAM — AUXILIARY ASSISTANT LAYER

Telegram remains useful, but it is not the system’s foundation.

## 78.1 Principle

```text
DESKTOP APPLICATION
       = PRIMARY

TELEGRAM
       = OPTIONAL AUXILIARY
```

## 78.2 Optional assistant roles

The system may expose different Telegram assistants, for example:

```text
BOT A — Operations / Monitoring
• Market status
• Prediction snapshots
• Alerts
• Health
• Research completion notifications

BOT B — Governance Assistant
• New change proposals
• Candidate ready notifications
• Validation summaries
• Approval reminders
• Incident alerts
```

These are interfaces, not independent authority planes.

## 78.3 No Telegram bypass

A Telegram command must never bypass:

```text
Authorization
Candidate Identity Verification
Hash Verification
Policy Checks
Approval Gate
Audit Record
Promotion Gate
```

## 78.4 Core availability

If Telegram is unavailable, the core system remains safe and fully operational within the permissions available to the desktop application and control plane.

---

# V2-25. TWO-BOT GOVERNANCE SEPARATION

Where Telegram assistants are used, operations and governance should remain logically separate.

```text
BOT 1 — OPERATIONS
   ↓
Read-oriented status / reports / alerts

BOT 2 — GOVERNANCE
   ↓
Proposal / approval interaction
   ↓
Trusted Approval Service
```

The governance bot still does not directly edit production. It submits an authenticated governance event to the trusted approval service.

---

# V2-26. GUARDIAN / SELF-PRESERVATION CORE

## 80.1 Purpose

The Guardian exists to answer a question that ordinary evolution does not answer:

> **How does the system prevent its own research process from destroying the system?**

The Guardian is the highest-level operational safety component below the human owner.

## 80.2 Guardian responsibilities

```text
Monitor authority boundaries
Enforce operating window
Enforce resource ceilings
Verify protected component integrity
Protect evaluator root
Protect approval gate
Protect rollback path
Protect known-good registry
Detect corruption
Trigger safe mode
Trigger emergency stop
Authorize only predefined recovery actions
Verify recovery checkpoints
```

## 80.3 Guardian is not self-evolving

The Evolution Engine cannot modify the Guardian.

A Guardian change requires a separate human-governed architecture change process.

## 80.4 Fail-closed behavior

If the Guardian cannot establish the integrity of a critical safety dependency, the system should prefer:

```text
SAFE MODE / HALT
```

over ambiguous autonomous continuation.

## 80.5 Guardian watch domains

```text
Authority
Integrity
Time / Schedule
Resource
Process Health
Persistence
Audit
Version State
Rollback State
Policy State
```

---

# V2-27. SELF-DESTRUCTION / ANTI-CORRUPTION MODEL

The architecture explicitly defends against the following failure families.

## 81.1 Code corruption

Candidate code may compile or run while still violating expected behavior. Static checks, sandbox tests, regression tests, and integrity checks must therefore be layered.

## 81.2 Degradation loops

A weak candidate can become the parent of another weak candidate. The system prevents this by requiring approval and validation state before a candidate can become the authoritative parent for future production-oriented evolution. Research branches may continue from unapproved states, but they remain labeled as research branches.

## 81.3 Evaluator gaming

The evaluator is isolated and versioned. A candidate cannot rewrite the metric that measures it.

## 81.4 Memory corruption

Historical evidence is append-only and integrity-protected.

## 81.5 Evolutionary drift

The system tracks changes over generations. It should detect patterns such as:

```text
ROBUSTNESS ↓
COMPLEXITY ↑
UNCERTAINTY ↑
TRIAL COUNT ↑
```

When the predefined anti-drift threshold is violated:

```text
FREEZE EVOLUTION
→ RETURN TO KNOWN-GOOD
→ CREATE INCIDENT
→ INVESTIGATE
→ UPDATE FAILURE MEMORY
```

## 81.6 Resource runaway

Experiment loops are bounded by research budgets and the daily operating window.

## 81.7 Audit corruption

Research engines do not have permission to erase or rewrite their own audit trail.

---

# V2-28. IMMUTABLE CORE / PROTECTED COMPONENTS

The following should be treated as protected by default:

```text
Guardian
Emergency Stop
Approval Gate
Promotion Gate
Rollback Mechanism
Known-Good Registry
Audit Root
Experiment Ledger Integrity
Evaluator Root
Holdout Service
Risk Hard Limits
Credential Store
Authorization Policy
Operating Window Ceiling
```

Changes to these components require an explicit architecture-change procedure and human approval.

## 82.1 Protected means more than “do not edit”

Protection should be enforced by actual permissions, process separation, signatures/digests, deployment packaging, and ownership boundaries. Relying only on a prompt or code comment is insufficient.

---

# V2-29. KNOWN-GOOD REGISTRY

The system maintains a registry of states that have passed the required promotion criteria.

## 83.1 Known-good object

```text
Version ID
Artifact Digest
Parent Version
Approval Record
Evidence Snapshot
Validation Summary
Deployment History
Rollback Eligibility
Policy Compatibility
Monitoring Profile
```

## 83.2 Rollback rule

Automatic rollback may target only a registered known-good state satisfying the current rollback policy.

The system may not say:

> “I found an older version that looks good.”

and then silently deploy it. The target must already be known to the control plane.

---

# V2-30. INCIDENT → RCA → FAILURE MEMORY → LEARNING

Rollback is not the end of the loop.

```text
FAILURE
  ↓
PROTECTION
  ↓
ROLLBACK
  ↓
INCIDENT
  ↓
TIMELINE
  ↓
RCA HYPOTHESES
  ↓
VERIFICATION
  ↓
POSTMORTEM
  ↓
FAILURE MEMORY
  ↓
KNOWLEDGE UPDATE
  ↓
NEW / UPDATED HYPOTHESIS
```

## 84.1 Incident object

```text
Incident ID
Detected At
Affected Version
Severity
Impact Window
System State
Market Context
Trigger
Rollback Target
Actions Taken
RCA Hypotheses
Confirmed Cause
Unknowns
Preventive Actions
Verification Plan
Human Review
```

## 84.2 Failed hypothesis memory

The system stores not only confirmed causes, but also explanations that were investigated and rejected. This prevents endless rediscovery of the same dead ends.

---

# V2-31. DAILY OPERATING WINDOW — 3 TO 8 HOURS

## 85.1 Core rule

The system operates inside a human-defined daily operating window with: 

```text
MINIMUM ACTIVE DURATION = 3 HOURS
MAXIMUM ACTIVE DURATION = 8 HOURS
```

The configured daily operating period must satisfy:

```text
3h ≤ Daily Operating Window ≤ 8h
```

This is a governance and resource constraint, not merely a user-interface timer.

## 85.2 Human ownership

The window is human-defined. The self-evolution engine cannot:

```text
Extend it
Shift it to gain extra experiments
Create extra hidden windows
Restart itself after shutdown
Bypass the Guardian
Increase the daily ceiling
```

Any policy change must go through the control plane and human approval.

## 85.3 Schedule object

```text
OperatingSchedule
{
    timezone,
    start_time,
    duration,
    minimum_duration,
    maximum_duration,
    active_days,
    runtime_enabled,
    research_enabled
}
```

## 85.4 Separate runtime and research schedules

The architecture supports separate windows: 

```text
SYSTEM
├── RUNTIME PLANE
│   └── Runtime Schedule
└── RESEARCH PLANE
    └── Research Schedule
```

Example:

```text
Runtime: 08:00–16:00
Research: 18:00–23:00
```

The system may support such schedules only when explicitly configured. The important invariant is that every active window remains bounded by policy.

---

# V2-32. OPERATING MODES

The canonical modes are:

```text
OFFLINE
SCHEDULED
STARTING
ACTIVE
PAUSED
DRAINING
SAFE_SHUTDOWN
EMERGENCY_STOP
```

## 86.1 OFFLINE

No active work.

## 86.2 SCHEDULED

Waiting for the approved start boundary.

## 86.3 STARTING

Integrity checks, dependency checks, state recovery checks, and schedule validation are running.

## 86.4 ACTIVE

Normal bounded operations.

## 86.5 PAUSED

New work is temporarily suspended but the process may remain available.

## 86.6 DRAINING

The window is ending. No new noncritical work may begin. Safe critical work is completed or checkpointed.

## 86.7 SAFE_SHUTDOWN

State is persisted, audit records written, resources released, and the process terminates safely.

## 86.8 EMERGENCY_STOP

Critical protection takes precedence. Any recovery path must be governed by predefined safety rules.

---

# V2-33. OPERATING WINDOW STATE MACHINE

```text
OFFLINE
   │
   ▼
SCHEDULED
   │ start boundary
   ▼
STARTING
   │ integrity OK
   ▼
ACTIVE
   │ pause request
   ├───────────────► PAUSED
   │                  │ resume
   │                  └──────────────► ACTIVE
   │
   │ window ending
   ▼
DRAINING
   │ critical work settled / checkpoint saved
   ▼
SAFE_SHUTDOWN
   │
   ▼
OFFLINE
```

Emergency path:

```text
ANY STATE
   ↓ critical safety violation
EMERGENCY_STOP
   ↓
SAFE RECOVERY / HALT
```

---

# V2-34. WINDOW END — CONTROLLED DRAINING

The end of the operating window is **not** implemented as a blind process kill.

## 88.1 Canonical shutdown sequence

```text
OPERATING WINDOW ENDING
        ↓
STOP NEW RESEARCH WORK
        ↓
STOP NEW CANDIDATE GENERATION
        ↓
STOP NEW NONCRITICAL EXPERIMENTS
        ↓
CLASSIFY ACTIVE TASKS
        ↓
FINISH CRITICAL COMMIT / AUDIT OPERATIONS
        ↓
CHECKPOINT RUNNING SAFE-TO-RESUME WORK
        ↓
SAVE SYSTEM STATE
        ↓
WRITE AUDIT
        ↓
VERIFY PERSISTENCE
        ↓
SAFE SHUTDOWN
```

## 88.2 Task classes

### Safe to stop

```text
Generate Hypothesis
Generate Candidate
Start New Experiment
Unstarted Parameter Search
Exploratory Analysis
```

### Safe to checkpoint

```text
Backtest
Stress Test
Validation Run
Monte Carlo Run
Replay
Long Research Job
```

### Critical

```text
Database Commit
Audit Write
Version Registration
Promotion Transition
Rollback Transition
Integrity Verification
Checkpoint Commit
```

Critical tasks must either complete safely or enter an explicitly recoverable state.

---

# V2-35. CHECKPOINT / RESUME ARCHITECTURE

Long-running research must survive the daily boundary and unexpected interruption.

## 89.1 Checkpoint object

```text
Checkpoint ID
Experiment ID
Campaign ID
Parent Version
Candidate Version
Dataset Version
Feature Version
Evaluator Version
Environment Version
Random Seed / RNG State
Processed Range
Progress
Partial Metrics
Pending Work
Created At
Integrity Digest
Status
```

Example:

```text
Experiment E-184
Progress: 67%
Checkpoint: CP-184-07
Dataset: D-22
Processed Range: 2019-01-01 → 2023-06-14
RNG State: recorded
Status: PAUSED
```

## 89.2 Resume contract

A resume may occur only after verifying that:

```text
Checkpoint Integrity = OK
Dataset Identity = Same
Code / Artifact Identity = Compatible
Evaluator Identity = Same
Policy = Compatible
```

Otherwise the job is restarted or abandoned and a reason is recorded.

## 89.3 No partial-result ambiguity

Partial results must be clearly labeled as partial. They cannot be accidentally treated as complete experiments.

---

# V2-36. CRASH RECOVERY

The system must distinguish: 

```text
CLEAN SHUTDOWN
EXPECTED PAUSE
INTERRUPTED WORK
CORRUPTED STATE
UNKNOWN STATE
```

## 90.1 Recovery flow

```text
BOOT
  ↓
Guardian Integrity Check
  ↓
Ledger Integrity Check
  ↓
Detect Interrupted Cycle
  ↓
Load Latest Valid Checkpoint
  ↓
Validate Checkpoint Identity
  ↓
DECIDE
 ┌──────────┬──────────┬──────────┐
 ▼          ▼          ▼
RESUME    RESTART    ABANDON
```

The decision is recorded.

## 90.2 Unknown state

If the system cannot prove that it knows its last valid state, it must not silently continue with production-impacting behavior.

---

# V2-37. RESOURCE GOVERNANCE

The daily window is complemented by resource budgets.

## 91.1 Resource domains

```text
CPU
RAM
Disk
GPU (if ever present)
Network
Concurrent Experiments
Storage Growth
Database I/O
Process Count
```

## 91.2 Research budget object

```text
ResearchBudget
{
    max_runtime_hours,
    max_experiments,
    max_trials,
    max_candidates,
    max_structural_mutations,
    max_holdout_queries,
    max_concurrent_jobs,
    max_cpu_share,
    max_memory_mb,
    max_disk_growth_mb,
    max_network_mb
}
```

## 91.3 Resource runaway prevention

If a research process exceeds a predefined resource boundary:

```text
THROTTLE
→ PAUSE
→ CHECKPOINT
→ ALERT
```

The process may not increase its own budget.

---

# V2-38. DAILY LEARNING CYCLE

The daily operating window becomes an auditable research unit.

```text
START WINDOW
   ↓
INTEGRITY / RECOVERY
   ↓
OBSERVE NEW DATA
   ↓
EVALUATE OUTCOMES
   ↓
UPDATE KNOWLEDGE
   ↓
GENERATE RESEARCH PRIORITIES
   ↓
FORM HYPOTHESES
   ↓
RUN BOUNDED EXPERIMENTS
   ↓
VALIDATE
   ↓
UPDATE KNOWLEDGE / FAILURE MEMORY
   ↓
PREPARE CANDIDATES
   ↓
HUMAN REVIEW PACKAGE (if ready)
   ↓
CHECKPOINT
   ↓
DRAIN
   ↓
SAFE SHUTDOWN
```

## 92.1 Learning-cycle record

```text
Learning Cycle ID
Date
Window Start
Window End
Actual Runtime
Observations
New Knowledge Objects
Updated Knowledge Objects
Contradictions Found
Hypotheses Created
Experiments Started
Experiments Completed
Candidates Generated
Candidates Rejected
Candidates Review-Ready
Holdout Queries
Human Decisions
Incidents
Rollbacks
Checkpoints
Shutdown Status
```

---

# V2-39. RESEARCH EFFICIENCY

The system must not define success only through simulated trading returns. A major output of the research engine is **information gained per unit of research budget**.

Useful measurements include:

```text
Research Hours
Experiments / Hour
Useful Findings / Hour
Validated Knowledge / Hour
Unique Hypotheses / Hour
Rejected Hypotheses / Hour
Average Trial Burden
Duplicate Rate
Inconclusive Rate
Rollback Rate
Research-to-Review Ratio
Review-to-Approval Ratio
```

These are process metrics, not performance guarantees.

A system that rejects 95% of weak candidates may be performing useful research.

---

# V2-40. HUMAN REVIEW CENTER — APPLICATION UX CONTRACT

The Approval Center should compress evidence, not hide it.

## 94.1 Review card

Every review-ready candidate should expose:

```text
Candidate ID
Parent Version
Problem
Hypothesis
What Changed
What Did Not Change
Control Metrics
Candidate Metrics
Validation Summary
Stress Summary
Regression Summary
Regime Impact
Known Risks
Unknowns
Trial Count
Holdout Exposure
Rollback Target
Integrity Status
Provenance
```

## 94.2 Decision controls

```text
APPROVE
REJECT
REQUEST MORE RESEARCH
FREEZE
VIEW FULL EVIDENCE
COMPARE
OPEN EVOLUTION GRAPH
OPEN INCIDENT HISTORY
```

Approval should never be represented by a simple “green badge” without access to the underlying evidence.

---

# V2-41. HUMAN DECISION LEARNING LOOP

```text
CHANGE PROPOSAL
      ↓
HUMAN REVIEW
      ↓
APPROVE / REJECT / MORE RESEARCH
      ↓
DECISION RECORD
      ↓
LATER OUTCOME
      ↓
COMPARE DECISION CONTEXT WITH OUTCOME
      ↓
UPDATE GOVERNANCE KNOWLEDGE
```

## 95.1 Example

A candidate may be rejected because the evidence is too narrow, not because the change is known to be wrong. The knowledge engine must preserve that distinction.

---

# V2-42. EVOLUTION GRAPH V2

The Evolution Graph records version lineage.

```text
                         V1.0
                      /    |    \
                     /     |     \\
                 V1.1-A  V1.1-B  V1.1-C
                   |         |      |
                REJECT     TEST     ACCEPT
                             |        |
                           V1.2-R    V1.2
```

## 96.1 Node types

The graph should support:

```text
Version
Hypothesis
Experiment
Candidate
Validation Run
Human Decision
Deployment
Incident
Rollback
Knowledge Object
```

## 96.2 Edge types

```text
PARENT_OF
DERIVED_FROM
TESTED_BY
SUPPORTED_BY
REJECTED_BY
APPROVED_BY
DEPLOYED_AS
ROLLED_BACK_TO
CAUSED
CONTRADICTS
SUPERSEDES
REVALIDATES
```

---

# V2-43. EVIDENCE GRAPH V2

Evolution tells the project where a version came from. Evidence tells the project why the transition was justified.

```text
VERSION
  ↓
HYPOTHESIS
  ↓
EXPERIMENT
  ↓
RESULT
  ↓
VALIDATION
  ↓
DECISION
  ↓
VERSION
```

A production transition should therefore be reconstructable as an evidence path rather than a single approval timestamp.

---

# V2-44. PROVENANCE V2

Every material candidate and deployed version should be traceable to:

```text
Code / Configuration Artifact
Dataset Versions
Feature Versions
Environment
Dependencies
Evaluator Version
Metric Registry Version
Research Campaign
Hypothesis
Human Decision
Deployment Event
Rollback Target
```

The system should preserve content digests where practical.

---

# V2-45. PERMISSION MATRIX V2

| Component | Read Research | Write Research | Create Candidate | Run Validation | Approve | Deploy | Rollback | Modify Guardian |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Observation Engine | YES | YES | NO | NO | NO | NO | NO | NO |
| Knowledge Engine | YES | YES | NO | NO | NO | NO | NO | NO |
| Hypothesis Engine | YES | YES | NO | NO | NO | NO | NO | NO |
| Experiment Engine | YES | YES | YES | YES | NO | NO | NO | NO |
| Candidate Generator | YES | YES | YES | NO | NO | NO | NO | NO |
| Evaluator | YES | OWN DOMAIN | NO | YES | NO | NO | NO | NO |
| Human Governance | YES | AUDIT ONLY | REVIEW | REVIEW | YES | REQUEST | REQUEST | REQUEST |
| Promotion Manager | YES | CONTROL PLANE | NO | VERIFY | NO | YES, GATED | NO | NO |
| Rollback Manager | YES | AUDIT | NO | VERIFY | NO | NO | YES, BOUNDED | NO |
| Guardian | YES | PROTECTED | NO | VERIFY | NO | NO | YES, BOUNDED | NO |
| Desktop Application | DISPLAY | VIA CONTROL PLANE | REQUEST | REQUEST | USER ACTION | REQUEST | REQUEST | REQUEST |
| Telegram Assistant | LIMITED | VIA CONTROL PLANE | REQUEST | REQUEST | REQUEST | REQUEST | REQUEST | NO |

The matrix is conceptual. Actual operating-system permissions and service accounts must enforce the boundaries.

---

# V2-46. HARD-FORBIDDEN V2 BEHAVIORS

The current architecture forbids the following actions by self-learning / research / evolution processes:

```text
1. Direct Production Mutation
2. Self-Modification of Guardian
3. Self-Modification of Approval Gate
4. Self-Modification of Rollback Mechanism
5. Self-Modification of Emergency Stop
6. Self-Modification of Known-Good Registry
7. Deletion of Failed Experiments
8. Deletion of Rejected Candidates
9. Rewriting Historical Audit Records
10. Unlocking Raw Final Holdout
11. Changing Success Criteria After Results
12. Changing Evaluator to Pass a Candidate
13. Changing Metric Definitions Mid-Campaign
14. Increasing Own Research Budget
15. Extending Own Operating Window
16. Creating Hidden Operating Windows
17. Increasing Own Authority
18. Granting Own Credentials
19. Bypassing Human Approval
20. Auto-Promoting an Unknown Candidate
21. Returning an Unregistered Rollback Target
22. Erasing Provenance
23. Hiding Contamination
24. Suppressing Failure Signals
25. Manipulating Timestamps
26. Using Future Information
27. Treating Confidence as Permission
28. Treating a Single Backtest as Proof
29. Treating Human Approval as Scientific Truth
30. Declaring “NO MORE RESEARCH NEEDED” as an irreversible state
31. Introducing AI into the core without a separately approved architecture change
```

---

# V2-47. ALLOWED AUTONOMOUS BEHAVIORS V2

The system may autonomously:

```text
Observe
Record
Validate Data
Compare
Detect Anomalies
Detect Degradation
Classify Failures
Run Bounded RCA
Build Knowledge Objects
Track Contradictions
Age Knowledge
Generate Falsifiable Hypotheses
Plan Experiments
Run Sandboxed Experiments
Generate Candidates
Run Predefined Validation
Reject Candidates by Predefined Gates
Run Stress Tests
Detect Regression
Build Change Reports
Prioritize Research Problems
Maintain Evolution / Evidence Graphs
Create Checkpoints
Resume Compatible Checkpoints
Enter Safe Mode
Rollback to Approved Known-Good State under Predefined Conditions
Notify Human
Conclude NO CHANGE
Continue Observation
```

---

# V2-48. NO-CHANGE / INSUFFICIENT-EVIDENCE ENGINE

One of the strongest protections against runaway evolution is the ability to stop searching for a change.

The system may conclude:

```text
NO_CHANGE
INSUFFICIENT_EVIDENCE
CONTRADICTORY_EVIDENCE
RESEARCH_TOO_EXPENSIVE
REGRESSION_TOO_HIGH
NO_REPRODUCIBLE_EFFECT
CURRENT_VERSION_ACCEPTABLE
WAIT_FOR_MORE_DATA
```

This is a valid output, not a failure.

## 102.1 Forced-change prohibition

The research engine must never assume that every problem requires a code change. The correct resolution may be:

```text
More Observation
Better Data
Better Measurement
No Change
Policy Change by Human
```

---

# V2-49. ANTI-DEATH LOOP

The anti-death loop protects the system from progressive self-destruction.

## 103.1 Trigger indicators

Examples:

```text
Robustness Decline Across Successive Versions
Complexity Growth Without Durable Benefit
Increasing Research Cost per Useful Finding
Repeated Rollbacks
Repeated Evaluator Near-Misses
Contradiction Explosion
Knowledge Confidence Inflation
High Candidate Failure Rate
High Parent-to-Child Regression Rate
```

## 103.2 Response

```text
TRIGGER
  ↓
FREEZE EVOLUTION
  ↓
PRESERVE EVIDENCE
  ↓
IDENTIFY LAST KNOWN-GOOD
  ↓
OPTIONAL AUTOMATIC ROLLBACK
  ↓
INCIDENT
  ↓
ROOT CAUSE
  ↓
FAILURE MEMORY
  ↓
HUMAN REVIEW IF POLICY REQUIRES
  ↓
RESTART RESEARCH UNDER FRESH BUDGET / WINDOW
```

## 103.3 Principle

> **Self-preservation has priority over self-improvement.**

---

# V2-50. CANDIDATE PARENTING RULES

Not every research candidate should become a parent.

## 104.1 Research parent

A failed or unapproved candidate may be a research parent for an explicitly experimental branch if policy allows it.

## 104.2 Production evolution parent

A candidate may act as a production-oriented parent only when it has:

```text
Required Validation
Human Approval where required
Integrity Verification
Known-Good / Approved State
Rollback Target
Monitoring Profile
Policy Compatibility
```

This prevents an untrusted branch from becoming the unquestioned ancestor of the production line.

---

# V2-51. RESEARCH PRIORITY V2

When many issues exist, the system may rank research targets using explicit measurable criteria.

Possible dimensions:

```text
Affected Prediction Count
Evidence Strength
Reproducibility
Potential Impact
Regime Coverage
Data Quality
Validation Confidence
Recurrence
Research Cost
Risk if Ignored
Novelty
```

This ranking means:

> “This problem deserves investigation earlier under the configured research policy.”

It must not mean:

> “This is the best strategy.”

---

# V2-52. SELF-LEARNING KNOWLEDGE DASHBOARD

The desktop application should include a dedicated Knowledge page.

## 106.1 Views

```text
Knowledge Overview
New Observations
Supported Knowledge
Aging Knowledge
Contradictions
Refuted Knowledge
Top Research Opportunities
Failure Patterns
Human Governance History
```

## 106.2 Knowledge explorer

Selecting a knowledge object should reveal:

```text
Origin Observation
Context
Evidence
Experiments
Contradictions
Status History
Confidence History
Affected Candidates
Human Decisions
Current Scope
Revalidation Due
```

This makes “learning” inspectable rather than magical.

---

# V2-53. FAILURE MEMORY DASHBOARD

The application should expose:

```text
Failure Frequency
Failure Classes
Affected Versions
Recurrence
RCA Status
Confirmed Causes
Rejected Explanations
Prevention Actions
Postmortem Status
```

Clicking a failure should connect it to:

```text
Predictions
Experiments
Knowledge
Hypotheses
Candidates
Incidents
Versions
```

---

# V2-54. RESEARCH CAMPAIGN LIFECYCLE V2

```text
PLANNED
  ↓
READY
  ↓
RUNNING
  ↓
PAUSED
  ↓
CHECKPOINTED
  ↓
COMPLETED
  ↓
EVALUATED
  ↓
CLOSED
```

Alternative terminal states:

```text
CANCELLED
BUDGET_EXHAUSTED
INCONCLUSIVE
CONTAMINATED
FAILED
```

A closed campaign retains its evidence even when no candidate survives.

---

# V2-55. SHADOW / CHALLENGER V2

The Candidate should normally pass through:

```text
RESEARCH
   ↓
VALIDATION
   ↓
HUMAN DECISION
   ↓
SHADOW
   ↓
BEHAVIORAL COMPARISON
   ↓
PROMOTION GATE
```

## 109.1 Shadow data

The shadow ledger should capture:

```text
Control Decision
Candidate Decision
Decision Timestamp
Input Snapshot
Regime
Risk Proposal
Execution Assumptions
Simulated Result
Disagreement
Tail Event
Data Quality
```

## 109.2 Shadow vs canary

Shadow has no execution authority. Canary introduces limited execution exposure and therefore requires additional human policy. Shadow should remain the default research bridge.

---

# V2-56. PROMOTION GATE V2

The promotion gate must verify a complete package:

```text
Human Approval = YES
Candidate Hash = VERIFIED
Parent = VERIFIED
Evidence = COMPLETE
Evaluator = VERIFIED
Metric Registry = VERIFIED
No Forbidden State = TRUE
Required Validation = PASS
Rollback Target = READY
Monitoring = READY
Policy Compatibility = TRUE
Operating Policy = ALLOWED
Shadow Requirement = SATISFIED
```

The promotion action must be a separate control-plane transition.

---

# V2-57. AUTOMATIC ROLLBACK BOUNDARY V2

Automatic rollback is permitted only when: 

```text
Condition is Predefined
Condition is Observable
Known-Good Target Exists
Target is Approved
Rollback Path is Tested / Validated
Rollback Event is Logged
Guardian Allows the Transition
```

Automatic rollback may never be reinterpreted as permission to search for a new strategy during the incident.

During an incident, the priority order is:

```text
PROTECT
→ STABILIZE
→ ROLLBACK
→ RECORD
→ INVESTIGATE
```

---

# V2-58. OPERATING WINDOW + EVOLUTION BOUNDARY

The following relationship is now explicit:

```text
DAILY WINDOW
   ↓
RESEARCH BUDGET
   ↓
EXPERIMENT BUDGET
   ↓
CANDIDATE BUDGET
   ↓
VALIDATION BUDGET
```

A research engine cannot consume tomorrow’s research capacity today.

When the window closes, the research state becomes:

```text
DRAINING / PAUSED / SAFE_SHUTDOWN
```

It does not become:

```text
EXTEND WINDOW
```

---

# V2-59. CONFIGURATION HIERARCHY V2

The system must distinguish between configuration classes.

## 113.1 Publicly adjustable by human

```text
Operating Schedule
Research Budget
Alert Preferences
UI Settings
Approved Research Scope
Approved Strategy Set
```

## 113.2 Governance-protected

```text
Risk Hard Limits
Approval Policy
Evaluator Root
Holdout Policy
Guardian Policy
Promotion Policy
Rollback Policy
Audit Policy
```

## 113.3 Candidate-controlled inside sandbox only

```text
Candidate Parameters
Research Features
Research Rules
Research Strategy Variants
Sandbox Search Parameters
```

No candidate is allowed to promote a sandbox setting into a protected class by reclassifying it itself.

---

# V2-60. TIME / CLOCK INTEGRITY

Because operating windows, point-in-time validation, and incident timelines all depend on time, clock integrity becomes part of the evidence chain.

The system should record:

```text
System Clock Source
UTC Timestamp
Broker Timestamp
Local Display Time
Timezone
Clock Drift
Event Sequence
```

A research result with an unknown causal timestamp should be downgraded in evidentiary status.

---

# V2-61. REPRODUCIBILITY PACKAGE V2

A candidate or experiment should be reproducible from a package containing, as applicable:

```text
Source / Artifact Digest
Configuration Snapshot
Dataset Version
Feature Version
Evaluator Version
Metric Version
Environment Version
Dependency Versions
Random Seed / RNG State
Experiment Definition
Search Space
Trial Count
Execution Parameters
Result Snapshot
```

The package is a research artifact. It is not automatically a production deployment package.

---

# V2-62. RESEARCH-PLANE / CONTROL-PLANE API BOUNDARY

The research plane communicates with the trusted control plane through explicit contracts.

## 116.1 Allowed research requests

```text
REQUEST_OBSERVATION
REQUEST_EXPERIMENT
REQUEST_VALIDATION
REQUEST_HOLDOUT_EVALUATION
REQUEST_CANDIDATE_REGISTRATION
REQUEST_REVIEW_PACKAGE
REQUEST_RESEARCH_BUDGET
```

## 116.2 Control-plane decisions

```text
ALLOW
DENY
PAUSE
REQUIRE_REVIEW
REQUIRE_MORE_EVIDENCE
MARK_CONTAMINATED
REGISTER_KNOWN_GOOD
PROMOTE
ROLLBACK
```

This interface prevents the research plane from directly mutating trusted state.

---

# V2-63. STATE SNAPSHOT CONTRACT

At safe points the system should create a coherent state snapshot containing at least:

```text
Runtime Version
Research State
Active Campaigns
Active Checkpoints
Knowledge Store Version
Failure Memory Version
Experiment Ledger Position
Pending Human Decisions
Operating Window State
Resource Budget State
Guardian State
Audit Position
```

A snapshot is valid only when its integrity is verified.

---

# V2-64. RESEARCH PAUSE SEMANTICS

Pause must be a first-class state rather than a process crash.

## 118.1 Reasons

```text
Window End
Budget Exhaustion
Resource Pressure
Human Request
Data Quality Failure
Evaluator Failure
Guardian Intervention
External Dependency Failure
```

## 118.2 Resume criteria

The system must know why it paused and what must be true before it can resume.

---

# V2-65. HUMAN-GOVERNED OPERATING SCHEDULE

The schedule itself should be visible and auditable in the application.

```text
TODAY
────────────────────────
Scheduled Start
Scheduled End
Actual Start
Actual End
Elapsed
Remaining
Mode
Research Budget Remaining
Experiment Budget Remaining
CPU / RAM status
Active Jobs
Checkpoints
```

The interface should clearly show when the system is: 

```text
ACTIVE
DRAINING
WAITING
PAUSED
OFFLINE
```

This prevents the user from assuming the system is doing research when the window is already closed.

---

# V2-66. CURRENT CANONICAL DAILY EXAMPLE

```text
Learning Cycle LC-001
Date: 2026-09-25
Window: 18:00–23:30
Configured Duration: 5h 30m

Observations: 12,481
Outcome Records Updated: 2,104
New Knowledge Objects: 7
Knowledge Revalidated: 3
Contradictions Detected: 2
Hypotheses Created: 5
Experiments Completed: 14
Candidates Generated: 3
Candidates Rejected: 2
Review-Ready Candidates: 1
Holdout Queries: 1
Human Decisions: 1
Incidents: 0
Rollbacks: 0
Checkpoints: 4
Shutdown: SAFE
```

These numbers are illustrative schema, not claimed project performance.

---

# V2-67. UPDATED SYSTEM CONSTITUTION

The following rules are now intended to be treated as project constitution-level rules.

```text
R01  DATA BEFORE SIGNAL.
R02  CAUSAL TIME BEFORE PERFORMANCE.
R03  STRUCTURE BEFORE ENTRY.
R04  REGIME BEFORE STRATEGY.
R05  STRATEGY BEFORE SIGNAL.
R06  SIGNAL BEFORE RISK.
R07  RISK BEFORE EXECUTION.
R08  EXECUTION BEFORE OUTCOME.
R09  RECONCILIATION BEFORE TRUST.
R10  RESEARCH BEFORE PROMOTION.
R11  SHADOW BEFORE LIVE.
R12  EVIDENCE BEFORE CLAIM.
R13  UNKNOWN IS NOT SAFE.
R14  BROKER REALITY BEATS ABSTRACT ASSUMPTIONS.
R15  RAW DATA MUST REMAIN TRACEABLE.
R16  DECISIONS MUST BE REPRODUCIBLE.
R17  CONFIGURATION MUST BE VERSIONED.
R18  FAILURES ARE RESEARCH DATA.
R19  TELEGRAM IS AN INTERFACE, NOT THE BRAIN.
R20  SCORE IS NOT PROBABILITY.
R21  CONFIDENCE IS NOT GUARANTEE.
R22  HIGH SCORE IS NOT SAFETY.
R23  NO FUTURE INFORMATION.
R24  NO SILENT REPAINTING.
R25  NO BLIND RESEND.
R26  NO UNPROTECTED EXPOSURE.
R27  NO ARCHITECTURAL ASSUMPTION IS ALPHA.
R28  EVERY HYPOTHESIS MUST HAVE AN EXIT CONDITION.
R29  SELF-LEARNING IS NOT SELF-MODIFICATION.
R30  KNOWLEDGE DOES NOT GRANT AUTHORITY.
R31  CANDIDATE DOES NOT GRANT AUTHORITY.
R32  EVALUATOR IS PROTECTED FROM THE CANDIDATE.
R33  FINAL HOLDOUT IS A TRUST BOUNDARY.
R34  TRIAL HISTORY IS EVIDENCE.
R35  FAILED EXPERIMENTS REMAIN VISIBLE.
R36  THE HUMAN OWNS PRODUCTION-IMPACTING CHANGE.
R37  GUARDIAN OUTRANKS EVOLUTION.
R38  SELF-PRESERVATION OUTRANKS SELF-IMPROVEMENT.
R39  THE OPERATING WINDOW IS HUMAN-DEFINED AND BOUNDED TO 3–8 HOURS DAILY.
R40  THE SYSTEM MAY NOT EXTEND ITS OWN WINDOW.
R41  WINDOW END IS A CONTROLLED DRAIN, NOT A HARD KILL.
R42  CHECKPOINTS ARE FIRST-CLASS STATE.
R43  UNKNOWN STATE MUST NOT SILENTLY BECOME PRODUCTION STATE.
R44  ROLLBACK TARGETS MUST BE KNOWN-GOOD.
R45  EVERY ROLLBACK CREATES AN INCIDENT.
R46  EVERY INCIDENT CAN FEED FAILURE MEMORY.
R47  CONTRADICTIONS ARE RESEARCH DATA.
R48  KNOWLEDGE AGES AND MAY REQUIRE REVALIDATION.
R49  NO-CHANGE IS A VALID RESEARCH RESULT.
R50  CURRENT SYSTEM CONTAINS NO AI DEPENDENCY.
```

---

# V2-68. UPDATED END-TO-END OPERATING LOOP

The full Version 2.0 loop is:

```text
MARKET DATA
   ↓
DATA VALIDATION
   ↓
TIMEFRAME STATE
   ↓
FEATURES / STRUCTURE / REGIME
   ↓
STRATEGY ELIGIBILITY
   ↓
SIGNAL / SCORE / RISK
   ↓
SHADOW EXECUTION
   ↓
OUTCOME
   ↓
OBSERVATION / AUDIT
   ↓
FAILURE / PATTERN DETECTION
   ↓
KNOWLEDGE OBJECT
   ↓
CONTRADICTION / CONFIDENCE / DECAY CHECK
   ↓
HYPOTHESIS
   ↓
RESEARCH PLAN
   ↓
BUDGET CHECK
   ↓
SANDBOX EXPERIMENT
   ↓
CANDIDATE
   ↓
VALIDATION FIREWALL
   ↓
CONTROL vs CANDIDATE
   ↓
STRESS / REGIME / REGRESSION
   ↓
EVIDENCE PACKAGE
   ↓
DESKTOP APPLICATION REVIEW
   ↓
HUMAN DECISION
   ├─────────────┐
   ▼             ▼
REJECT       APPROVE
   │             │
   │             ▼
   │           SHADOW
   │             │
   │             ▼
   │       PROMOTION GATE
   │             │
   │             ▼
   │        CONTROLLED RUNTIME
   │             │
   │             ▼
   │         MONITORING
   │             │
   │       ┌─────┴─────┐
   │       ▼           ▼
   │    STABLE      INCIDENT
   │                   │
   │                   ▼
   │                ROLLBACK
   │                   │
   │                   ▼
   │               POSTMORTEM
   │                   │
   └──────► FAILURE MEMORY
                           │
                           ▼
                       LEARNING
```

The Guardian and Operating Window surround the chain and may pause or halt it according to policy.

---

# V2-69. UPDATED BUILD ORDER

The implementation order should preserve the architecture’s trust boundaries.

## Phase 0 — Immutable foundations

```text
Repository
Configuration Schema
Versioning Rules
Audit Format
Identity / Hashing
Basic Persistence
Guardian Skeleton
```

## Phase 1 — Deterministic runtime

```text
MT5 Adapters
C++ Data Bus
Data Validation
Timeframe State
Core Runtime
Shadow Ledger
```

## Phase 2 — Observation and outcomes

```text
Prediction Ledger
Outcome Engine
Failure Detection
System Health
```

## Phase 3 — Self-learning

```text
Knowledge Object Store
Knowledge Lifecycle
Context Learning
Contradiction Engine
Knowledge Decay
Failure Memory
```

## Phase 4 — Research plane

```text
Hypothesis Engine
Research Planner
Experiment Ledger
Research Budget
Experiment Fingerprint
Sandbox
```

## Phase 5 — Evolution

```text
Candidate Generator
Candidate Registry
Evolution Graph
Candidate Comparison
```

## Phase 6 — Validation

```text
Validation Firewall
OOS
Walk-Forward
Stress
Regression
Holdout Service
Evaluator Firewall
```

## Phase 7 — Governance

```text
Approval Gate
Change Investigation Report
Promotion Gate
Known-Good Registry
Rollback
Incident / Postmortem
```

## Phase 8 — Operating Window and recovery

```text
Schedule Manager
3–8h enforcement
DRAINING
Checkpoint / Resume
Crash Recovery
Resource Budgets
```

## Phase 9 — Desktop control center

```text
Dashboard
Research UI
Knowledge UI
Candidate UI
Approval Center
Evolution Graph UI
Incident UI
Schedule UI
Audit UI
```

## Phase 10 — Auxiliary Telegram

```text
Operations Assistant
Governance Notifications
Authenticated Requests
```

## Phase 11 — Controlled real-world validation

Only after the preceding layers are stable should any additional deployment stage be considered.

---

# V2-70. DEFINITION OF DONE — VERSION 2.0

Version 2.0 is conceptually complete when the system can demonstrate all of the following in a reproducible test environment.

## Research

```text
Can observe itself.
Can detect repeated weaknesses.
Can create structured failure objects.
Can build knowledge objects.
Can detect contradictions.
Can form falsifiable hypotheses.
Can run bounded experiments.
Can generate isolated candidates.
```

## Validation

```text
Can compare candidate vs control.
Can run leakage checks.
Can run appropriate time-aware validation.
Can record trial history.
Can protect locked holdout.
Can prevent evaluator mutation.
Can detect regression.
Can conclude no change.
```

## Governance

```text
Can produce a complete Change Investigation Report.
Can route it to the desktop Approval Center.
Can preserve the human decision.
Cannot bypass the approval gate.
```

## Self-preservation

```text
Can detect protected-component integrity failures.
Can freeze research.
Can rollback to known-good state under policy.
Can create an incident.
Can create a postmortem record.
Cannot rewrite its own safety boundary.
```

## Operating window

```text
Can enforce 3–8h daily policy.
Can enter DRAINING.
Can checkpoint active research.
Can resume valid checkpoints.
Can recover from interruption.
Cannot extend its own window.
```

## Interface

```text
Desktop application contains full control context.
Telegram can assist but is non-authoritative.
```

---

# V2-71. RESEARCH QUESTIONS THAT REMAIN OPEN AFTER V2

The following remain explicit research questions rather than solved claims:

```text
1. How should knowledge confidence be calibrated quantitatively for non-stationary market contexts?

2. What is the safest practical way to estimate statistical evidence under a continuously adaptive search process?

3. How should PBO / DSR / CPCV / SPA / Reality Check be combined for different candidate classes?

4. How should research budgets be converted into a formal evidence penalty without becoming arbitrary?

5. How should contradictions be resolved when market regimes change faster than validation refresh cycles?

6. How should knowledge decay be calibrated without forcing frequent unnecessary revalidation?

7. What minimum shadow duration is appropriate for each type of candidate?

8. How should candidate complexity be measured and penalized?

9. How should the system distinguish genuine causal improvement from context-specific coincidence?

10. How should human decisions be represented mathematically without treating them as unquestionable truth labels?

11. What is the safest research-method evolution boundary?

12. Which parts of architecture evolution, if any, should ever become automated?

13. How should long-horizon research be scheduled when the daily 3–8h limit intersects with experiment completion times?

14. Which checkpoint granularity minimizes recovery loss without overwhelming storage?

15. How should broker-profile drift alter historical comparability?
```

An open research question is not a gap to be hidden. It is a named boundary of current knowledge.

---

# V2-72. V2 CHANGE LOG

| Change ID | Update | Status |
|---|---|---|
| V2-001 | Formal self-learning architecture | CORE |
| V2-002 | Knowledge Object model | CORE |
| V2-003 | Knowledge lifecycle / contradiction / decay | CORE |
| V2-004 | Learning-to-Evolution Gate | CORE |
| V2-005 | Hypothesis Engine v2 | CORE |
| V2-006 | Research Planner / bounded campaigns | CORE |
| V2-007 | Evolution Budget extension | CORE |
| V2-008 | Experiment Ledger v2 | CORE |
| V2-009 | Validation / Evidence / Evaluator Firewalls | CORE |
| V2-010 | Control vs Candidate governance | CORE |
| V2-011 | Desktop Application as Primary Control Center | CORE |
| V2-012 | Telegram demoted to auxiliary layer | CORE |
| V2-013 | Guardian / self-preservation architecture | CORE |
| V2-014 | Known-Good Registry | CORE |
| V2-015 | Anti-death loop | CORE |
| V2-016 | 3–8h daily operating window | CORE |
| V2-017 | Separate runtime/research schedules | CORE |
| V2-018 | DRAINING / safe shutdown semantics | CORE |
| V2-019 | Checkpoint / resume / crash recovery | CORE |
| V2-020 | Resource governance tied to operating windows | CORE |
| V2-021 | Learning-cycle audit metrics | CORE |
| V2-022 | No-Change / Insufficient-Evidence engine | CORE |
| V2-023 | Explicit No-AI invariant | CORE |
| V2-024 | Expanded forbidden-behavior constitution | CORE |
| V2-025 | Expanded application data domains | CORE |

---

# V2-73. SOURCE REVIEW AND CONSOLIDATION NOTE

Version 2.0 was consolidated against the project’s current canonical materials. The source set included:

```text
1. Existing unified XAUUSD master reference
2. Canonical XAUUSD multi-timeframe core reference
3. Self-evolution / human-governance research document
4. Planning notes covering desktop control, approval, Telegram assistants, and earlier self-learning decisions
```

The earlier numbered copies of the XAUUSD reference were treated as repeated exports of the same architecture rather than independent competing designs where their content matched the canonical reference.

## 127.1 Source hashes used in the consolidation

```text
core_reference.md
b3b3480a6905bbacb26cdbf0649a1b927a914de31f5ccc8b4903b0455c2a519c

self_evolution_research.md
0436cfb456e21ee24cb2a62102a708a617c82b695a7be588eb3ee7287b5d291c

planning_notes.md
4d6883c5aca816800ec79dc92fb5b11b75365b212264c3bce41d817fc067e8d2
```

The source hashes are included for provenance and do not imply that the architecture itself has been experimentally validated.

---

# V2-74. FINAL V2 MASTER STATEMENT

> **The AURA system is a deterministic, research-first, multi-timeframe shadow system surrounded by a human-governed self-learning and self-evolution research architecture. It does not depend on AI. It learns from observations, outcomes, failures, context, experiments, and governance history; converts supported knowledge into falsifiable hypotheses; creates bounded candidates inside a sandbox; validates them through protected evidence procedures; presents complete change packages to the human through the desktop application; and may evolve only through explicit authority. Telegram is auxiliary. The Guardian protects the architecture from self-corruption. The daily operating window is human-defined and bounded to 3–8 hours, with controlled draining, checkpointing, and recovery. The system may automatically preserve itself and return to a known-good state, but it may never automatically grant itself new authority.**

## 128.1 Final conceptual equation

```text
TRUSTWORTHY AUTONOMY
=
AUTOMATED OBSERVATION
+
STRUCTURED LEARNING
+
FALSIFIABLE RESEARCH
+
BOUNDED EVOLUTION
+
PROTECTED EVALUATION
+
HUMAN AUTHORITY
+
SAFE EXECUTION
+
SELF-PRESERVATION
+
TEMPORAL / RESOURCE BOUNDS
+
AUDITABLE MEMORY
```

## 128.2 Final separation

```text
LEARNING      = acquires knowledge
RESEARCH      = tests knowledge
EVOLUTION     = creates candidates
EVALUATION    = judges evidence
HUMAN         = grants authority
RUNTIME       = executes approved state
GUARDIAN      = protects the system
OPERATING WIN = bounds time and resources
MEMORY        = preserves history
```

## 128.3 Final principle

> **The system should become better at discovering what is true about itself before it is ever allowed to become more powerful.**

---

# APPENDIX C — V2 ENTITY REGISTRY

This appendix provides a compact registry of the new Version 2.0 research and governance entities.

```text
KNOWLEDGE_OBJECT
FAILURE_PATTERN
CONTRADICTION
HYPOTHESIS
RESEARCH_CAMPAIGN
EXPERIMENT
EXPERIMENT_FINGERPRINT
CANDIDATE
VALIDATION_RUN
HOLDOUT_QUERY
EVIDENCE_SNAPSHOT
HUMAN_DECISION
CHANGE_PROPOSAL
PROMOTION_PACKAGE
KNOWN_GOOD
DEPLOYMENT
INCIDENT
POSTMORTEM
CHECKPOINT
LEARNING_CYCLE
OPERATING_SCHEDULE
RESOURCE_BUDGET
GUARDIAN_EVENT
AUDIT_EVENT
```

---

# APPENDIX D — V2 MINIMUM DATABASE DOMAINS

The original core domains are retained. Version 2.0 adds or formalizes: 

```text
knowledge_objects
knowledge_history
knowledge_contradictions
knowledge_revalidation
failures
failure_evidence
rca_cases
hypotheses
research_campaigns
experiments
experiment_trials
experiment_fingerprints
candidates
candidate_artifacts
validation_runs
validation_evidence
holdout_queries
contamination_records
evaluator_versions
metric_registry_versions
human_decisions
change_proposals
promotion_packages
known_good_versions
incidents
postmortems
evolution_edges
evidence_edges
checkpoints
operating_schedules
operating_window_events
resource_budgets
resource_events
learning_cycles
guardian_events
system_snapshots
```

---

# APPENDIX E — V2 OPERATING CHECKLIST

## Before Window

```text
Schedule Valid
Guardian Healthy
Policy Hash Verified
Evaluator Integrity Verified
Ledger Healthy
Known-Good Registry Available
Storage Available
Resource Budget Loaded
Last Snapshot Valid
```

## During Window

```text
Observe
Learn
Research
Budget
Checkpoint
Audit
Human Notifications
Monitor Integrity
```

## Before Drain

```text
Stop New Noncritical Work
Classify Active Tasks
Checkpoint Resumable Jobs
Finalize Critical Commits
Write Audit
Verify Persistence
```

## After Shutdown

```text
State = OFFLINE
Last Checkpoint = VALID
Audit = WRITTEN
No Hidden Processes = VERIFIED
Next Window = SCHEDULED
```

---

# APPENDIX F — V2 FAILURE CATEGORIES

```text
DATA_FAILURE
SCHEMA_FAILURE
CLOCK_FAILURE
CONNECTION_FAILURE
FEATURE_FAILURE
REGIME_FAILURE
ELIGIBILITY_FAILURE
SIGNAL_FAILURE
RISK_FAILURE
EXECUTION_FAILURE
RECONCILIATION_FAILURE
PERSISTENCE_FAILURE
RESEARCH_FAILURE
EVALUATOR_FAILURE
HOLDOUT_FAILURE
GOVERNANCE_FAILURE
AUTHORIZATION_FAILURE
SCHEDULE_FAILURE
CHECKPOINT_FAILURE
RECOVERY_FAILURE
RESOURCE_FAILURE
INTEGRITY_FAILURE
GUARDIAN_FAILURE
TELEGRAM_FAILURE
UNKNOWN_FAILURE
```

---

# APPENDIX G — V2 RESEARCH OUTPUT TAXONOMY

A research cycle may terminate with any of:

```text
SUPPORTED
REFUTED
INCONCLUSIVE
CONTRADICTED
NO_CHANGE
DATA_INSUFFICIENT
VALIDATION_FAILED
ROBUSTNESS_FAILED
REGRESSION_FAILED
BUDGET_EXHAUSTED
CONTAMINATED
HUMAN_REJECTED
HUMAN_REQUESTED_MORE_RESEARCH
APPROVED_FOR_SHADOW
APPROVED_FOR_PROMOTION_REVIEW
```

These states should remain distinct.

---

# APPENDIX H — V2 DESIGN DOCTRINE

```text
1. The system should be able to learn without changing itself.
2. The system should be able to change itself in research without changing production.
3. The evaluator should remain more trusted than the candidate.
4. The human should remain more authoritative than the evolution engine.
5. The Guardian should remain more conservative than the evolution engine.
6. The operating window should remain more restrictive than research ambition.
7. History should remain richer than the current version.
8. Failure should remain visible.
9. Contradiction should remain visible.
10. Uncertainty should remain visible.
11. A no-change result should remain legitimate.
12. The system should never need to lie to itself to continue evolving.
```





# V3-61. DOCUMENT INTEGRITY NOTE

The body below V3-60 is the preserved V2 master content included for continuity and completeness.

When a statement in the preserved body conflicts with a current V3 control rule, the V3 rule wins according to the canonical precedence rule in V3-02.

This document therefore acts as:

```text
CURRENT CONTROL OVERLAY
+
COMPLETE PRESERVED V2 TECHNICAL BODY
```

No earlier research material is considered deleted merely because the project has evolved.
