#include "aurum.h"
#include <string.h>

void config_default(Config *c) {
    *c = (Config){.fx_brl=10000,.fx_usd=50000,.fx_eur=60000,.international_bps=200,
        .installment_bps_per_extra=50,.tariff_cap=5000,.pin_threshold=10000,
        .minimum_installment=500,.minimum_due_floor=1000,.late_fee=1000,
        .hold_ttl_minutes=1440,.daily_count_limit=10,.review_score=50,.decline_score=80,
        .cycle_days=30,.due_grace_days=10,.minimum_due_bps=1000,.reward_month_cap=1000,
        .reward_unit=10000,.accounts_capacity=100000,.cards_capacity=100000,
        .merchants_capacity=100000,.auths_capacity=100000,.captures_capacity=100000,
        .lots_capacity=100000,.invoices_capacity=100000,.history_capacity=100000,
        .ledger_capacity=100000,.keys_capacity=100000,.rewards_capacity=100000,
        .payments_capacity=100000,.refunds_capacity=100000};
}
bool config_valid(const Config *c) {
    if (c->fx_brl != 10000 || c->fx_usd < 1 || c->fx_usd > 100000 || c->fx_eur < 1 || c->fx_eur > 100000) return false;
    if (c->international_bps < 0 || c->international_bps > 10000 || c->installment_bps_per_extra < 0 || c->installment_bps_per_extra > 10000 || c->minimum_due_bps < 0 || c->minimum_due_bps > 10000) return false;
    if (!money_valid(c->tariff_cap) || !money_valid(c->pin_threshold) || !money_valid(c->minimum_installment) || c->minimum_installment == 0 || !money_valid(c->minimum_due_floor) || !money_valid(c->late_fee) || !money_valid(c->reward_month_cap) || !money_valid(c->reward_unit) || c->reward_unit == 0) return false;
    if (c->hold_ttl_minutes < 1 || c->hold_ttl_minutes > 10080 || c->daily_count_limit < 1 || c->daily_count_limit > 100000 || c->review_score < 0 || c->review_score >= c->decline_score || c->decline_score > 100 || c->cycle_days != 30 || c->due_grace_days != 10) return false;
    const size_t capacities[] = {c->accounts_capacity,c->cards_capacity,c->merchants_capacity,c->auths_capacity,c->captures_capacity,c->lots_capacity,c->invoices_capacity,c->history_capacity,c->ledger_capacity,c->keys_capacity,c->rewards_capacity,c->payments_capacity,c->refunds_capacity};
    for (size_t i=0; i<sizeof capacities/sizeof capacities[0]; ++i) if (capacities[i] < 1 || capacities[i] > 1000000) return false;
    return true;
}
bool capture_installment_allowed(const Config *c, Money p, int64_t n) {
    return money_valid(p) && p > 0 && n >= 1 && n <= 12 && (n == 1 || p / n >= c->minimum_installment);
}
bool installment_allowed(const Config *c, Money p, int64_t n, const char *country) {
    return capture_installment_allowed(c,p,n) && (strcmp(country,"BR") == 0 || n <= 3);
}
Price price_components(const Config *c, Money p, int64_t n, const char *currency, Tier tier) {
    Price result = {.principal=p,.reason=NONE};
    int64_t factor, product;
    if (!money_valid(p) || n < 1 || n > 12) { result.reason=NUMERIC_RANGE; return result; }
    if (strcmp(currency,"BRL") != 0 && tier != PREMIUM && !floor_rate(p,c->international_bps,&result.international_fee)) { result.reason=NUMERIC_RANGE; return result; }
    if (!checked_mul(c->installment_bps_per_extra,n-1,&factor) || !checked_mul(p,factor,&product)) { result.reason=NUMERIC_RANGE; return result; }
    result.installment_fee = product / 10000;
    Money sum = result.international_fee + result.installment_fee;
    result.fee = sum < c->tariff_cap ? sum : c->tariff_cap;
    return result;
}
Price quote(const Config *c, const Command *q) {
    Price result = {.reason=NONE};
    int64_t rate;
    if (strcmp(q->currency,"BRL") == 0) rate=c->fx_brl;
    else if (strcmp(q->currency,"USD") == 0) rate=c->fx_usd;
    else if (strcmp(q->currency,"EUR") == 0) rate=c->fx_eur;
    else { result.reason=UNSUPPORTED_CURRENCY; return result; }
    if (!money_fx(q->amount,rate,&result.principal)) { result.reason=NUMERIC_RANGE; return result; }
    if (!installment_allowed(c,result.principal,q->installments,q->country)) { result.reason=INSTALLMENTS; return result; }
    return price_components(c,result.principal,q->installments,q->currency,q->tier);
}
Money available_credit(Money limit, Money debt, Money held) {
    if (debt >= limit || held >= limit-debt) return 0;
    return limit-debt-held;
}
Reason credit_admission(Money p, Money f, Money available) {
    Money total;
    if (!money_add(p,f,&total)) return NUMERIC_RANGE;
    return total <= available ? NONE : CREDIT_LIMIT;
}
Reason operation_admission(Money p, Money limit) { return p <= limit ? NONE : OPERATION_LIMIT; }
Reason daily_admission(Money p, Money used, Money limit) {
    return used <= limit && p <= limit-used ? NONE : DAILY_AMOUNT;
}
Reason count_admission(int64_t count, int64_t limit) { return count < limit ? NONE : DAILY_COUNT; }
int64_t risk_score(int64_t base, const char *country, bool present, Money p, int64_t recent) {
    int64_t score=base;
    if (strcmp(country,"BR") != 0) score+=15;
    if (!present) score+=20;
    if (p >= 100000) score+=15;
    if (recent >= 3) score+=20;
    return score < 100 ? score : 100;
}
Decision risk_classify(const Config *c, int64_t score) {
    if (score >= c->decline_score) return DECLINED;
    if (score >= c->review_score) return REVIEW;
    return APPROVED;
}
int64_t recent_count(const Engine *e, const char *account_id) {
    int64_t count=0;
    for (size_t i=0; i<e->history_len; ++i) {
        const History *h=&e->history[i];
        if (strcmp(h->account_id,account_id) != 0 || h->decision == DECLINED) continue;
        if ((h->decision == APPROVED || h->decision == REVIEW) && h->minute >= e->now-60 && h->minute <= e->now) ++count;
    }
    return count;
}
void daily_totals(const Engine *e, const char *account_id, int64_t day, Money *gross, int64_t *count) {
    *gross=0; *count=0;
    for (size_t i=0; i<e->history_len; ++i) {
        const History *h=&e->history[i];
        if (strcmp(h->account_id,account_id) != 0 || h->minute/1440 != day) continue;
        if (h->decision == APPROVED) { *gross+=h->principal; ++*count; }
        else if (h->decision == REVIEW) ++*count;
    }
}
Reason eligibility(const Engine *e, const Account *a, const Card *card, const Merchant *m, const Command *c) {
    if (!a->active) return ACCOUNT_INACTIVE;
    if (!card->active) return CARD_BLOCKED;
    if (e->now/1440 > card->expiry_day) return CARD_EXPIRED;
    if (!m->active) return MERCHANT_BLOCKED;
    if (m->category == RESTRICTED) return RESTRICTED_CATEGORY;
    if (strcmp(c->country,"BR") != 0 && !card->allow_international) return INTERNATIONAL_DISABLED;
    if (c->channel == CONTACTLESS && !card->allow_contactless) return CONTACTLESS_DISABLED;
    if (c->card_present && c->amount > e->config.pin_threshold && !c->pin_ok) return PIN_REQUIRED;
    for (size_t i=0; i<e->invoices_len; ++i) {
        const Invoice *in=&e->invoices[i];
        if (strcmp(in->account_id,a->id) == 0 && e->now/1440 > in->due_day && invoice_outstanding(e,in) > 0) return PAST_DUE;
    }
    return NONE;
}
int64_t reward_raw(const Config *c, Money p, Tier tier, Category category) {
    if (category == RESTRICTED) return 0;
    return (p/c->reward_unit)*(tier == PREMIUM ? 2 : 1);
}
int64_t reward_grant(const Config *c, int64_t raw, int64_t gross) {
    int64_t remaining=gross >= c->reward_month_cap ? 0 : c->reward_month_cap-gross;
    return raw < remaining ? raw : remaining;
}
int64_t reward_gross(const Engine *e, const char *account_id, int64_t cycle) {
    int64_t total=0;
    for (size_t i=0; i<e->rewards_len; ++i) if (strcmp(e->rewards[i].account_id,account_id) == 0 && e->rewards[i].cycle == cycle) total+=e->rewards[i].granted;
    return total;
}

bool record_decision(Engine *e, const char *account_id, Decision decision, Money principal) {
    if (decision==DECLINED) return true;
    if ((decision!=APPROVED && decision!=REVIEW) || !money_valid(principal) || !find_account(e,account_id)) return false;
    History history={.minute=e->now,.decision=decision,.principal=principal};
    strcpy(history.account_id,account_id);
    return array_append((void **)&e->history,&e->history_len,&e->history_alloc,e->config.history_capacity,sizeof history,&history);
}
