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
  calls outside core or explicit libc allowlists fail the gate. `strcpy`,
  `memcpy`, and `memset` require a syntactically direct local value destination
  (including owned by-value parameters), without pointer aliases.
* Casts cannot create mutable pointers to externally owned data. A cast to a
  non-const pointer is conservatively rejected, even for a local object. The
  compiler-evaluated integer literal zero used by `NULL` is the sole exception.
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
  Cover AUTHORIZE, CAPTURE, REFUND, CANCEL, TICK, CLOSE, PAY and ASSESS_LATE_FEE,
  cache replay, key conflicts, final reconciliation, and a refund to cash.

These gates neither analyze memory ownership globally nor claim mathematical
purity for arbitrary C. Mandatory frozen domain, differential, protocol, and
adversarial suites complement their bounded structural and dynamic evidence.

## Validation scope

Changed API boundaries invalidate prior architecture and immutability evidence.
Run structural gate and its deliberate negative controls, then direct runtime
checks with compiler warnings and ASan/UBSan where available. Reuse frozen oracle
expectations; no extraction tool, Joern, new package install, or original-tree
write belongs to this gate.

## Commands and evidence

Use the existing original environment only as a Python/libclang tool provider:

```sh
/home/gustavo/workspace/aurum-fixture/.venv/bin/python tests/architecture_gate.py --self-test
/home/gustavo/workspace/aurum-fixture/.venv/bin/python tests/purity_gate.py --report-dir evidence/purity-runtime-normal
/home/gustavo/workspace/aurum-fixture/.venv/bin/python tests/purity_gate.py --sanitizers --report-dir evidence/purity-runtime-sanitized
```

The structural runner writes `evidence/purity-architecture.json` and the function
inventory `evidence/architecture-inventory.json`. Runtime reports preserve source
hashes, commands, compiler output, process status, and before/after fingerprints.
`purity_semantic.h` is generated only to walk fields without C struct padding;
it does not generate expected results or contain business decision logic.

The direct core sanitizer runner disables LeakSanitizer because the sandbox host
rejects its ptrace-based exit inspection. ASan and UBSan remain enabled and halt
on their first finding. This is a runner-specific limitation, not the status of
any broader externally executed leak gate. The core has no permitted heap calls.
