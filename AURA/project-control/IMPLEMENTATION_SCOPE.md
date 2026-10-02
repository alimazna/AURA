# AURA — Current Implementation Scope

## Principle

The Master V3 document is the architectural reference, but not every architectural capability must be implemented immediately.

This file defines the currently authorized implementation scope.

## Current objective

Build a coherent, testable AURA foundation that progresses incrementally toward the Master V3 architecture.

## Current MVP scope

The Master V3 MVP guidance starts with:

- XAUUSD only
- M15 primary operational setup
- H4 primary structural context
- M5/M1 execution context
- basic higher-timeframe context
- data validation
- timeframe state
- regime
- one strategy family
- risk proposal
- shadow execution
- shadow ledger
- health monitoring
- replay

## Active build order

### NOW

1. Immutable foundations
2. Resilience and graceful degradation foundations
3. Deterministic runtime foundations
4. MT5 market-data boundary
5. Shadow execution lifecycle
6. Required persistence/audit
7. Tests that prove implemented behavior

### DEFERRED

- broader self-learning expansion
- research plane expansion
- candidate generation/evolution
- validation firewall expansion
- governance/approval expansion
- operating-window/recovery expansion
- full desktop control center
- Telegram auxiliary layer
- controlled real-world validation

These remain part of the Master architecture, but are not automatically active now.

## Important interpretation

The project is being built from the Master V3 architecture.

The existing repository may be reused where compatible, but it is not the authority.

Existing code may be retained, replaced, or rewritten when needed to satisfy the current scope.

## Current non-goals

- unattended live trading
- profitability claims
- calibrated probability claims
- broker-independent execution claims
- automatic production self-modification
- automatic promotion of unknown future states

## Scope changes

A deferred capability enters active scope only through an explicit human decision recorded in `DECISIONS.md`, followed by task creation/update in `TASK_MANIFEST.yaml`.
