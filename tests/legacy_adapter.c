/* Test-only ABI bridge: fixture preparation and direct calls to the new core.
 * Never linked into build/aurum. No business expectation or oracle lives here. */
#include "aurum.h"
#include <string.h>

static StateView adapter_view(const Engine *e) {
    StateView s = {.config=e->config, .now=e->now};
    s.accounts=e->accounts; s.accounts_len=e->accounts_len;
    s.cards=e->cards; s.cards_len=e->cards_len;
    s.merchants=e->merchants; s.merchants_len=e->merchants_len;
    s.auths=e->auths; s.auths_len=e->auths_len;
    s.captures=e->captures; s.captures_len=e->captures_len;
    s.lots=e->lots; s.lots_len=e->lots_len;
    s.invoices=e->invoices; s.invoices_len=e->invoices_len;
    s.history=e->history; s.history_len=e->history_len;
    s.ledger=e->ledger; s.ledger_len=e->ledger_len;
    s.keys=e->keys; s.keys_len=e->keys_len;
    s.payments=e->payments; s.payments_len=e->payments_len;
    s.refunds=e->refunds; s.refunds_len=e->refunds_len;
    s.rewards=e->rewards; s.rewards_len=e->rewards_len;
    return s;
}

bool money_valid(Money v) { return core_money_valid(v); }
bool checked_mul(int64_t a, int64_t b, int64_t *out) { Number n=core_checked_mul(a, b); if(n.ok) *out=n.value; return n.ok; }
bool money_add(Money a, Money b, Money *out) { Number n=core_money_add(a, b); if(n.ok) *out=n.value; return n.ok; }
bool money_sub(Money a, Money b, Money *out) { Number n=core_money_sub(a, b); if(n.ok) *out=n.value; return n.ok; }
bool floor_rate(Money v, int64_t bps, Money *out) { Number n=core_floor_rate(v, bps); if(n.ok) *out=n.value; return n.ok; }
bool ceil_rate(Money v, int64_t bps, Money *out) { Number n=core_ceil_rate(v, bps); if(n.ok) *out=n.value; return n.ok; }
bool money_fx(Money v, int64_t rate, Money *out) { Number n=core_money_fx(v, rate); if(n.ok) *out=n.value; return n.ok; }
bool proportional(Money total, Money part, Money whole, Money *out) { Number n=core_proportional(total, part, whole); if(n.ok) *out=n.value; return n.ok; }
bool cumulative_delta(Money total, Money before, Money amount, Money whole, Money *out) { Number n=core_cumulative_delta(total, before, amount, whole); if(n.ok) *out=n.value; return n.ok; }
Money split_part(Money total, int64_t n, int64_t index) { return core_split_part(total, n, index); }
void config_default(Config *c) { *c=core_config_default(); }
bool config_valid(const Config *c) { return core_config_valid(c); }
bool installment_allowed(const Config *c, Money p, int64_t n, const char *country) { return core_installment_allowed(c, p, n, country); }
bool capture_installment_allowed(const Config *c, Money p, int64_t n) { return core_capture_installment_allowed(c, p, n); }
Price price_components(const Config *c, Money p, int64_t n, const char *currency, Tier tier) { return core_price_components(c, p, n, currency, tier); }
Price quote(const Config *c, const Command *q) { return core_quote(c, q); }
Money available_credit(Money limit, Money debt, Money held) { return core_available_credit(limit, debt, held); }
Reason credit_admission(Money p, Money f, Money available) { return core_credit_admission(p, f, available); }
Reason operation_admission(Money p, Money limit) { return core_operation_admission(p, limit); }
Reason daily_admission(Money p, Money used, Money limit) { return core_daily_admission(p, used, limit); }
Reason count_admission(int64_t count, int64_t limit) { return core_count_admission(count, limit); }
int64_t risk_score(int64_t base, const char *country, bool present, Money p, int64_t recent) { return core_risk_score(base, country, present, p, recent); }
Decision risk_classify(const Config *c, int64_t score) { return core_risk_classify(c, score); }
Money authorization_hold(const Authorization *a) { return core_authorization_hold(a); }
Reason capture_admission(const Authorization *a, int64_t now, Money amount) { return core_capture_admission(a, now, amount); }
Reason refund_admission(const Capture *c, Money amount) { return core_refund_admission(c, amount); }
AuthState capture_state(Money total, Money captured) { return core_capture_state(total, captured); }
int64_t reward_raw(const Config *c, Money p, Tier tier, Category category) { return core_reward_raw(c, p, tier, category); }
int64_t reward_grant(const Config *c, int64_t raw, int64_t gross) { return core_reward_grant(c, raw, gross); }
int64_t invoice_due_day(const Config *c, int64_t cycle) { return core_invoice_due_day(c, cycle); }
Money invoice_minimum(const Config *c, Money total) { return core_invoice_minimum(c, total); }
Reason close_admission(const Config *c, int64_t day, int64_t next, int64_t cycle) { return core_close_admission(c, day, next, cycle); }
Reason payment_admission(Money amount, Money outstanding) { return core_payment_admission(amount, outstanding); }
void command_default(Command *c, Operation op) { *c=core_command_default(op); }
Result result_default(const Command *c) { return core_result_default(c); }
Money lot_principal(const Lot *l) { return core_lot_principal(l); }
Money lot_fee(const Lot *l) { return core_lot_fee(l); }
bool command_valid(const Command *c) { return core_command_valid(c); }
bool command_equal(const Command *a, const Command *b) { return core_command_equal(a, b); }
bool command_same_key(const Command *a, const Command *b) { return core_command_same_key(a, b); }
bool cacheable(const Result *r) { return core_cacheable(r); }
bool identifier_valid(const char *s) { return core_identifier_valid(s); }
int64_t recent_count(const Engine *e,const char *id) { return core_recent_count(adapter_view(e),id); }
void daily_totals(const Engine *e,const char *id,int64_t day,Money *gross,int64_t *count) { DailyTotals d=core_daily_totals(adapter_view(e),id,day); *gross=d.gross; *count=d.count; }
Reason eligibility(const Engine *e,const Account *a,const Card *c,const Merchant *m,const Command *q) { return core_eligibility(adapter_view(e),a,c,m,q); }
int64_t reward_gross(const Engine *e,const char *id,int64_t cycle) { return core_reward_gross(adapter_view(e),id,cycle); }
Money invoice_outstanding(const Engine *e,const Invoice *i) { return core_invoice_outstanding(adapter_view(e),i); }
Projection project_account(const Engine *e,const Account *a) { return core_project_account(adapter_view(e),a); }
bool ledger_balanced(const Engine *e) { return core_ledger_balanced(adapter_view(e)); }
bool reconcile(const Engine *e,const char *id,Money *debt,Money *held) { Reconciliation r=core_reconcile(adapter_view(e),id); *debt=r.debt_difference; *held=r.held_difference; return r.valid; }
Account *find_account(const Engine *e,const char *id) { return (Account *)core_find_account(adapter_view(e),id); }
Card *find_card(const Engine *e,const char *id) { return (Card *)core_find_card(adapter_view(e),id); }
Merchant *find_merchant(const Engine *e,const char *id) { return (Merchant *)core_find_merchant(adapter_view(e),id); }
Authorization *find_auth(const Engine *e,const char *id) { return (Authorization *)core_find_auth(adapter_view(e),id); }
Capture *find_capture(const Engine *e,const char *id) { return (Capture *)core_find_capture(adapter_view(e),id); }
Invoice *find_invoice(const Engine *e,const char *id) { return (Invoice *)core_find_invoice(adapter_view(e),id); }

/* Historical opening records are test inputs, never production operations. */
bool ledger_post(Engine *e,const char *event,const char *id,LedgerAccount account,Money amount) {
    if (!amount) return true;
    if (amount < -MONEY_MAX || amount > MONEY_MAX || strlen(event)>=sizeof ((Entry *)0)->event_id || !core_identifier_valid(id)) return false;
    Entry entry={.account=account,.amount=amount};
    strcpy(entry.event_id,event); strcpy(entry.account_id,id);
    return array_append((void **)&e->ledger,&e->ledger_len,&e->ledger_alloc,e->config.ledger_capacity,sizeof entry,&entry);
}
bool ledger_pair(Engine *e,const char *event,const char *id,LedgerAccount debit,LedgerAccount credit,Money value) {
    if (!core_money_valid(value)) return false;
    return ledger_post(e,event,id,debit,value) && ledger_post(e,event,id,credit,-value);
}
bool refresh_projections(Engine *e) {
    for(size_t i=0;i<e->accounts_len;++i) {
        Projection p=core_project_account(adapter_view(e),&e->accounts[i]);
        if(!core_money_valid(p.debt)||!core_money_valid(p.held)||!core_money_valid(p.cash_refund_total)) return false;
        e->accounts[i].cached_debt=p.debt; e->accounts[i].cached_held=p.held;
    }
    return true;
}
bool record_decision(Engine *e,const char *id,Decision decision,Money principal) {
    HistoryRecord record=core_history_record(adapter_view(e),id,decision,principal);
    if(!record.valid) return false;
    if(!record.store) return true;
    return array_append((void **)&e->history,&e->history_len,&e->history_alloc,e->config.history_capacity,sizeof record.value,&record.value);
}

