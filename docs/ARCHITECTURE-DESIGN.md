# Pure domain and transactional ownership

`subject/include/core.h` owns domain enums and value records (Config, Account, Card,
Merchant, Command, Result, Authorization, Capture, Lot, Invoice, History, Entry,
Cached, Payment, Refund, Reward, Price, Projection). They retain the original
field meanings and representable ranges. `aurum.h` includes `core.h` and owns only
the mutable storage container and shell API. No original mutable engine enters
the core.

The production core receives immutable views, returns values, performs no I/O,
allocation, hidden clock reads, global mutation or writes through input pointers.
Locals are ordinary C values. Value-returning constructors replace out parameters.

```c
typedef struct { bool ok; int64_t value; } Number;
typedef struct { Money gross; int64_t count; } DailyTotals;
typedef struct { bool valid; Money debt_difference, held_difference; } Reconciliation;
typedef struct {
    Config config;
    int64_t now;
    const Account *accounts; size_t accounts_len;
    /* Same const-pointer/length pair for cards, merchants, auths, captures,
       lots, invoices, history, ledger, keys, payments, refunds, rewards. */
} StateView;

typedef enum { TABLE_ACCOUNT, TABLE_CARD, TABLE_MERCHANT, TABLE_AUTH,
    TABLE_CAPTURE, TABLE_LOT, TABLE_INVOICE, TABLE_HISTORY, TABLE_LEDGER,
    TABLE_KEY, TABLE_PAYMENT, TABLE_REFUND, TABLE_REWARD, TABLE_CLOCK } Table;
typedef union { Account account; Card card; Merchant merchant;
    Authorization auth; Capture capture; Lot lot; Invoice invoice;
    History history; Entry entry; Cached key; Payment payment;
    Refund refund; Reward reward; int64_t now; } EditValue;
typedef struct { Table table; size_t index; bool append; EditValue value; } Edit;
typedef enum { CORE_EDIT, CORE_COMMIT, CORE_ABORT, CORE_READ } CoreStatus;
/* Continuation contains only value fields: phase/cursors, result, intermediate
   domain records and allocation amounts. Its fields are core-owned protocol. */
typedef struct { CoreStatus status; Edit edit; Continuation next; Result result; } Step;
Continuation core_begin(void);
Step core_step(StateView state, Command command, Continuation previous);
Result core_capacity_failure(Command command);
```

The first step performs envelope validation, read-only operations and cache lookup.
`CORE_READ` finishes without storage allocation or modification, including cache
replay. `CORE_ABORT` returns an error and requires discarding every private edit.
`CORE_EDIT` requests one append, replacement or clock assignment and supplies the
next continuation. `CORE_COMMIT` publishes the privately staged snapshot. The shell
never chooses financial order, values, account classification, cacheability,
expiration eligibility or rollback policy. It only executes typed edits and
honors the terminal status. Allocation failure discards the private transaction
and returns `core_capacity_failure(command)`.

Every next call receives a view of the current private staged snapshot. The
previous published snapshot remains alive and intact until commit. There is no
fixed bound on the number of calls or edits; capacities retain 1..1,000,000 per
collection. A constant-size continuation is transport, not a collection limit.
Selection scans implement deterministic ordering without core scratch allocation;
ordered multi-entity operations can therefore take quadratic time. This preserves
the domain without claiming scalability beyond the documented algorithm.

The core checks each requested edit against configured capacity before issuing it.
Correlated edits are staged and all validations finish before publication. Cache
append and cached projection refresh are core phases of the same transaction.
An error at any stage leaves all published collections and logical time unchanged.

## Pure helper interface

All public domain helper symbols use `core_`. Parameter lists below preserve the
original const input parameters unless explicitly changed; no function takes an
output pointer.

* `Config core_config_default(void)`; `bool core_config_valid(const Config *)`.
* `Account core_seed_account_default(StateView)`;
  `Card core_seed_card_default(void)`; `Merchant core_seed_merchant_default(void)`;
  `bool core_seed_account_valid(StateView,Account)`,
  `core_seed_card_valid(StateView,Card)`, `core_seed_merchant_valid(StateView,Merchant)`.
* `Command core_command_default(Operation)`; `Result core_result_default(const Command *)`.
* `bool core_identifier_valid(const char *)`, `core_command_valid(const Command *)`,
  `core_command_equal(const Command *, const Command *)`,
  `core_command_same_key(const Command *, const Command *)`, `core_cacheable(const Result *)`.
* `bool core_money_valid(Money)`; `Number core_checked_mul(int64_t,int64_t)`,
  `core_money_add(Money,Money)`, `core_money_sub(Money,Money)`,
  `core_floor_rate(Money,int64_t)`, `core_ceil_rate(Money,int64_t)`,
  `core_money_fx(Money,int64_t)`, `core_proportional(Money,Money,Money)`,
  `core_cumulative_delta(Money,Money,Money,Money)`; `Money core_split_part(Money,int64_t,int64_t)`.
* `Price core_quote(const Config *,const Command *)`,
  `core_price_components(const Config *,Money,int64_t,const char *,Tier)`.
* `bool core_installment_allowed(const Config *,Money,int64_t,const char *)`,
  `core_capture_installment_allowed(const Config *,Money,int64_t)`.
* `Money core_available_credit(Money,Money,Money)`; `Reason core_credit_admission(Money,Money,Money)`,
  `core_operation_admission(Money,Money)`, `core_daily_admission(Money,Money,Money)`,
  `core_count_admission(int64_t,int64_t)`.
* `int64_t core_risk_score(int64_t,const char *,bool,Money,int64_t)`;
  `Decision core_risk_classify(const Config *,int64_t)`.
* `int64_t core_recent_count(StateView,const char *)`;
  `DailyTotals core_daily_totals(StateView,const char *,int64_t)`;
  `Reason core_eligibility(StateView,const Account *,const Card *,const Merchant *,const Command *)`.
* `Money core_authorization_hold(const Authorization *)`;
  `Reason core_capture_admission(const Authorization *,int64_t,Money)`,
  `core_refund_admission(const Capture *,Money)`; `AuthState core_capture_state(Money,Money)`.
* `int64_t core_reward_raw(const Config *,Money,Tier,Category)`,
  `core_reward_grant(const Config *,int64_t,int64_t)`,
  `core_reward_gross(StateView,const char *,int64_t)`.
* `int64_t core_invoice_due_day(const Config *,int64_t)`;
  `Money core_invoice_minimum(const Config *,Money)`;
  `Reason core_close_admission(const Config *,int64_t,int64_t,int64_t)`,
  `core_payment_admission(Money,Money)`.
* `const Account *core_find_account(StateView,const char *)` and corresponding
  `core_find_card`, `core_find_merchant`, `core_find_auth`, `core_find_capture`,
  `core_find_invoice`, each with its const record return type.
* `Money core_lot_principal(const Lot *)`, `core_lot_fee(const Lot *)`,
  `core_invoice_outstanding(StateView,const Invoice *)`;
  `Projection core_project_account(StateView,const Account *)`;
  `bool core_ledger_balanced(StateView)`;
  `Reconciliation core_reconcile(StateView,const char *)`.

Parsing and JSON serialization remain shell concerns. Logical defaults, command
canonical equality, pricing, admission, transitions, financial ordering,
projection and reconciliation remain domain concerns. Production contains no
oracle/scenario IDs or expectation data.
