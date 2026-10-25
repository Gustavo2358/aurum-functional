#include "aurum.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

static Result failure(const Command *c, Decision decision, Reason reason)
{
    Result r = result_default(c);
    r.decision = decision;
    r.reason = reason;
    return r;
}

static void event_name(char out[128], const Command *c)
{
    (void)snprintf(out, 128, "%s:%s", operation_name(c->op), c->request_id);
}

Money authorization_hold(const Authorization *a)
{
    Money principal, fee, hold;
    if (a->state != ACTIVE && a->state != PARTIAL) return 0;
    if (!money_sub(a->principal, a->captured_principal, &principal) ||
        !money_sub(a->fee, a->captured_fee, &fee) ||
        !money_add(principal, fee, &hold)) return -1;
    return hold;
}

Reason capture_admission(const Authorization *a, int64_t now, Money amount)
{
    if (!money_valid(amount) || amount == 0) return INVALID_INPUT;
    if (!a) return NOT_FOUND;
    if ((a->state != ACTIVE && a->state != PARTIAL) || now >= a->expires_at)
        return AUTH_STATE;
    Money remaining;
    if (!money_sub(a->principal, a->captured_principal, &remaining)) return NUMERIC_RANGE;
    return amount > remaining ? CAPTURE_AMOUNT : NONE;
}

Reason refund_admission(const Capture *capture, Money amount)
{
    if (!money_valid(amount) || amount == 0) return INVALID_INPUT;
    if (!capture) return NOT_FOUND;
    Money remaining;
    if (!money_sub(capture->principal, capture->refunded_principal, &remaining))
        return NUMERIC_RANGE;
    return amount > remaining ? REFUND_AMOUNT : NONE;
}

AuthState capture_state(Money total, Money captured)
{
    if (captured == 0) return ACTIVE;
    return captured == total ? CAPTURED : PARTIAL;
}

Result authorize_apply(Engine *e, const Command *c)
{
    Account *a = find_account(e, c->account_id);
    if (!a) return failure(c, ERROR, NOT_FOUND);
    Card *card = find_card(e, c->card_id);
    if (!card) return failure(c, ERROR, NOT_FOUND);
    Merchant *merchant = find_merchant(e, c->merchant_id);
    if (!merchant) return failure(c, ERROR, NOT_FOUND);
    if (strcmp(card->account_id, a->id) != 0) return failure(c, ERROR, OWNERSHIP);

    Result r = result_default(c);
    r.decision = DECLINED;
    r.reason = eligibility(e, a, card, merchant, c);
    if (r.reason != NONE) return r;

    Command priced = *c;
    priced.tier = a->tier;
    Price price = quote(&e->config, &priced);
    r.principal = price.principal;
    r.fee = price.fee;
    r.has_quote = true;
    r.reason = price.reason;
    if (r.reason != NONE) {
        if (r.reason == NUMERIC_RANGE || r.reason == INVALID_INPUT) r.decision = ERROR;
        return r;
    }

    Projection p = project_account(e, a);
    r.reason = operation_admission(price.principal, a->per_operation_limit);
    if (r.reason != NONE) return r;
    r.reason = credit_admission(price.principal, price.fee, p.available_credit);
    if (r.reason != NONE) {
        if (r.reason == NUMERIC_RANGE) r.decision = ERROR;
        return r;
    }
    r.reason = daily_admission(price.principal, p.daily_gross_principal, a->daily_limit);
    if (r.reason != NONE) return r;
    r.reason = count_admission(p.daily_count, e->config.daily_count_limit);
    if (r.reason != NONE) return r;

    int64_t score = risk_score(a->base_score, c->country, c->card_present,
                               price.principal, recent_count(e, a->id));
    r.decision = risk_classify(&e->config, score);
    r.reason = r.decision == APPROVED ? NONE : RISK;
    if (r.decision == DECLINED) return r;

    if (!record_decision(e,a->id,r.decision,price.principal))
        return failure(c, ERROR, CAPACITY);
    if (r.decision == REVIEW) return r;

    Authorization auth = {0};
    strcpy(auth.id, c->request_id);
    strcpy(auth.account_id, a->id);
    auth.state = ACTIVE;
    auth.principal = price.principal;
    auth.fee = price.fee;
    auth.approved_at = e->now;
    if (e->now > INT64_MAX - e->config.hold_ttl_minutes)
        return failure(c, ERROR, NUMERIC_RANGE);
    auth.expires_at = e->now + e->config.hold_ttl_minutes;
    auth.installments = c->installments;
    auth.tier = a->tier;
    auth.category = merchant->category;
    Money hold;
    if (!money_add(auth.principal, auth.fee, &hold)) return failure(c, ERROR, NUMERIC_RANGE);
    if (!array_append((void **)&e->auths, &e->auths_len, &e->auths_alloc,
                      e->config.auths_capacity, sizeof auth, &auth))
        return failure(c, ERROR, CAPACITY);
    char event[128];
    event_name(event, c);
    if (!ledger_pair(e, event, a->id, HOLD_ASSET, HOLD_OFFSET, hold))
        return failure(c, ERROR, CAPACITY);
    strcpy(r.auth_id, auth.id);
    r.reserved = hold;
    r.expires_at = auth.expires_at;
    return r;
}

Result capture_apply(Engine *e, const Command *c)
{
    Authorization *auth = find_auth(e, c->auth_id);
    if (!auth) return failure(c, ERROR, NOT_FOUND);
    Reason reason = capture_admission(auth, e->now, c->principal);
    if (reason != NONE)
        return failure(c, reason == INVALID_INPUT || reason == NUMERIC_RANGE ? ERROR : DECLINED, reason);
    if (!capture_installment_allowed(&e->config, c->principal, auth->installments))
        return failure(c, DECLINED, INSTALLMENTS);

    Money fee, total;
    if (!cumulative_delta(auth->fee, auth->captured_principal, c->principal,
                          auth->principal, &fee) ||
        !money_add(c->principal, fee, &total)) return failure(c, ERROR, NUMERIC_RANGE);

    Capture capture = {0};
    strcpy(capture.id, c->request_id);
    strcpy(capture.auth_id, auth->id);
    strcpy(capture.account_id, auth->account_id);
    capture.principal = c->principal;
    capture.fee = fee;
    capture.cycle = (e->now / 1440) / e->config.cycle_days;
    capture.granted = reward_grant(&e->config,
        reward_raw(&e->config, c->principal, auth->tier, auth->category),
        reward_gross(e, auth->account_id, capture.cycle));
    if (!array_append((void **)&e->captures, &e->captures_len, &e->captures_alloc,
                      e->config.captures_capacity, sizeof capture, &capture))
        return failure(c, ERROR, CAPACITY);

    for (int64_t index = 0; index < auth->installments; ++index) {
        Lot lot = {0};
        strcpy(lot.account_id, auth->account_id);
        strcpy(lot.capture_id, capture.id);
        lot.index = index;
        lot.cycle = capture.cycle + index;
        lot.principal = split_part(c->principal, auth->installments, index);
        lot.fee = split_part(fee, auth->installments, index);
        if (!array_append((void **)&e->lots, &e->lots_len, &e->lots_alloc,
                          e->config.lots_capacity, sizeof lot, &lot))
            return failure(c, ERROR, CAPACITY);
    }

    Reward reward = {0};
    strcpy(reward.account_id, auth->account_id);
    strcpy(reward.capture_id, capture.id);
    reward.cycle = capture.cycle;
    reward.granted = capture.granted;
    if (!array_append((void **)&e->rewards, &e->rewards_len, &e->rewards_alloc,
                      e->config.rewards_capacity, sizeof reward, &reward))
        return failure(c, ERROR, CAPACITY);

    char event[128];
    event_name(event, c);
    if (!ledger_pair(e, event, auth->account_id, HOLD_OFFSET, HOLD_ASSET, total) ||
        !ledger_pair(e, event, auth->account_id, RECEIVABLE_PRINCIPAL, MERCHANT_CLEARING, c->principal) ||
        !ledger_pair(e, event, auth->account_id, RECEIVABLE_FEE, FEE_REVENUE, fee))
        return failure(c, ERROR, CAPACITY);
    if (!money_add(auth->captured_principal, c->principal, &auth->captured_principal) ||
        !money_add(auth->captured_fee, fee, &auth->captured_fee))
        return failure(c, ERROR, NUMERIC_RANGE);
    auth->state = capture_state(auth->principal, auth->captured_principal);
    Result r = result_default(c);
    r.decision = OK;
    strcpy(r.auth_id, auth->id);
    strcpy(r.capture_id, capture.id);
    r.principal = c->principal;
    r.fee = fee;
    r.released = total;
    r.points = capture.granted;
    return r;
}

Result cancel_apply(Engine *e, const Command *c)
{
    Authorization *auth = find_auth(e, c->auth_id);
    if (!auth) return failure(c, ERROR, NOT_FOUND);
    Result r = result_default(c);
    strcpy(r.auth_id, auth->id);
    if (auth->state != ACTIVE && auth->state != PARTIAL) {
        r.decision = NOOP;
        return r;
    }
    Money hold = authorization_hold(auth);
    if (!money_valid(hold)) return failure(c, ERROR, NUMERIC_RANGE);
    char event[128];
    event_name(event, c);
    if (!ledger_pair(e, event, auth->account_id, HOLD_OFFSET, HOLD_ASSET, hold))
        return failure(c, ERROR, CAPACITY);
    auth->state = CANCELLED;
    r.decision = OK;
    r.released = hold;
    return r;
}

static int authorization_order(const void *left, const void *right)
{
    const Authorization *a = *(const Authorization *const *)left;
    const Authorization *b = *(const Authorization *const *)right;
    return strcmp(a->id, b->id);
}

Result tick_apply(Engine *e, const Command *c)
{
    if (c->now < e->now) return failure(c, ERROR, INVALID_TIME);
    size_t count = 0;
    for (size_t i = 0; i < e->auths_len; ++i) {
        const Authorization *a = &e->auths[i];
        if ((a->state == ACTIVE || a->state == PARTIAL) && a->expires_at <= c->now) ++count;
    }
    Authorization **expiring = NULL;
    if (count) {
        if (count > SIZE_MAX / sizeof *expiring) return failure(c, ERROR, CAPACITY);
        expiring = malloc(count * sizeof *expiring);
        if (!expiring) return failure(c, ERROR, CAPACITY);
        size_t next = 0;
        for (size_t i = 0; i < e->auths_len; ++i) {
            Authorization *a = &e->auths[i];
            if ((a->state == ACTIVE || a->state == PARTIAL) && a->expires_at <= c->now)
                expiring[next++] = a;
        }
        qsort(expiring, count, sizeof *expiring, authorization_order);
    }
    Result r = result_default(c);
    r.decision = OK;
    for (size_t i = 0; i < count; ++i) {
        Authorization *a = expiring[i];
        Money hold = authorization_hold(a);
        if (!money_valid(hold) || hold > INT64_MAX - r.released) {
            free(expiring);
            return failure(c, ERROR, NUMERIC_RANGE);
        }
        r.released += hold;
        char event[128];
        (void)snprintf(event, sizeof event, "EXPIRE:%s", a->id);
        if (!ledger_pair(e, event, a->account_id, HOLD_OFFSET, HOLD_ASSET, hold)) {
            free(expiring);
            return failure(c, ERROR, CAPACITY);
        }
        a->state = EXPIRED;
    }
    free(expiring);
    e->now = c->now;
    return r;
}

static int refund_lot_order(const void *left, const void *right)
{
    const Lot *a = *(const Lot *const *)left;
    const Lot *b = *(const Lot *const *)right;
    if (a->cycle != b->cycle) return a->cycle > b->cycle ? -1 : 1;
    if (a->index != b->index) return a->index > b->index ? -1 : 1;
    return 0;
}

static bool refund_post(Engine *e, const char *event, const char *account_id,
                        LedgerAccount source, LedgerAccount receivable,
                        Money component, Money cash)
{
    Money unpaid;
    return money_sub(component, cash, &unpaid) &&
        ledger_pair(e, event, account_id, source, receivable, unpaid) &&
        ledger_pair(e, event, account_id, source, CASH, cash);
}

Result refund_apply(Engine *e, const Command *c)
{
    Capture *capture = find_capture(e, c->capture_id);
    if (!capture) return failure(c, ERROR, NOT_FOUND);
    Reason reason = refund_admission(capture, c->principal);
    if (reason != NONE)
        return failure(c, reason == INVALID_INPUT || reason == NUMERIC_RANGE ? ERROR : DECLINED, reason);
    Money fee, points;
    if (!cumulative_delta(capture->fee, capture->refunded_principal, c->principal,
                          capture->principal, &fee) ||
        !cumulative_delta(capture->granted, capture->refunded_principal, c->principal,
                          capture->principal, &points))
        return failure(c, ERROR, NUMERIC_RANGE);
    Account *account = find_account(e, capture->account_id);
    if (!account) return failure(c, ERROR, NOT_FOUND);

    size_t count = 0;
    for (size_t i = 0; i < e->lots_len; ++i)
        if (strcmp(e->lots[i].capture_id, capture->id) == 0) ++count;
    Lot **lots = NULL;
    if (count) {
        if (count > SIZE_MAX / sizeof *lots) return failure(c, ERROR, CAPACITY);
        lots = malloc(count * sizeof *lots);
        if (!lots) return failure(c, ERROR, CAPACITY);
        size_t next = 0;
        for (size_t i = 0; i < e->lots_len; ++i)
            if (strcmp(e->lots[i].capture_id, capture->id) == 0) lots[next++] = &e->lots[i];
        qsort(lots, count, sizeof *lots, refund_lot_order);
    }
    Money principal_cash = c->principal, fee_cash = fee;
    for (size_t i = 0; i < count; ++i) {
        Money principal_due = lot_principal(lots[i]);
        Money fee_due = lot_fee(lots[i]);
        Money principal_part = principal_due < principal_cash ? principal_due : principal_cash;
        Money fee_part = fee_due < fee_cash ? fee_due : fee_cash;
        if (!money_add(lots[i]->cancelled_principal, principal_part, &lots[i]->cancelled_principal) ||
            !money_add(lots[i]->cancelled_fee, fee_part, &lots[i]->cancelled_fee) ||
            !money_sub(principal_cash, principal_part, &principal_cash) ||
            !money_sub(fee_cash, fee_part, &fee_cash)) {
            free(lots);
            return failure(c, ERROR, NUMERIC_RANGE);
        }
    }
    free(lots);

    Money cash;
    if (!money_add(principal_cash, fee_cash, &cash) ||
        !money_add(account->cash_refund_total, cash, &account->cash_refund_total) ||
        !money_add(capture->refunded_principal, c->principal, &capture->refunded_principal) ||
        !money_add(capture->refunded_fee, fee, &capture->refunded_fee) ||
        !money_add(capture->reversed, points, &capture->reversed))
        return failure(c, ERROR, NUMERIC_RANGE);
    Refund refund = {0};
    strcpy(refund.id, c->request_id);
    strcpy(refund.capture_id, capture->id);
    refund.principal = c->principal;
    refund.fee = fee;
    refund.cash = cash;
    refund.points = points;
    if (!array_append((void **)&e->refunds, &e->refunds_len, &e->refunds_alloc,
                      e->config.refunds_capacity, sizeof refund, &refund))
        return failure(c, ERROR, CAPACITY);
    char event[128];
    event_name(event, c);
    if (!refund_post(e, event, capture->account_id, MERCHANT_CLEARING,
                     RECEIVABLE_PRINCIPAL, c->principal, principal_cash) ||
        !refund_post(e, event, capture->account_id, FEE_REVENUE,
                     RECEIVABLE_FEE, fee, fee_cash))
        return failure(c, ERROR, CAPACITY);
    Result r = result_default(c);
    r.decision = OK;
    strcpy(r.capture_id, capture->id);
    r.principal = c->principal;
    r.fee = fee;
    r.cash_refund = cash;
    r.points_reversed = points;
    return r;
}
