# Preimplementation purity and architecture contract

This contract is written before production implementation. `docs/ARCHITECTURE-DESIGN.md`
is the API authority; frozen original acceptance remains the behavioral oracle.
No expectation is derived from a newly implemented core.

## Structural gate (libclang, production C)

* Every core function, including private helpers, accepts values or recursively
  read-only pointer graphs. No mutable output parameter is accepted.
* `StateView` contains const collection pointers and lengths. `Continuation`,
  `Edit`, `Step`, and `Result` cannot carry pointers escaping the call.
* Core mutable global/static storage, implicit/indirect calls, assembly, and
  calls outside core or an explicit pure libc allowlist fail the gate.
* Casts cannot create mutable pointers to externally owned data. A cast to a
  non-const pointer is conservatively rejected, even for a local object.
* Public domain symbols are `core_` functions; shell cannot define them.
* Core must not import shell ownership headers or oracle/fixture identifiers.
* Every production function and branch is inventoried with location, call edges,
  and DOMAIN/SUPPORT (core) or TECHNICAL/SUPPORT (shell) classification. Contract
  associations are review aids, not automatically established semantic proof.
* Shell policy placement receives source review alongside the inventory. AST
  gates prove restrictions, not that arbitrary C expresses no business rule.

## Direct runtime gate (core only)

* Invoke representative constructors, arithmetic, pricing, projection, lookup,
  and transition helpers directly, without shell wrappers.
* Independently fixed arithmetic boundaries reject overflow and produce expected
  integer rounding. Invalid and read-only commands return expected terminal status.
* Every collection input is placed in read-only memory. Hash raw input storage
  and record selected semantic fields before and after calls; a write faults and
  fails, even if a function tries to restore its input later.
* Repeated calls with identical values, interleaved with different states and
  commands, return equal semantic values. Ignore C struct padding when comparing
  returned aggregates. Continuations and edits remain caller-owned values.
* Exercise a successful command through CORE_EDIT and its terminal status using
  an independent mechanical edit applicator; published input remains unchanged.

These gates neither analyze memory ownership globally nor claim mathematical
purity for arbitrary C. Mandatory frozen domain, differential, protocol, and
adversarial suites complement their bounded structural and dynamic evidence.

## Validation scope

Changed API boundaries invalidate prior architecture and immutability evidence.
Run structural gate and its deliberate negative controls, then direct runtime
checks with compiler warnings and ASan/UBSan where available. Reuse frozen oracle
expectations; no extraction tool, Joern, new package install, or original-tree
write belongs to this gate.
