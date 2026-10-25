#include "core.h"
#include <limits.h>
#include <string.h>

Config core_config_default(void) {
    return (Config){.fx_brl=10000,.fx_usd=50000,.fx_eur=60000,.international_bps=200,
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
bool core_config_valid(const Config *c) {
    if (c->fx_brl != 10000 || c->fx_usd < 1 || c->fx_usd > 100000 || c->fx_eur < 1 || c->fx_eur > 100000) return false;
    if (c->international_bps < 0 || c->international_bps > 10000 || c->installment_bps_per_extra < 0 || c->installment_bps_per_extra > 10000 || c->minimum_due_bps < 0 || c->minimum_due_bps > 10000) return false;
    if (!core_money_valid(c->tariff_cap) || !core_money_valid(c->pin_threshold) || !core_money_valid(c->minimum_installment) || c->minimum_installment == 0 || !core_money_valid(c->minimum_due_floor) || !core_money_valid(c->late_fee) || !core_money_valid(c->reward_month_cap) || !core_money_valid(c->reward_unit) || c->reward_unit == 0) return false;
    if (c->hold_ttl_minutes < 1 || c->hold_ttl_minutes > 10080 || c->daily_count_limit < 1 || c->daily_count_limit > 100000 || c->review_score < 0 || c->review_score >= c->decline_score || c->decline_score > 100 || c->cycle_days != 30 || c->due_grace_days != 10) return false;
    const size_t capacities[] = {c->accounts_capacity,c->cards_capacity,c->merchants_capacity,c->auths_capacity,c->captures_capacity,c->lots_capacity,c->invoices_capacity,c->history_capacity,c->ledger_capacity,c->keys_capacity,c->rewards_capacity,c->payments_capacity,c->refunds_capacity};
    for (size_t i=0; i<sizeof capacities/sizeof capacities[0]; ++i) if (capacities[i] < 1 || capacities[i] > 1000000) return false;
    return true;
}

bool core_capture_installment_allowed(const Config *config, Money p, int64_t n) {
    return core_money_valid(p) && p > 0 && n >= 1 && n <= 12 &&
        (n == 1 || p / n >= config->minimum_installment);
}
bool core_installment_allowed(const Config *config, Money p, int64_t n, const char *country) {
    return core_capture_installment_allowed(config,p,n) && (!strcmp(country,"BR") || n <= 3);
}
Price core_price_components(const Config *config, Money p, int64_t n, const char *currency, Tier tier) {
    Price result = {.principal=p,.reason=NONE};
    if (!core_money_valid(p) || n < 1 || n > 12) { result.reason=NUMERIC_RANGE; return result; }
    if (strcmp(currency,"BRL") && tier != PREMIUM) {
        Number international = core_floor_rate(p,config->international_bps);
        if (!international.ok) { result.reason=NUMERIC_RANGE; return result; }
        result.international_fee=international.value;
    }
    Number factor = core_checked_mul(config->installment_bps_per_extra,n-1);
    Number product = factor.ok ? core_checked_mul(p,factor.value) : (Number){0};
    if (!product.ok) { result.reason=NUMERIC_RANGE; return result; }
    result.installment_fee=product.value/10000;
    Money sum=result.international_fee+result.installment_fee;
    result.fee=sum < config->tariff_cap ? sum : config->tariff_cap;
    return result;
}
Price core_quote(const Config *config, const Command *command) {
    Price result = {0};
    int64_t rate;
    if (!strcmp(command->currency,"BRL")) rate=config->fx_brl;
    else if (!strcmp(command->currency,"USD")) rate=config->fx_usd;
    else if (!strcmp(command->currency,"EUR")) rate=config->fx_eur;
    else { result.reason=UNSUPPORTED_CURRENCY; return result; }
    Number principal=core_money_fx(command->amount,rate);
    if (!principal.ok) { result.reason=NUMERIC_RANGE; return result; }
    result.principal=principal.value;
    if (!core_installment_allowed(config,result.principal,command->installments,command->country)) {
        result.reason=INSTALLMENTS; return result;
    }
    return core_price_components(config,result.principal,command->installments,command->currency,command->tier);
}
Money core_available_credit(Money limit, Money debt, Money held) {
    if (debt >= limit || held >= limit-debt) return 0;
    return limit-debt-held;
}
Reason core_credit_admission(Money principal, Money fee, Money available) {
    Number total=core_money_add(principal,fee);
    if (!total.ok) return NUMERIC_RANGE;
    return total.value <= available ? NONE : CREDIT_LIMIT;
}
Reason core_operation_admission(Money principal, Money limit) { return principal <= limit ? NONE : OPERATION_LIMIT; }
Reason core_daily_admission(Money principal, Money used, Money limit) {
    return used <= limit && principal <= limit-used ? NONE : DAILY_AMOUNT;
}
Reason core_count_admission(int64_t count, int64_t limit) { return count < limit ? NONE : DAILY_COUNT; }
int64_t core_risk_score(int64_t base, const char *country, bool present, Money principal, int64_t recent) {
    int64_t score=base;
    if (strcmp(country,"BR")) score+=15;
    if (!present) score+=20;
    if (principal >= 100000) score+=15;
    if (recent >= 3) score+=20;
    return score < 100 ? score : 100;
}
Decision core_risk_classify(const Config *config, int64_t score) {
    if (score >= config->decline_score) return DECLINED;
    if (score >= config->review_score) return REVIEW;
    return APPROVED;
}
Reason core_eligibility(StateView state, const Account *account, const Card *card,
                        const Merchant *merchant, const Command *command) {
    if (!account->active) return ACCOUNT_INACTIVE;
    if (!card->active) return CARD_BLOCKED;
    if (state.now/1440 > card->expiry_day) return CARD_EXPIRED;
    if (!merchant->active) return MERCHANT_BLOCKED;
    if (merchant->category == RESTRICTED) return RESTRICTED_CATEGORY;
    if (strcmp(command->country,"BR") && !card->allow_international) return INTERNATIONAL_DISABLED;
    if (command->channel == CONTACTLESS && !card->allow_contactless) return CONTACTLESS_DISABLED;
    if (command->card_present && command->amount > state.config.pin_threshold && !command->pin_ok) return PIN_REQUIRED;
    for (size_t i=0; i<state.invoices_len; ++i) {
        const Invoice *invoice=&state.invoices[i];
        if (!strcmp(invoice->account_id,account->id) && state.now/1440 > invoice->due_day &&
            core_invoice_outstanding(state,invoice) > 0) return PAST_DUE;
    }
    return NONE;
}
Money core_authorization_hold(const Authorization *authorization) {
    if (authorization->state != ACTIVE && authorization->state != PARTIAL) return 0;
    Number principal=core_money_sub(authorization->principal,authorization->captured_principal);
    Number fee=core_money_sub(authorization->fee,authorization->captured_fee);
    if (!principal.ok || !fee.ok) return -1;
    Number hold=core_money_add(principal.value,fee.value);
    return hold.ok ? hold.value : -1;
}
Reason core_capture_admission(const Authorization *authorization, int64_t now, Money amount) {
    if (!core_money_valid(amount) || !amount) return INVALID_INPUT;
    if (!authorization) return NOT_FOUND;
    if ((authorization->state != ACTIVE && authorization->state != PARTIAL) || now >= authorization->expires_at) return AUTH_STATE;
    Number remaining=core_money_sub(authorization->principal,authorization->captured_principal);
    if (!remaining.ok) return NUMERIC_RANGE;
    return amount > remaining.value ? CAPTURE_AMOUNT : NONE;
}
Reason core_refund_admission(const Capture *capture, Money amount) {
    if (!core_money_valid(amount) || !amount) return INVALID_INPUT;
    if (!capture) return NOT_FOUND;
    Number remaining=core_money_sub(capture->principal,capture->refunded_principal);
    if (!remaining.ok) return NUMERIC_RANGE;
    return amount > remaining.value ? REFUND_AMOUNT : NONE;
}
AuthState core_capture_state(Money total, Money captured) {
    if (!captured) return ACTIVE;
    return captured == total ? CAPTURED : PARTIAL;
}
int64_t core_reward_raw(const Config *config, Money principal, Tier tier, Category category) {
    if (category == RESTRICTED) return 0;
    return (principal/config->reward_unit)*(tier == PREMIUM ? 2 : 1);
}
int64_t core_reward_grant(const Config *config, int64_t raw, int64_t gross) {
    int64_t remaining=gross >= config->reward_month_cap ? 0 : config->reward_month_cap-gross;
    return raw < remaining ? raw : remaining;
}
int64_t core_invoice_due_day(const Config *config, int64_t cycle) {
    if (cycle < 0 || cycle > (INT64_MAX-config->due_grace_days)/config->cycle_days-1) return -1;
    return (cycle+1)*config->cycle_days+config->due_grace_days;
}
Money core_invoice_minimum(const Config *config, Money total) {
    Number minimum=core_ceil_rate(total,config->minimum_due_bps);
    if (!minimum.ok) return -1;
    Money value=minimum.value < config->minimum_due_floor ? config->minimum_due_floor : minimum.value;
    return value < total ? value : total;
}
Reason core_close_admission(const Config *config, int64_t day, int64_t next, int64_t cycle) {
    if (cycle < 0 || cycle > next) return CYCLE_ORDER;
    if (cycle < next) return NONE;
    if (cycle > INT64_MAX/config->cycle_days-1 || day < (cycle+1)*config->cycle_days) return CYCLE_OPEN;
    return NONE;
}
Reason core_payment_admission(Money amount, Money outstanding) {
    if (!core_money_valid(amount) || !amount) return INVALID_INPUT;
    return amount <= outstanding ? NONE : OVERPAYMENT;
}
