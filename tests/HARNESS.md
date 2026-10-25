# Frozen acceptance and differential harness

The original source, normative contracts, oracle and official harness are frozen
under `original/`. `original-manifest.json` records 107 SHA-256 hashes from clean
revision `a9f3b7e8ec7f09e2ccf9f514be635950226ad513` of
`/home/gustavo/workspace/aurum-fixture`. That repository was only read. `frozen-manifest.json` also hashes all six preimplementation corpus/manifest files.
No original
build artifacts, `.git`, Joern outputs or real data were copied.

`vendor/` contains the existing `gherkin-official` 42.0.1 Python distribution and
its MIT license, copied from the original virtual environment without installing
packages. `acceptance.py` supplies its import path. Production never imports it.

## Run against the replacement

From the repository root after `make all`:

```sh
python3 tests/verify_frozen.py
python3 tests/abi_layout.py
python3 tests/acceptance.py --binary build/aurum --library build/libaurum.so --report-dir evidence/harness-candidate
python3 tests/differential.py --candidate build/aurum
python3 tests/differential.py --candidate build/aurum --corpus tests/corpus/generated.json --golden tests/corpus/golden-generated.json --report evidence/harness-generated.json
python3 tests/state_differential.py --library build/libaurum.so
```

The shared library includes `tests/legacy_adapter.c`, a test-only ABI bridge into
the replacement core. It is not linked into the production executable. Frozen
ctypes bindings read the frozen header; Engine and value-record field layouts must
therefore match it. `abi_layout.py` independently compiles both headers and compares
all record sizes and offsets/sizes of 213 fields before loading the library.
The bridge converts pure value results to historical output
parameters and prepares explicit historical fixture records. Financial decisions,
projections and formulas call the replacement `core_*` functions. No old C source
is linked into this library.

Acceptance executes the 296 normative Gherkin cases plus three runner controls,
73 protocol checks, 17 adversarial groups, 16,887 numeric cases, nine invariant
streams (540 operations, 212,281 assertions), and API regression groups. The
Gherkin runner rejects missing steps, missing operations, empty expectations and
invalid fixture states; there are no skips.

The base differential corpus contains 136 complete invocations from the frozen
independent adversarial/protocol tests and all 12 normative flows, with additional
invocation errors and command-file input. It covers all 13 public operations.
Each case records seed/input bytes, argument vector and environment overrides.
The golden captures stdout bytes, stderr bytes, exit code and changed/deleted
files relative to a dedicated process working directory. It detects stream,
serialization, status and filesystem differences; `/dev/full` and closed-pipe
checks run separately in the frozen protocol gate.

The generated corpus contains 24 reproducible sequences, RNG seed 20261025, with
7,557 operations. It spans two accounts, currencies, tiers, installment boundaries,
partial captures, cancellation/expiration, refunds, payments, replay/conflicts,
four billing cycles, late fees, and final queries for every authorization and
possible invoice. Its stdout/stderr/status/file golden is separate from the base
corpus. `golden-state.json` additionally hashes all canonical value fields in every
Engine collection, configuration, logical clock and Result after each operation.
Allocator capacities, addresses and struct padding are deliberately excluded.

Golden compatibility results come from the original binary; independent normative
expectations remain in the original Gherkin and arithmetic/property tests. Neither
form of evidence proves equivalence for all possible inputs.

## Baseline and negative controls

Before production implementation, the original was compiled in isolation and all
acceptance gates passed. Reports and stdout/stderr are under
`evidence/harness-original/`. Rebuild that baseline explicitly with:

```sh
python3 tests/acceptance.py --build-original --mutations --report-dir evidence/harness-original-recheck
```

The preimplementation input/golden generators refuse to replace existing files.
Do not regenerate goldens to make a candidate pass.

```sh
python3 tests/differential_mutations.py
python3 tests/negative_controls.py --original evidence/harness-original/aurum
```

The first command compiles 11 real semantic/protocol mutations of isolated copies
of the frozen original and requires each to fail the frozen differential corpus.
A consistent local-variable rename must pass as an equivalence control. Evidence
is in `evidence/harness-differential-mutations.json`. The second executes wrappers
that independently change stdout, stderr, process status, file creation and file
deletion, requiring all five differences to be detected. It writes
`evidence/harness-negative-controls.json`. These controls validate harness
sensitivity; mutations of the original are not claimed as mutation testing of the
replacement implementation.

No Joern, slicing or extraction gate runs in these scripts.
