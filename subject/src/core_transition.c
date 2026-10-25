#include "core.h"
#include <limits.h>
#include <string.h>

enum Phase {
    BEGIN, AUTHORIZE_START, AUTHORIZE_AUTH, AUTHORIZE_POST, AUTHORIZE_RESULT,
    CAPTURE_START, CAPTURE_LOTS, CAPTURE_REWARD, CAPTURE_POST, CAPTURE_AUTH, CAPTURE_RESULT,
    CANCEL_START, CANCEL_AUTH, CANCEL_RESULT, TICK_START, TICK_SCAN, TICK_AUTH, TICK_CLOCK,
    REFUND_START, REFUND_LOTS, REFUND_ACCOUNT, REFUND_CAPTURE, REFUND_RECORD, REFUND_POST, REFUND_RESULT,
    CLOSE_START, CLOSE_LOTS, CLOSE_ACCOUNT, CLOSE_RESULT,
    PAY_START, PAY_LATE, PAY_INVOICE, PAY_LOTS, PAY_LOT, PAY_RECORD,
    ASSESS_START, ASSESS_INVOICE, LEDGER, FINISH, CACHE, REFRESH
};
typedef struct { char text[128]; } Name;

static Name event_name(Operation op, const char *request_id) {
    const char *prefix="";
    switch (op) {
        case AUTHORIZE: prefix="AUTHORIZE"; break;
        case CAPTURE: prefix="CAPTURE"; break;
        case CANCEL: prefix="CANCEL"; break;
        case REFUND: prefix="REFUND"; break;
        case PAY: prefix="PAY"; break;
        case ASSESS_LATE_FEE: prefix="ASSESS_LATE_FEE"; break;
        case TICK: prefix="EXPIRE"; break;
        default: break;
    }
    Name name={0};
    size_t length=0;
    for (size_t i=0; prefix[i]; ++i) name.text[length++]=prefix[i];
    name.text[length++]=':';
    for (size_t i=0; request_id[i]; ++i) name.text[length++]=request_id[i];
    return name;
}
static Name invoice_name(size_t number) {
    Name name={0};
    char reverse[32]={0};
    size_t length=0;
    do { reverse[length++]=(char)('0'+number%10); number/=10; } while (number);
    name.text[0]='I';
    for (size_t i=0; i<length; ++i) name.text[i+1]=reverse[length-i-1];
    return name;
}
static Result failed(Command command, Reason reason) {
    Result result=core_result_default(&command);
    result.decision=ERROR;
    result.reason=reason;
    return result;
}
Result core_capacity_failure(Command command) { return failed(command,CAPACITY); }
Continuation core_begin(void) { return (Continuation){0}; }
static Continuation fail(Continuation continuation, Command command, Reason reason) {
    continuation.result=failed(command,reason);
    return continuation;
}
static Continuation reject(Continuation continuation, Reason reason) {
    continuation.result.reason=reason;
    continuation.result.decision=(reason == INVALID_INPUT || reason == NUMERIC_RANGE) ? ERROR : DECLINED;
    continuation.phase=FINISH;
    return continuation;
}
static Step terminal(CoreStatus status, Continuation continuation) {
    return (Step){.status=status,.next=continuation,.result=continuation.result};
}
static Step edited(StateView state, Continuation continuation, Edit edit) {
    size_t length=0, capacity=0;
    switch (edit.table) {
        case TABLE_ACCOUNT: length=state.accounts_len; capacity=state.config.accounts_capacity; break;
        case TABLE_CARD: length=state.cards_len; capacity=state.config.cards_capacity; break;
        case TABLE_MERCHANT: length=state.merchants_len; capacity=state.config.merchants_capacity; break;
        case TABLE_AUTH: length=state.auths_len; capacity=state.config.auths_capacity; break;
        case TABLE_CAPTURE: length=state.captures_len; capacity=state.config.captures_capacity; break;
        case TABLE_LOT: length=state.lots_len; capacity=state.config.lots_capacity; break;
        case TABLE_INVOICE: length=state.invoices_len; capacity=state.config.invoices_capacity; break;
        case TABLE_HISTORY: length=state.history_len; capacity=state.config.history_capacity; break;
        case TABLE_LEDGER: length=state.ledger_len; capacity=state.config.ledger_capacity; break;
        case TABLE_KEY: length=state.keys_len; capacity=state.config.keys_capacity; break;
        case TABLE_PAYMENT: length=state.payments_len; capacity=state.config.payments_capacity; break;
        case TABLE_REFUND: length=state.refunds_len; capacity=state.config.refunds_capacity; break;
        case TABLE_REWARD: length=state.rewards_len; capacity=state.config.rewards_capacity; break;
        case TABLE_CLOCK: break;
    }
    if (edit.append && length >= capacity) {
        continuation.result=continuation.capacity_result;
        return terminal(CORE_ABORT,continuation);
    }
    if (edit.append) edit.index=length;
    return (Step){.status=CORE_EDIT,.edit=edit,.next=continuation,.result=continuation.result};
}
static Continuation pair(Continuation continuation, const char *account_id,
                         LedgerAccount debit, LedgerAccount credit, Money amount) {
    if (!amount) return continuation;
    Entry debit_entry={.account=debit,.amount=amount};
    strcpy(debit_entry.event_id,continuation.event_id);
    strcpy(debit_entry.account_id,account_id);
    Entry credit_entry=debit_entry;
    credit_entry.account=credit;
    credit_entry.amount=-amount;
    continuation.entries[continuation.entries_len++]=debit_entry;
    continuation.entries[continuation.entries_len++]=credit_entry;
    return continuation;
}
static Continuation post(Continuation continuation, unsigned after) {
    continuation.entry_cursor=0;
    continuation.after_ledger=after;
    continuation.phase=LEDGER;
    return continuation;
}
static size_t next_expiring(StateView state, int64_t now) {
    size_t next=SIZE_MAX;
    for (size_t i=0; i<state.auths_len; ++i) {
        const Authorization *auth=&state.auths[i];
        if ((auth->state != ACTIVE && auth->state != PARTIAL) || auth->expires_at > now) continue;
        if (next == SIZE_MAX || strcmp(auth->id,state.auths[next].id) < 0) next=i;
    }
    return next;
}
static int refund_order(const Lot *a, const Lot *b) {
    if (a->cycle != b->cycle) return a->cycle > b->cycle ? -1 : 1;
    return (a->index < b->index)-(a->index > b->index);
}
static size_t next_refund_lot(StateView state, Continuation continuation) {
    size_t next=SIZE_MAX;
    for (size_t i=0; i<state.lots_len; ++i) {
        const Lot *lot=&state.lots[i];
        if (strcmp(lot->capture_id,continuation.capture.id)) continue;
        if (continuation.ordered_started) {
            int previous=refund_order(lot,&continuation.lot);
            if (previous < 0 || (previous == 0 && i <= continuation.target)) continue;
        }
        if (next == SIZE_MAX || refund_order(lot,&state.lots[next]) < 0) next=i;
    }
    return next;
}
static int invoice_order(const Invoice *a, const Invoice *b) {
    if (a->due_day != b->due_day) return a->due_day < b->due_day ? -1 : 1;
    if (a->cycle != b->cycle) return a->cycle < b->cycle ? -1 : 1;
    return strcmp(a->id,b->id);
}
static size_t next_late(StateView state, const char *account_id) {
    size_t next=SIZE_MAX;
    for (size_t i=0; i<state.invoices_len; ++i) {
        const Invoice *invoice=&state.invoices[i];
        if (strcmp(invoice->account_id,account_id) || invoice->late == invoice->paid_late) continue;
        if (next == SIZE_MAX || invoice_order(invoice,&state.invoices[next]) < 0) next=i;
    }
    return next;
}
static int lot_order(const Lot *a, const Lot *b) {
    if (a->cycle != b->cycle) return a->cycle < b->cycle ? -1 : 1;
    int capture=strcmp(a->capture_id,b->capture_id);
    if (capture) return capture;
    return (a->index > b->index)-(a->index < b->index);
}
static size_t next_payment_lot(StateView state, const char *account_id, unsigned component) {
    size_t next=SIZE_MAX;
    for (size_t i=0; i<state.lots_len; ++i) {
        const Lot *lot=&state.lots[i];
        if (strcmp(lot->account_id,account_id) || !lot->invoice_id[0]) continue;
        Money due=component == 0 ? core_lot_fee(lot) : core_lot_principal(lot);
        if (!due) continue;
        if (next == SIZE_MAX || lot_order(lot,&state.lots[next]) < 0) next=i;
    }
    return next;
}

Step core_step(StateView state, Command command, Continuation continuation) {
    Continuation next=continuation;
    for (;;) {
        if (next.result.decision == ERROR && next.phase != BEGIN) return terminal(CORE_ABORT,next);
        switch (next.phase) {
        case BEGIN: {
            next.result=core_result_default(&command);
            next.capacity_result=core_capacity_failure(command);
            if (!core_command_valid(&command)) return terminal(CORE_ABORT,fail(next,command,INVALID_INPUT));
            if (command.op == QUOTE) {
                Price price=core_quote(&state.config,&command);
                next.result.principal=price.principal; next.result.fee=price.fee;
                next.result.has_quote=true; next.result.reason=price.reason;
                if (price.reason != NONE) next.result.decision=price.reason == NUMERIC_RANGE ? ERROR : DECLINED;
                return terminal(CORE_READ,next);
            }
            if (command.op >= GET_ACCOUNT && command.op <= RECONCILE) {
                bool found=true;
                if (command.op == GET_ACCOUNT) found=core_find_account(state,command.account_id) != NULL;
                if (command.op == GET_AUTH) found=core_find_auth(state,command.auth_id) != NULL;
                if (command.op == GET_INVOICE) found=core_find_invoice(state,command.invoice_id) != NULL;
                if (command.op == RECONCILE && command.account_id[0]) found=core_find_account(state,command.account_id) != NULL;
                if (!found) next=fail(next,command,NOT_FOUND);
                else if (command.op == RECONCILE && !core_reconcile(state,command.account_id).valid)
                    next=fail(next,command,RECONCILIATION);
                return terminal(CORE_READ,next);
            }
            if (command.op != TICK) {
                for (size_t i=0; i<state.keys_len; ++i) {
                    const Cached *key=&state.keys[i];
                    if (!core_command_same_key(&key->command,&command)) continue;
                    if (core_command_equal(&key->command,&command)) next.result=key->result;
                    else next=fail(next,command,IDEMPOTENCY_CONFLICT);
                    return terminal(CORE_READ,next);
                }
            }
            Name event=event_name(command.op,command.request_id);
            strcpy(next.event_id,event.text);
            switch (command.op) {
                case AUTHORIZE: next.phase=AUTHORIZE_START; break;
                case CAPTURE: next.phase=CAPTURE_START; break;
                case CANCEL: next.phase=CANCEL_START; break;
                case REFUND: next.phase=REFUND_START; break;
                case CLOSE: next.phase=CLOSE_START; break;
                case PAY: next.phase=PAY_START; break;
                case ASSESS_LATE_FEE: next.phase=ASSESS_START; break;
                case TICK: next.phase=TICK_START; break;
                default: return terminal(CORE_ABORT,fail(next,command,INVALID_INPUT));
            }
            break;
        }
        case AUTHORIZE_START: {
            const Account *account=core_find_account(state,command.account_id);
            const Card *card=core_find_card(state,command.card_id);
            const Merchant *merchant=core_find_merchant(state,command.merchant_id);
            if (!account || !card || !merchant) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            if (strcmp(card->account_id,account->id)) return terminal(CORE_ABORT,fail(next,command,OWNERSHIP));
            next.result.decision=DECLINED;
            next.result.reason=core_eligibility(state,account,card,merchant,&command);
            next.phase=FINISH;
            if (next.result.reason != NONE) break;
            Command priced=command;
            priced.tier=account->tier;
            Price price=core_quote(&state.config,&priced);
            next.result.principal=price.principal; next.result.fee=price.fee; next.result.has_quote=true;
            next.result.reason=price.reason;
            if (price.reason != NONE) { next=reject(next,price.reason); break; }
            Projection projection=core_project_account(state,account);
            next.result.reason=core_operation_admission(price.principal,account->per_operation_limit);
            if (next.result.reason != NONE) break;
            next.result.reason=core_credit_admission(price.principal,price.fee,projection.available_credit);
            if (next.result.reason != NONE) { next=reject(next,next.result.reason); break; }
            next.result.reason=core_daily_admission(price.principal,projection.daily_gross_principal,account->daily_limit);
            if (next.result.reason != NONE) break;
            next.result.reason=core_count_admission(projection.daily_count,state.config.daily_count_limit);
            if (next.result.reason != NONE) break;
            int64_t score=core_risk_score(account->base_score,command.country,command.card_present,
                                          price.principal,core_recent_count(state,account->id));
            next.result.decision=core_risk_classify(&state.config,score);
            next.result.reason=next.result.decision == APPROVED ? NONE : RISK;
            if (next.result.decision == DECLINED) break;
            HistoryRecord history=core_history_record(state,account->id,next.result.decision,price.principal);
            if (!history.valid) return terminal(CORE_ABORT,fail(next,command,CAPACITY));
            next.account=*account;
            next.auth=(Authorization){.state=ACTIVE,.principal=price.principal,.fee=price.fee,
                .approved_at=state.now,.installments=command.installments,.tier=account->tier,.category=merchant->category};
            strcpy(next.auth.id,command.request_id);
            strcpy(next.auth.account_id,account->id);
            next.phase=next.result.decision == REVIEW ? FINISH : AUTHORIZE_AUTH;
            return edited(state,next,(Edit){.table=TABLE_HISTORY,.append=true,.value.history=history.value});
        }
        case AUTHORIZE_AUTH: {
            if (state.now > INT64_MAX-state.config.hold_ttl_minutes)
                return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            next.auth.expires_at=state.now+state.config.hold_ttl_minutes;
            Number hold=core_money_add(next.auth.principal,next.auth.fee);
            if (!hold.ok) return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            next.remaining=hold.value;
            next.phase=AUTHORIZE_POST;
            return edited(state,next,(Edit){.table=TABLE_AUTH,.append=true,.value.auth=next.auth});
        }
        case AUTHORIZE_POST:
            next=pair(next,next.auth.account_id,HOLD_ASSET,HOLD_OFFSET,next.remaining);
            next=post(next,AUTHORIZE_RESULT);
            break;
        case AUTHORIZE_RESULT:
            strcpy(next.result.auth_id,next.auth.id);
            next.result.reserved=next.remaining;
            next.result.expires_at=next.auth.expires_at;
            next.phase=FINISH;
            break;
        case CAPTURE_START: {
            const Authorization *auth=core_find_auth(state,command.auth_id);
            if (!auth) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            Reason reason=core_capture_admission(auth,state.now,command.principal);
            if (reason != NONE) { next=reject(next,reason); break; }
            if (!core_capture_installment_allowed(&state.config,command.principal,auth->installments)) {
                next=reject(next,INSTALLMENTS); break;
            }
            Number fee=core_cumulative_delta(auth->fee,auth->captured_principal,command.principal,auth->principal);
            Number total=fee.ok ? core_money_add(command.principal,fee.value) : (Number){0};
            if (!total.ok) return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            next.auth=*auth;
            next.target=(size_t)(auth-state.auths);
            next.capture=(Capture){.principal=command.principal,.fee=fee.value,
                .cycle=(state.now/1440)/state.config.cycle_days};
            strcpy(next.capture.id,command.request_id);
            strcpy(next.capture.auth_id,auth->id);
            strcpy(next.capture.account_id,auth->account_id);
            next.capture.granted=core_reward_grant(&state.config,
                core_reward_raw(&state.config,command.principal,auth->tier,auth->category),
                core_reward_gross(state,auth->account_id,next.capture.cycle));
            next.remaining=total.value;
            next.cursor=0;
            next.phase=CAPTURE_LOTS;
            return edited(state,next,(Edit){.table=TABLE_CAPTURE,.append=true,.value.capture=next.capture});
        }
        case CAPTURE_LOTS: {
            if (next.cursor >= (size_t)next.auth.installments) { next.phase=CAPTURE_REWARD; break; }
            Lot lot={.index=(int64_t)next.cursor,.cycle=next.capture.cycle+(int64_t)next.cursor,
                .principal=core_split_part(next.capture.principal,next.auth.installments,(int64_t)next.cursor),
                .fee=core_split_part(next.capture.fee,next.auth.installments,(int64_t)next.cursor)};
            strcpy(lot.account_id,next.auth.account_id);
            strcpy(lot.capture_id,next.capture.id);
            ++next.cursor;
            return edited(state,next,(Edit){.table=TABLE_LOT,.append=true,.value.lot=lot});
        }
        case CAPTURE_REWARD: {
            Reward reward={.cycle=next.capture.cycle,.granted=next.capture.granted};
            strcpy(reward.account_id,next.auth.account_id);
            strcpy(reward.capture_id,next.capture.id);
            next.phase=CAPTURE_POST;
            return edited(state,next,(Edit){.table=TABLE_REWARD,.append=true,.value.reward=reward});
        }
        case CAPTURE_POST:
            next=pair(next,next.auth.account_id,HOLD_OFFSET,HOLD_ASSET,next.remaining);
            next=pair(next,next.auth.account_id,RECEIVABLE_PRINCIPAL,MERCHANT_CLEARING,next.capture.principal);
            next=pair(next,next.auth.account_id,RECEIVABLE_FEE,FEE_REVENUE,next.capture.fee);
            next=post(next,CAPTURE_AUTH);
            break;
        case CAPTURE_AUTH: {
            Number principal=core_money_add(next.auth.captured_principal,command.principal);
            Number fee=core_money_add(next.auth.captured_fee,next.capture.fee);
            if (!principal.ok || !fee.ok) return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            next.auth.captured_principal=principal.value; next.auth.captured_fee=fee.value;
            next.auth.state=core_capture_state(next.auth.principal,principal.value);
            next.phase=CAPTURE_RESULT;
            return edited(state,next,(Edit){.table=TABLE_AUTH,.index=next.target,.value.auth=next.auth});
        }
        case CAPTURE_RESULT:
            strcpy(next.result.auth_id,next.auth.id);
            strcpy(next.result.capture_id,next.capture.id);
            next.result.principal=command.principal;
            next.result.fee=next.capture.fee;
            next.result.released=next.remaining;
            next.result.points=next.capture.granted;
            next.phase=FINISH;
            break;
        case CANCEL_START: {
            const Authorization *auth=core_find_auth(state,command.auth_id);
            if (!auth) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            strcpy(next.result.auth_id,auth->id);
            if (auth->state != ACTIVE && auth->state != PARTIAL) {
                next.result.decision=NOOP; next.phase=FINISH; break;
            }
            next.remaining=core_authorization_hold(auth);
            if (!core_money_valid(next.remaining)) return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            next.auth=*auth;
            next.target=(size_t)(auth-state.auths);
            next=pair(next,auth->account_id,HOLD_OFFSET,HOLD_ASSET,next.remaining);
            next=post(next,CANCEL_AUTH);
            break;
        }
        case CANCEL_AUTH:
            next.auth.state=CANCELLED;
            next.phase=CANCEL_RESULT;
            return edited(state,next,(Edit){.table=TABLE_AUTH,.index=next.target,.value.auth=next.auth});
        case CANCEL_RESULT:
            next.result.released=next.remaining;
            next.phase=FINISH;
            break;
        case TICK_START:
            if (command.now < state.now) return terminal(CORE_ABORT,fail(next,command,INVALID_TIME));
            next.phase=TICK_SCAN;
            break;
        case TICK_SCAN: {
            next.target=next_expiring(state,command.now);
            if (next.target == SIZE_MAX) { next.phase=TICK_CLOCK; break; }
            next.auth=state.auths[next.target];
            Money hold=core_authorization_hold(&next.auth);
            if (!core_money_valid(hold) || hold > INT64_MAX-next.result.released)
                return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            next.result.released+=hold;
            Name event=event_name(TICK,next.auth.id);
            strcpy(next.event_id,event.text);
            next=pair(next,next.auth.account_id,HOLD_OFFSET,HOLD_ASSET,hold);
            next=post(next,TICK_AUTH);
            break;
        }
        case TICK_AUTH:
            next.auth.state=EXPIRED;
            next.phase=TICK_SCAN;
            return edited(state,next,(Edit){.table=TABLE_AUTH,.index=next.target,.value.auth=next.auth});
        case TICK_CLOCK:
            next.phase=FINISH;
            return edited(state,next,(Edit){.table=TABLE_CLOCK,.value.now=command.now});
        case REFUND_START: {
            const Capture *capture=core_find_capture(state,command.capture_id);
            if (!capture) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            Reason reason=core_refund_admission(capture,command.principal);
            if (reason != NONE) { next=reject(next,reason); break; }
            Number fee=core_cumulative_delta(capture->fee,capture->refunded_principal,command.principal,capture->principal);
            Number points=core_cumulative_delta(capture->granted,capture->refunded_principal,command.principal,capture->principal);
            if (!fee.ok || !points.ok) return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            const Account *account=core_find_account(state,capture->account_id);
            if (!account) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            next.capture=*capture;
            next.account=*account;
            next.cursor=(size_t)(capture-state.captures);
            next.account_index=(size_t)(account-state.accounts);
            next.remaining=command.principal;
            next.fee_remaining=fee.value;
            next.result.fee=fee.value;
            next.result.points_reversed=points.value;
            next.phase=REFUND_LOTS;
            break;
        }
        case REFUND_LOTS: {
            size_t index=next_refund_lot(state,next);
            if (index == SIZE_MAX) { next.phase=REFUND_ACCOUNT; break; }
            Lot lot=state.lots[index];
            Money principal_due=core_lot_principal(&lot), fee_due=core_lot_fee(&lot);
            Money principal_part=principal_due < next.remaining ? principal_due : next.remaining;
            Money fee_part=fee_due < next.fee_remaining ? fee_due : next.fee_remaining;
            Number principal=core_money_add(lot.cancelled_principal,principal_part);
            Number fee=core_money_add(lot.cancelled_fee,fee_part);
            Number remaining=core_money_sub(next.remaining,principal_part);
            Number fee_remaining=core_money_sub(next.fee_remaining,fee_part);
            if (!principal.ok || !fee.ok || !remaining.ok || !fee_remaining.ok)
                return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            lot.cancelled_principal=principal.value; lot.cancelled_fee=fee.value;
            next.remaining=remaining.value; next.fee_remaining=fee_remaining.value;
            next.lot=lot; next.target=index; next.ordered_started=true;
            return edited(state,next,(Edit){.table=TABLE_LOT,.index=index,.value.lot=lot});
        }
        case REFUND_ACCOUNT: {
            Number cash=core_money_add(next.remaining,next.fee_remaining);
            Number cash_total=cash.ok ? core_money_add(next.account.cash_refund_total,cash.value) : (Number){0};
            Number principal=core_money_add(next.capture.refunded_principal,command.principal);
            Number fee=core_money_add(next.capture.refunded_fee,next.result.fee);
            Number reversed=core_money_add(next.capture.reversed,next.result.points_reversed);
            if (!cash_total.ok || !principal.ok || !fee.ok || !reversed.ok)
                return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            next.account.cash_refund_total=cash_total.value;
            next.capture.refunded_principal=principal.value;
            next.capture.refunded_fee=fee.value;
            next.capture.reversed=reversed.value;
            next.result.cash_refund=cash.value;
            next.phase=REFUND_CAPTURE;
            return edited(state,next,(Edit){.table=TABLE_ACCOUNT,.index=next.account_index,.value.account=next.account});
        }
        case REFUND_CAPTURE:
            next.phase=REFUND_RECORD;
            return edited(state,next,(Edit){.table=TABLE_CAPTURE,.index=next.cursor,.value.capture=next.capture});
        case REFUND_RECORD: {
            Refund refund={.principal=command.principal,.fee=next.result.fee,
                .cash=next.result.cash_refund,.points=next.result.points_reversed};
            strcpy(refund.id,command.request_id);
            strcpy(refund.capture_id,next.capture.id);
            next.phase=REFUND_POST;
            return edited(state,next,(Edit){.table=TABLE_REFUND,.append=true,.value.refund=refund});
        }
        case REFUND_POST: {
            Number unpaid_principal=core_money_sub(command.principal,next.remaining);
            Number unpaid_fee=core_money_sub(next.result.fee,next.fee_remaining);
            if (!unpaid_principal.ok || !unpaid_fee.ok) return terminal(CORE_ABORT,fail(next,command,CAPACITY));
            next=pair(next,next.capture.account_id,MERCHANT_CLEARING,RECEIVABLE_PRINCIPAL,unpaid_principal.value);
            next=pair(next,next.capture.account_id,MERCHANT_CLEARING,CASH,next.remaining);
            next=pair(next,next.capture.account_id,FEE_REVENUE,RECEIVABLE_FEE,unpaid_fee.value);
            next=pair(next,next.capture.account_id,FEE_REVENUE,CASH,next.fee_remaining);
            next=post(next,REFUND_RESULT);
            break;
        }
        case REFUND_RESULT:
            strcpy(next.result.capture_id,next.capture.id);
            next.result.principal=command.principal;
            next.phase=FINISH;
            break;
        case CLOSE_START: {
            const Account *account=core_find_account(state,command.account_id);
            if (!account) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            bool existing=false;
            for (size_t i=0; i<state.invoices_len; ++i) {
                const Invoice *invoice=&state.invoices[i];
                if (!strcmp(invoice->account_id,account->id) && invoice->cycle == command.cycle) {
                    next.result.decision=NOOP;
                    strcpy(next.result.invoice_id,invoice->id);
                    existing=true; break;
                }
            }
            if (existing) { next.phase=FINISH; break; }
            if (command.cycle < account->next_cycle_to_close)
                return terminal(CORE_ABORT,fail(next,command,CYCLE_ORDER));
            Reason reason=core_close_admission(&state.config,state.now/1440,account->next_cycle_to_close,command.cycle);
            if (reason != NONE) return terminal(CORE_ABORT,fail(next,command,reason));
            next.account=*account;
            next.account_index=(size_t)(account-state.accounts);
            Invoice invoice={.cycle=command.cycle,.due_day=core_invoice_due_day(&state.config,command.cycle)};
            strcpy(invoice.account_id,account->id);
            Name name=invoice_name(state.invoices_len+1);
            strcpy(invoice.id,name.text);
            for (size_t i=0; i<state.lots_len; ++i) {
                const Lot *lot=&state.lots[i];
                if (strcmp(lot->account_id,account->id) || lot->cycle != command.cycle) continue;
                Number principal=core_money_add(invoice.issued_principal,lot->principal-lot->cancelled_principal);
                Number fee=core_money_add(invoice.issued_fee,lot->fee-lot->cancelled_fee);
                if (!principal.ok || !fee.ok) return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
                invoice.issued_principal=principal.value; invoice.issued_fee=fee.value;
            }
            Number total=core_money_add(invoice.issued_principal,invoice.issued_fee);
            if (!total.ok) return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
            invoice.issued_total=total.value;
            invoice.minimum_due=core_invoice_minimum(&state.config,total.value);
            next.invoice=invoice;
            next.cursor=0;
            next.phase=CLOSE_LOTS;
            return edited(state,next,(Edit){.table=TABLE_INVOICE,.append=true,.value.invoice=invoice});
        }
        case CLOSE_LOTS: {
            while (next.cursor < state.lots_len) {
                size_t index=next.cursor++;
                Lot lot=state.lots[index];
                if (strcmp(lot.account_id,next.account.id) || lot.cycle != command.cycle) continue;
                strcpy(lot.invoice_id,next.invoice.id);
                return edited(state,next,(Edit){.table=TABLE_LOT,.index=index,.value.lot=lot});
            }
            next.phase=CLOSE_ACCOUNT;
            break;
        }
        case CLOSE_ACCOUNT:
            ++next.account.next_cycle_to_close;
            next.phase=CLOSE_RESULT;
            return edited(state,next,(Edit){.table=TABLE_ACCOUNT,.index=next.account_index,.value.account=next.account});
        case CLOSE_RESULT:
            strcpy(next.result.invoice_id,next.invoice.id);
            next.result.principal=next.invoice.issued_principal;
            next.result.fee=next.invoice.issued_fee;
            next.result.issued_total=next.invoice.issued_total;
            next.result.minimum_due=next.invoice.minimum_due;
            next.phase=FINISH;
            break;
        case PAY_START: {
            const Account *account=core_find_account(state,command.account_id);
            if (!account) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            Projection projection=core_project_account(state,account);
            Reason reason=core_payment_admission(command.amount,projection.invoiced_outstanding);
            if (reason != NONE) { next=reject(next,reason); break; }
            next.account=*account;
            next.remaining=command.amount;
            next.phase=PAY_LATE;
            break;
        }
        case PAY_LATE: {
            if (!next.remaining) { next.phase=PAY_RECORD; break; }
            next.target=next_late(state,next.account.id);
            if (next.target == SIZE_MAX) { next.component=0; next.phase=PAY_LOTS; break; }
            next.invoice=state.invoices[next.target];
            Money due=next.invoice.late-next.invoice.paid_late;
            Money paid=due < next.remaining ? due : next.remaining;
            next.invoice.paid_late+=paid;
            next.remaining-=paid;
            next=pair(next,next.account.id,CASH,RECEIVABLE_LATE,paid);
            next=post(next,PAY_INVOICE);
            break;
        }
        case PAY_INVOICE:
            next.phase=PAY_LATE;
            return edited(state,next,(Edit){.table=TABLE_INVOICE,.index=next.target,.value.invoice=next.invoice});
        case PAY_LOTS: {
            if (!next.remaining || next.component >= 2) { next.phase=PAY_RECORD; break; }
            next.target=next_payment_lot(state,next.account.id,next.component);
            if (next.target == SIZE_MAX) { ++next.component; break; }
            next.lot=state.lots[next.target];
            Money due=next.component == 0 ? core_lot_fee(&next.lot) : core_lot_principal(&next.lot);
            Money paid=due < next.remaining ? due : next.remaining;
            if (next.component == 0) next.lot.paid_fee+=paid;
            else next.lot.paid_principal+=paid;
            next.remaining-=paid;
            next=pair(next,next.account.id,CASH,next.component == 0 ? RECEIVABLE_FEE : RECEIVABLE_PRINCIPAL,paid);
            next=post(next,PAY_LOT);
            break;
        }
        case PAY_LOT:
            next.phase=PAY_LOTS;
            return edited(state,next,(Edit){.table=TABLE_LOT,.index=next.target,.value.lot=next.lot});
        case PAY_RECORD: {
            if (next.remaining) return terminal(CORE_ABORT,fail(next,command,RECONCILIATION));
            next.result.principal=command.amount;
            next.capacity_result=next.result;
            next.capacity_result.decision=ERROR; next.capacity_result.reason=CAPACITY;
            Payment payment={.amount=command.amount};
            strcpy(payment.id,command.request_id);
            strcpy(payment.account_id,next.account.id);
            next.phase=FINISH;
            return edited(state,next,(Edit){.table=TABLE_PAYMENT,.append=true,.value.payment=payment});
        }
        case ASSESS_START: {
            const Invoice *invoice=core_find_invoice(state,command.invoice_id);
            if (!invoice) return terminal(CORE_ABORT,fail(next,command,NOT_FOUND));
            strcpy(next.result.invoice_id,invoice->id);
            if (invoice->late_fee_assessed || state.now/1440 <= invoice->due_day || !core_invoice_outstanding(state,invoice)) {
                next.result.decision=NOOP; next.phase=FINISH; break;
            }
            next.invoice=*invoice;
            next.target=(size_t)(invoice-state.invoices);
            next.capacity_result=next.result;
            next.capacity_result.decision=ERROR; next.capacity_result.reason=CAPACITY;
            next=pair(next,invoice->account_id,RECEIVABLE_LATE,LATE_REVENUE,state.config.late_fee);
            next=post(next,ASSESS_INVOICE);
            break;
        }
        case ASSESS_INVOICE:
            next.invoice.late=state.config.late_fee;
            next.invoice.late_fee_assessed=true;
            next.result.fee=state.config.late_fee;
            next.phase=FINISH;
            return edited(state,next,(Edit){.table=TABLE_INVOICE,.index=next.target,.value.invoice=next.invoice});
        case LEDGER: {
            if (next.entry_cursor == next.entries_len) {
                next.entries_len=0;
                next.phase=next.after_ledger;
                break;
            }
            Entry entry=next.entries[next.entry_cursor++];
            return edited(state,next,(Edit){.table=TABLE_LEDGER,.append=true,.value.entry=entry});
        }
        case FINISH:
            next.capacity_result=core_capacity_failure(command);
            next.phase=CACHE;
            break;
        case CACHE:
            next.phase=REFRESH;
            next.cursor=0;
            if (core_cacheable(&next.result)) {
                Cached cached={.command=command,.result=next.result};
                return edited(state,next,(Edit){.table=TABLE_KEY,.append=true,.value.key=cached});
            }
            break;
        case REFRESH:
            while (next.cursor < state.accounts_len) {
                size_t index=next.cursor++;
                Account account=state.accounts[index];
                Projection projection=core_project_account(state,&account);
                if (!core_money_valid(projection.debt) || !core_money_valid(projection.held) ||
                    !core_money_valid(projection.cash_refund_total))
                    return terminal(CORE_ABORT,fail(next,command,NUMERIC_RANGE));
                account.cached_debt=projection.debt; account.cached_held=projection.held;
                return edited(state,next,(Edit){.table=TABLE_ACCOUNT,.index=index,.value.account=account});
            }
            return terminal(CORE_COMMIT,next);
        default:
            return terminal(CORE_ABORT,fail(next,command,INVALID_INPUT));
        }
    }
}
