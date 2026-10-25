#ifndef AURUM_H
#define AURUM_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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
/* Engine owns every array. Callers own Command/Result values. No pointers survive a mutation. */
typedef struct {
 Config config; int64_t now;
 Account *accounts; size_t accounts_len, accounts_alloc;
 Card *cards; size_t cards_len, cards_alloc;
 Merchant *merchants; size_t merchants_len, merchants_alloc;
 Authorization *auths; size_t auths_len, auths_alloc;
 Capture *captures; size_t captures_len, captures_alloc;
 Lot *lots; size_t lots_len, lots_alloc;
 Invoice *invoices; size_t invoices_len, invoices_alloc;
 History *history; size_t history_len, history_alloc;
 Entry *ledger; size_t ledger_len, ledger_alloc;
 Cached *keys; size_t keys_len, keys_alloc;
 Payment *payments; size_t payments_len, payments_alloc;
 Refund *refunds; size_t refunds_len, refunds_alloc;
 Reward *rewards; size_t rewards_len, rewards_alloc;
} Engine;
typedef struct { Money principal, international_fee, installment_fee, fee; Reason reason; } Price;
typedef struct { Money debt, held, available_credit, invoiced_outstanding, future_outstanding, cash_refund_total, daily_gross_principal; int64_t points, daily_count, next_cycle_to_close; } Projection;

bool money_valid(Money v);
bool checked_mul(int64_t a, int64_t b, int64_t *out);
bool money_add(Money a, Money b, Money *out);
bool money_sub(Money a, Money b, Money *out);
bool floor_rate(Money v, int64_t bps, Money *out);
bool ceil_rate(Money v, int64_t bps, Money *out);
bool money_fx(Money v, int64_t rate, Money *out);
bool proportional(Money total, Money part, Money whole, Money *out);
bool cumulative_delta(Money total, Money before, Money amount, Money whole, Money *out);
Money split_part(Money total, int64_t n, int64_t index);
void config_default(Config *c);
bool config_valid(const Config *c);
bool installment_allowed(const Config *c, Money p, int64_t n, const char *country);
bool capture_installment_allowed(const Config *c, Money p, int64_t n);
Price price_components(const Config *c, Money p, int64_t n, const char *currency, Tier tier);
Price quote(const Config *c, const Command *q);
Money available_credit(Money limit, Money debt, Money held);
Reason credit_admission(Money p, Money f, Money available);
Reason operation_admission(Money p, Money limit);
Reason daily_admission(Money p, Money used, Money limit);
Reason count_admission(int64_t count, int64_t limit);
int64_t risk_score(int64_t base, const char *country, bool present, Money p, int64_t recent);
Decision risk_classify(const Config *c, int64_t score);
int64_t recent_count(const Engine *e, const char *account_id);
void daily_totals(const Engine *e, const char *account_id, int64_t day, Money *gross, int64_t *count);
Reason eligibility(const Engine *e, const Account *a, const Card *card, const Merchant *m, const Command *c);
Money authorization_hold(const Authorization *a);
Reason capture_admission(const Authorization *a, int64_t now, Money amount);
Reason refund_admission(const Capture *c, Money amount);
AuthState capture_state(Money total, Money captured);
int64_t reward_raw(const Config *c, Money p, Tier tier, Category category);
int64_t reward_grant(const Config *c, int64_t raw, int64_t gross);
int64_t reward_gross(const Engine *e, const char *account_id, int64_t cycle);
int64_t invoice_due_day(const Config *c, int64_t cycle);
Money invoice_minimum(const Config *c, Money total);
Reason close_admission(const Config *c, int64_t day, int64_t next, int64_t cycle);
Reason payment_admission(Money amount, Money outstanding);

void engine_init(Engine *e);
void engine_free(Engine *e);
bool engine_clone(const Engine *src, Engine *dst);
bool array_append(void **data, size_t *len, size_t *alloc, size_t limit, size_t size, const void *value);
Account *find_account(const Engine *e, const char *id);
Card *find_card(const Engine *e, const char *id);
Merchant *find_merchant(const Engine *e, const char *id);
Authorization *find_auth(const Engine *e, const char *id);
Capture *find_capture(const Engine *e, const char *id);
Invoice *find_invoice(const Engine *e, const char *id);
Money lot_principal(const Lot *l);
Money lot_fee(const Lot *l);
Money invoice_outstanding(const Engine *e, const Invoice *in);
Projection project_account(const Engine *e, const Account *a);
bool ledger_post(Engine *e, const char *event, const char *account_id, LedgerAccount account, Money amount);
bool ledger_pair(Engine *e, const char *event, const char *account_id, LedgerAccount debit, LedgerAccount credit, Money value);
bool ledger_balanced(const Engine *e);
bool reconcile(const Engine *e, const char *account_id, Money *debt_difference, Money *held_difference);
bool refresh_projections(Engine *e);
void command_default(Command *c, Operation op);
Result result_default(const Command *c);
bool command_valid(const Command *c);
bool command_equal(const Command *a, const Command *b);
bool command_same_key(const Command *a, const Command *b);
bool record_decision(Engine *e, const char *account_id, Decision decision, Money principal);
bool cacheable(const Result *r);
Result execute(Engine *e, const Command *c);
Result authorize_apply(Engine *e, const Command *c);
Result capture_apply(Engine *e, const Command *c);
Result cancel_apply(Engine *e, const Command *c);
Result tick_apply(Engine *e, const Command *c);
Result refund_apply(Engine *e, const Command *c);
Result close_apply(Engine *e, const Command *c);
Result pay_apply(Engine *e, const Command *c);
Result assess_apply(Engine *e, const Command *c);

bool identifier_valid(const char *s);
bool decimal_parse(const char *s, int64_t max, int64_t *value);
/* parse_command: 1 command, 0 ignored line, -1 invalid envelope. */
int parse_command(const char *line, Command *c);
bool seed_load(FILE *f, Engine *e);
/* read_line: 1 complete, 0 EOF, -1 too long/invalid byte, -2 I/O error. */
int read_line(FILE *f, char *line, size_t size);
const char *operation_name(Operation op);
const char *decision_name(Decision d);
const char *reason_name(Reason r);
const char *state_name(AuthState s);
const char *ledger_name(LedgerAccount a);
void write_result(FILE *f, const Engine *e, const Command *c, const Result *r);
#endif
