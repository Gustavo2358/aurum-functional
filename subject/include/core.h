#ifndef CORE_H
#define CORE_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define MONEY_MAX INT64_C(1000000000000)
#define TIME_MAX INT64_C(525600000)
#define ID_SIZE 49
#define LINE_MAX_BYTES 8192

typedef int64_t Money;
typedef enum { REGULAR, PREMIUM } Tier;
typedef enum { NORMAL, RESTRICTED } Category;
typedef enum { POS, ECOM, CONTACTLESS } Channel;
typedef enum { ACTIVE, PARTIAL, CAPTURED, CANCELLED, EXPIRED } AuthState;
typedef enum { APPROVED, REVIEW, DECLINED, ERROR, OK, NOOP } Decision;
typedef enum {
 NONE, INVALID_INPUT, NUMERIC_RANGE, NOT_FOUND, OWNERSHIP, ACCOUNT_INACTIVE,
 CARD_BLOCKED, CARD_EXPIRED, MERCHANT_BLOCKED, RESTRICTED_CATEGORY,
 INTERNATIONAL_DISABLED, CONTACTLESS_DISABLED, PIN_REQUIRED, PAST_DUE,
 UNSUPPORTED_CURRENCY, INSTALLMENTS, OPERATION_LIMIT, CREDIT_LIMIT, DAILY_AMOUNT,
 DAILY_COUNT, RISK, CAPACITY, IDEMPOTENCY_CONFLICT, AUTH_STATE, CAPTURE_AMOUNT,
 REFUND_AMOUNT, CYCLE_ORDER, CYCLE_OPEN, OVERPAYMENT, INVALID_TIME, RECONCILIATION
} Reason;
typedef enum {
 QUOTE, AUTHORIZE, CAPTURE, CANCEL, REFUND, CLOSE, PAY, ASSESS_LATE_FEE,
 TICK, GET_ACCOUNT, GET_AUTH, GET_INVOICE, RECONCILE, INVALID_OP
} Operation;
typedef enum {
 HOLD_ASSET, HOLD_OFFSET, RECEIVABLE_PRINCIPAL, MERCHANT_CLEARING,
 RECEIVABLE_FEE, FEE_REVENUE, RECEIVABLE_LATE, LATE_REVENUE, CASH, OPENING_OFFSET
} LedgerAccount;
typedef struct {
 int64_t fx_brl, fx_usd, fx_eur, international_bps, installment_bps_per_extra;
 Money tariff_cap, pin_threshold, minimum_installment, minimum_due_floor, late_fee;
 int64_t hold_ttl_minutes, daily_count_limit, review_score, decline_score;
 int64_t cycle_days, due_grace_days, minimum_due_bps, reward_month_cap, reward_unit;
 size_t accounts_capacity, cards_capacity, merchants_capacity, auths_capacity;
 size_t captures_capacity, lots_capacity, invoices_capacity, history_capacity;
 size_t ledger_capacity, keys_capacity, rewards_capacity, payments_capacity, refunds_capacity;
} Config;
typedef struct {
 char id[ID_SIZE]; bool active; Tier tier; Money credit_limit, daily_limit, per_operation_limit;
 int64_t base_score, next_cycle_to_close; Money cached_debt, cached_held, cash_refund_total;
} Account;
typedef struct {
 char id[ID_SIZE], account_id[ID_SIZE]; bool active; int64_t expiry_day;
 bool allow_international, allow_contactless;
} Card;
typedef struct { char id[ID_SIZE]; bool active; Category category; char country[3]; } Merchant;
typedef struct {
 Operation op; char request_id[ID_SIZE], account_id[ID_SIZE], card_id[ID_SIZE], merchant_id[ID_SIZE];
 char auth_id[ID_SIZE], capture_id[ID_SIZE], invoice_id[ID_SIZE];
 Money amount, principal; char currency[4], country[3]; Tier tier; Channel channel;
 bool card_present, pin_ok; int64_t installments, cycle, now;
} Command;
typedef struct {
 Operation op; char request_id[ID_SIZE]; Decision decision; Reason reason;
 char auth_id[ID_SIZE], capture_id[ID_SIZE], invoice_id[ID_SIZE];
 Money principal, fee, reserved, released, cash_refund, issued_total, minimum_due;
 int64_t points, points_reversed, expires_at; bool has_quote;
} Result;
typedef struct {
 char id[ID_SIZE], account_id[ID_SIZE]; AuthState state;
 Money principal, fee, captured_principal, captured_fee;
 int64_t approved_at, expires_at, installments; Tier tier; Category category;
} Authorization;
typedef struct {
 char id[ID_SIZE], auth_id[ID_SIZE], account_id[ID_SIZE]; Money principal, fee, refunded_principal, refunded_fee;
 int64_t cycle, granted, reversed;
} Capture;
typedef struct {
 char account_id[ID_SIZE], capture_id[ID_SIZE], invoice_id[ID_SIZE]; int64_t index, cycle;
 Money principal, fee, paid_principal, paid_fee, cancelled_principal, cancelled_fee;
} Lot;
typedef struct {
 char id[ID_SIZE], account_id[ID_SIZE]; int64_t cycle, due_day;
 Money issued_principal, issued_fee, issued_total, minimum_due, late, paid_late; bool late_fee_assessed;
} Invoice;
typedef struct { char account_id[ID_SIZE]; int64_t minute; Decision decision; Money principal; } History;
typedef struct { char event_id[128], account_id[ID_SIZE]; LedgerAccount account; Money amount; } Entry;
typedef struct { Command command; Result result; } Cached;
typedef struct { char id[ID_SIZE], account_id[ID_SIZE]; Money amount; } Payment;
typedef struct { char id[ID_SIZE], capture_id[ID_SIZE]; Money principal, fee, cash; int64_t points; } Refund;
typedef struct { char account_id[ID_SIZE], capture_id[ID_SIZE]; int64_t cycle, granted; } Reward;
typedef struct { Money principal, international_fee, installment_fee, fee; Reason reason; } Price;
typedef struct { Money debt, held, available_credit, invoiced_outstanding, future_outstanding, cash_refund_total, daily_gross_principal; int64_t points, daily_count, next_cycle_to_close; } Projection;

typedef struct {
 Config config; int64_t now;
 const Account *accounts; size_t accounts_len;
 const Card *cards; size_t cards_len;
 const Merchant *merchants; size_t merchants_len;
 const Authorization *auths; size_t auths_len;
 const Capture *captures; size_t captures_len;
 const Lot *lots; size_t lots_len;
 const Invoice *invoices; size_t invoices_len;
 const History *history; size_t history_len;
 const Entry *ledger; size_t ledger_len;
 const Cached *keys; size_t keys_len;
 const Payment *payments; size_t payments_len;
 const Refund *refunds; size_t refunds_len;
 const Reward *rewards; size_t rewards_len;
} StateView;

typedef struct { bool ok; int64_t value; } Number;
typedef struct { Money gross; int64_t count; } DailyTotals;
typedef struct { bool valid; Money debt_difference, held_difference; } Reconciliation;
typedef struct { bool valid, store; History value; } HistoryRecord;
typedef enum {
 TABLE_ACCOUNT, TABLE_CARD, TABLE_MERCHANT, TABLE_AUTH, TABLE_CAPTURE,
 TABLE_LOT, TABLE_INVOICE, TABLE_HISTORY, TABLE_LEDGER, TABLE_KEY,
 TABLE_PAYMENT, TABLE_REFUND, TABLE_REWARD, TABLE_CLOCK
} Table;
typedef union {
 Account account; Card card; Merchant merchant; Authorization auth;
 Capture capture; Lot lot; Invoice invoice; History history; Entry entry;
 Cached key; Payment payment; Refund refund; Reward reward; int64_t now;
} EditValue;
typedef struct { Table table; size_t index; bool append; EditValue value; } Edit;
/* Continuations own only values. The entry buffer holds at most four pairs for
   one elementary event; arbitrary collections are traversed over successive steps. */
typedef struct {
 unsigned phase, after_ledger, component;
 size_t cursor, target, account_index, entry_cursor, entries_len;
 bool ordered_started;
 Result result, capacity_result;
 Money remaining, fee_remaining;
 Authorization auth; Capture capture; Invoice invoice; Lot lot; Account account;
 Entry entries[8];
 char event_id[128];
} Continuation;
typedef enum { CORE_EDIT, CORE_COMMIT, CORE_ABORT, CORE_READ } CoreStatus;
typedef struct { CoreStatus status; Edit edit; Continuation next; Result result; } Step;
Continuation core_begin(void);
Step core_step(StateView state, Command command, Continuation previous);
Result core_capacity_failure(Command command);

Config core_config_default(void);
bool core_config_valid(const Config *c);
Account core_seed_account_default(StateView state);
Card core_seed_card_default(void);
Merchant core_seed_merchant_default(void);
bool core_seed_account_valid(StateView state, Account account);
bool core_seed_card_valid(StateView state, Card card);
bool core_seed_merchant_valid(StateView state, Merchant merchant);
Command core_command_default(Operation op);
Result core_result_default(const Command *c);
bool core_identifier_valid(const char *s);
bool core_command_valid(const Command *c);
bool core_command_equal(const Command *a, const Command *b);
bool core_command_same_key(const Command *a, const Command *b);
bool core_cacheable(const Result *r);
bool core_money_valid(Money value);
Number core_checked_mul(int64_t a, int64_t b);
Number core_money_add(Money a, Money b);
Number core_money_sub(Money a, Money b);
Number core_floor_rate(Money value, int64_t bps);
Number core_ceil_rate(Money value, int64_t bps);
Number core_money_fx(Money value, int64_t rate);
Number core_proportional(Money total, Money part, Money whole);
Number core_cumulative_delta(Money total, Money before, Money amount, Money whole);
Money core_split_part(Money total, int64_t n, int64_t index);
Price core_quote(const Config *config, const Command *command);
Price core_price_components(const Config *config, Money p, int64_t n, const char *currency, Tier tier);
bool core_installment_allowed(const Config *config, Money p, int64_t n, const char *country);
bool core_capture_installment_allowed(const Config *config, Money p, int64_t n);
Money core_available_credit(Money limit, Money debt, Money held);
Reason core_credit_admission(Money principal, Money fee, Money available);
Reason core_operation_admission(Money principal, Money limit);
Reason core_daily_admission(Money principal, Money used, Money limit);
Reason core_count_admission(int64_t count, int64_t limit);
int64_t core_risk_score(int64_t base, const char *country, bool present, Money principal, int64_t recent);
Decision core_risk_classify(const Config *config, int64_t score);
int64_t core_recent_count(StateView state, const char *account_id);
DailyTotals core_daily_totals(StateView state, const char *account_id, int64_t day);
Reason core_eligibility(StateView state, const Account *account, const Card *card, const Merchant *merchant, const Command *command);
HistoryRecord core_history_record(StateView state, const char *account_id, Decision decision, Money principal);
Money core_authorization_hold(const Authorization *authorization);
Reason core_capture_admission(const Authorization *authorization, int64_t now, Money amount);
Reason core_refund_admission(const Capture *capture, Money amount);
AuthState core_capture_state(Money total, Money captured);
int64_t core_reward_raw(const Config *config, Money principal, Tier tier, Category category);
int64_t core_reward_grant(const Config *config, int64_t raw, int64_t gross);
int64_t core_reward_gross(StateView state, const char *account_id, int64_t cycle);
int64_t core_invoice_due_day(const Config *config, int64_t cycle);
Money core_invoice_minimum(const Config *config, Money total);
Reason core_close_admission(const Config *config, int64_t day, int64_t next, int64_t cycle);
Reason core_payment_admission(Money amount, Money outstanding);
const Account *core_find_account(StateView state, const char *id);
const Card *core_find_card(StateView state, const char *id);
const Merchant *core_find_merchant(StateView state, const char *id);
const Authorization *core_find_auth(StateView state, const char *id);
const Capture *core_find_capture(StateView state, const char *id);
const Invoice *core_find_invoice(StateView state, const char *id);
Money core_lot_principal(const Lot *lot);
Money core_lot_fee(const Lot *lot);
Money core_invoice_outstanding(StateView state, const Invoice *invoice);
Projection core_project_account(StateView state, const Account *account);
bool core_ledger_balanced(StateView state);
Reconciliation core_reconcile(StateView state, const char *account_id);
#endif
