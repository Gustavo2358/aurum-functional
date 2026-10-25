# Aurum CLI

Build the C17 `aurum` binary using the repository Makefile, then run:

```sh
./build/aurum --seed seed.txt --commands commands.txt
./build/aurum --seed seed.txt --commands -
```

`--commands -` reads stdin. Both flags are required, may appear in either order, and may appear only once. Seed must be a file. Exit 0 means the complete stream was processed, including local rejected commands. Invocation, seed, or blocking I/O failure produces a diagnostic on stderr and exit 2. Stdout contains only JSON result lines. A closed output pipe is reported as an I/O failure with exit 2 on platforms exposing SIGPIPE.

## Grammar

ASCII `OP key=value key=value`, separated by space or tab. LF and CRLF are accepted. A physical line may contain at most 8192 bytes including its terminator; an overlong or invalid-byte line is drained fully and rejected once. Empty lines and full-line comments beginning with `#` after spaces/tabs are ignored. Inline comments, quoting, escapes, unknown fields, repeated fields, and empty values are invalid.

Identifiers are case-sensitive, with 1–48 characters in `A-Z`, `a-z`, `0-9`, `_`, `-`. Decimal integers accept leading zeros. Signs, decimal points, and exponent notation are invalid. Money is exact integer cents in 0–1000000000000; command amounts must be positive. Boolean values are exactly `true` and `false`. Country has two uppercase ASCII letters; currency has three. A syntactically valid unsupported currency reaches the quotation decision.

## Seed

The seed creates empty financial state. No entity is created implicitly.

```text
CLOCK now=144600
ACCOUNT id=A1
CARD id=C1 account_id=A1
MERCHANT id=M1
```

Order: one required `CLOCK`, zero or more `CONFIG`, accounts, cards, merchants. Referenced accounts must already exist. IDs are unique within each entity type. Any error rejects the entire load before commands start. `CLOCK now` is 0–525600000. The account's first cycle is `now / 1440 / 30`.

Entity fields and defaults:

- `ACCOUNT id=ID active=true tier=REGULAR credit_limit=1000000 daily_limit=300000 per_operation_limit=200000 base_score=10`. Only `id` is required. `tier` is REGULAR/PREMIUM; `base_score` is 0–100.
- `CARD id=ID account_id=ID status=ACTIVE expiry_day=500 allow_international=true allow_contactless=true`. Both IDs are required. Status is ACTIVE/BLOCKED; expiry is a nonnegative integer day.
- `MERCHANT id=ID status=ACTIVE category=NORMAL country=BR`. Only `id` is required. Status is ACTIVE/BLOCKED; category is NORMAL/RESTRICTED.

`CONFIG` accepts one or more fields below. A configuration name may occur only once across the entire seed. Defaults and limits are specified in the domain and input schema. Cross-field validation happens after the configuration section, so both score thresholds can be changed together across separate CONFIG lines.

```text
fx_brl fx_usd fx_eur international_bps installment_bps_per_extra
 tariff_cap pin_threshold minimum_installment minimum_due_floor late_fee
 hold_ttl_minutes daily_count_limit review_score decline_score cycle_days
 due_grace_days minimum_due_bps reward_month_cap reward_unit
 accounts_capacity cards_capacity merchants_capacity auths_capacity
 captures_capacity lots_capacity invoices_capacity history_capacity
 ledger_capacity keys_capacity rewards_capacity payments_capacity refunds_capacity
```

Each storage capacity is 1–1000000 and defaults to 100000; arrays grow on demand. Limits cover every stored record, including cached responses. Runtime capacity exhaustion returns ERROR/CAPACITY atomically. Configuration cannot be changed in the command stream.

## Commands

| Operation | Required keys | Optional keys and defaults |
|---|---|---|
| QUOTE | amount | currency=BRL tier=REGULAR installments=1 country=BR |
| AUTHORIZE | request_id account_id card_id merchant_id amount | currency=BRL installments=1 country=BR channel=POS card_present=true pin_ok=true |
| CAPTURE | request_id auth_id principal | — |
| CANCEL | request_id auth_id | — |
| REFUND | request_id capture_id principal | — |
| CLOSE | request_id account_id cycle | — |
| PAY | request_id account_id amount | — |
| ASSESS_LATE_FEE | request_id invoice_id | — |
| TICK | now | — |
| GET_ACCOUNT | account_id | — |
| GET_AUTH | auth_id | — |
| GET_INVOICE | invoice_id | — |
| RECONCILE | — | account_id; absent means whole engine |

Channel is POS/ECOM/CONTACTLESS. `installments` and `cycle` are nonnegative decimal integers representable by int64. Invalid installment counts reach the installment business gate. `TICK now` has the same bounds as `CLOCK`. Financial command IDs are the request IDs in typed namespaces. Invoice IDs are I1, I2, and so on in global creation order.

## Canonical JSON

Every result starts with `op`, optional parsed `request_id`, `decision`, `reason`, in this order. Unknown or unreadable operations use `op="INVALID"`. Errors contain this prefix only, except reconciliation failures also expose the differences below. There are no spaces between JSON tokens; every object ends with LF. Optional IDs are omitted when absent; numeric fields remain integer JSON numbers.

Successful or declined command fields follow the prefix in the order listed:

| Operation | Fields |
|---|---|
| QUOTE | principal, fee |
| AUTHORIZE | principal, fee, reserved, optional auth_id and expires_at |
| CAPTURE | optional auth_id, optional capture_id, principal, fee, points |
| CANCEL / TICK | released |
| REFUND | optional capture_id, principal, fee, cash_refund, points_reversed |
| CLOSE | optional invoice_id, issued_total, minimum_due |
| PAY | paid |
| ASSESS_LATE_FEE | fee |
| GET_ACCOUNT | id, held, debt, available_credit, invoiced_outstanding, future_outstanding, points, cash_refund_total, daily_gross_principal, daily_count, next_cycle_to_close, auth_ids |
| GET_AUTH | id, state, principal, fee, captured_principal, captured_fee, remaining_hold, approved_at, expires_at, installments |
| GET_INVOICE | id, account_id, cycle, issued_principal, issued_fee, issued_total, minimum_due, due_day, outstanding, late_fee_assessed |
| RECONCILE | status, debt_difference, held_difference |

`auth_ids` contains all authorization IDs for the queried account in ASCII order. Reconciliation `status` is OK or INVARIANT_VIOLATION; it never repairs state. A missing account is NOT_FOUND. A cached mutation response uses only the original result fields, so subsequent state changes cannot change replay bytes. Queries project the current engine.
