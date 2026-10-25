#include "core.h"
#include <string.h>

const Account *core_find_account(StateView state, const char *id) {
    for (size_t i=0; i<state.accounts_len; ++i)
        if (!strcmp(state.accounts[i].id,id)) return &state.accounts[i];
    return NULL;
}
const Card *core_find_card(StateView state, const char *id) {
    for (size_t i=0; i<state.cards_len; ++i)
        if (!strcmp(state.cards[i].id,id)) return &state.cards[i];
    return NULL;
}
const Merchant *core_find_merchant(StateView state, const char *id) {
    for (size_t i=0; i<state.merchants_len; ++i)
        if (!strcmp(state.merchants[i].id,id)) return &state.merchants[i];
    return NULL;
}
const Authorization *core_find_auth(StateView state, const char *id) {
    for (size_t i=0; i<state.auths_len; ++i)
        if (!strcmp(state.auths[i].id,id)) return &state.auths[i];
    return NULL;
}
const Capture *core_find_capture(StateView state, const char *id) {
    for (size_t i=0; i<state.captures_len; ++i)
        if (!strcmp(state.captures[i].id,id)) return &state.captures[i];
    return NULL;
}
const Invoice *core_find_invoice(StateView state, const char *id) {
    for (size_t i=0; i<state.invoices_len; ++i)
        if (!strcmp(state.invoices[i].id,id)) return &state.invoices[i];
    return NULL;
}

Account core_seed_account_default(StateView state) {
    return (Account){.active=true,.tier=REGULAR,.credit_limit=1000000,.daily_limit=300000,
        .per_operation_limit=200000,.base_score=10,.next_cycle_to_close=state.now/1440/state.config.cycle_days};
}
Card core_seed_card_default(void) {
    return (Card){.active=true,.expiry_day=500,.allow_international=true,.allow_contactless=true};
}
Merchant core_seed_merchant_default(void) {
    Merchant merchant={.active=true,.category=NORMAL};
    memcpy(merchant.country,"BR",3);
    return merchant;
}
bool core_seed_account_valid(StateView state, Account account) {
    return core_identifier_valid(account.id) && !core_find_account(state,account.id) &&
        (account.tier == REGULAR || account.tier == PREMIUM) &&
        core_money_valid(account.credit_limit) && core_money_valid(account.daily_limit) &&
        core_money_valid(account.per_operation_limit) && account.base_score >= 0 && account.base_score <= 100 &&
        state.accounts_len < state.config.accounts_capacity;
}
bool core_seed_card_valid(StateView state, Card card) {
    return core_identifier_valid(card.id) && core_identifier_valid(card.account_id) &&
        core_find_account(state,card.account_id) && !core_find_card(state,card.id) &&
        card.expiry_day >= 0 && state.cards_len < state.config.cards_capacity;
}
bool core_seed_merchant_valid(StateView state, Merchant merchant) {
    return core_identifier_valid(merchant.id) && !core_find_merchant(state,merchant.id) &&
        (merchant.category == NORMAL || merchant.category == RESTRICTED) &&
        merchant.country[0] >= 'A' && merchant.country[0] <= 'Z' &&
        merchant.country[1] >= 'A' && merchant.country[1] <= 'Z' && !merchant.country[2] &&
        state.merchants_len < state.config.merchants_capacity;
}
int64_t core_recent_count(StateView state, const char *account_id) {
    int64_t count=0;
    for (size_t i=0; i<state.history_len; ++i) {
        const History *history=&state.history[i];
        if (!strcmp(history->account_id,account_id) &&
            (history->decision == APPROVED || history->decision == REVIEW) &&
            history->minute >= state.now-60 && history->minute <= state.now) ++count;
    }
    return count;
}
DailyTotals core_daily_totals(StateView state, const char *account_id, int64_t day) {
    DailyTotals totals={0};
    for (size_t i=0; i<state.history_len; ++i) {
        const History *history=&state.history[i];
        if (strcmp(history->account_id,account_id) || history->minute/1440 != day) continue;
        if (history->decision == APPROVED) { totals.gross+=history->principal; ++totals.count; }
        else if (history->decision == REVIEW) ++totals.count;
    }
    return totals;
}
HistoryRecord core_history_record(StateView state, const char *account_id, Decision decision, Money principal) {
    HistoryRecord record={0};
    if (decision == DECLINED) { record.valid=true; return record; }
    if ((decision != APPROVED && decision != REVIEW) || !core_money_valid(principal) ||
        !core_find_account(state,account_id)) return record;
    record.valid=true;
    record.store=true;
    record.value=(History){.minute=state.now,.decision=decision,.principal=principal};
    strcpy(record.value.account_id,account_id);
    return record;
}
int64_t core_reward_gross(StateView state, const char *account_id, int64_t cycle) {
    int64_t gross=0;
    for (size_t i=0; i<state.rewards_len; ++i)
        if (!strcmp(state.rewards[i].account_id,account_id) && state.rewards[i].cycle == cycle)
            gross+=state.rewards[i].granted;
    return gross;
}
Money core_lot_principal(const Lot *lot) { return lot->principal-lot->paid_principal-lot->cancelled_principal; }
Money core_lot_fee(const Lot *lot) { return lot->fee-lot->paid_fee-lot->cancelled_fee; }
Money core_invoice_outstanding(StateView state, const Invoice *invoice) {
    Money total=invoice->late-invoice->paid_late;
    for (size_t i=0; i<state.lots_len; ++i)
        if (!strcmp(state.lots[i].invoice_id,invoice->id))
            total+=core_lot_principal(&state.lots[i])+core_lot_fee(&state.lots[i]);
    return total;
}
Projection core_project_account(StateView state, const Account *account) {
    Projection projection={.cash_refund_total=account->cash_refund_total,
                           .next_cycle_to_close=account->next_cycle_to_close};
    for (size_t i=0; i<state.auths_len; ++i)
        if (!strcmp(state.auths[i].account_id,account->id)) projection.held+=core_authorization_hold(&state.auths[i]);
    for (size_t i=0; i<state.lots_len; ++i) {
        const Lot *lot=&state.lots[i];
        if (strcmp(lot->account_id,account->id)) continue;
        Money total=core_lot_principal(lot)+core_lot_fee(lot);
        projection.debt+=total;
        if (lot->invoice_id[0]) projection.invoiced_outstanding+=total;
        else projection.future_outstanding+=total;
    }
    for (size_t i=0; i<state.invoices_len; ++i) {
        const Invoice *invoice=&state.invoices[i];
        if (strcmp(invoice->account_id,account->id)) continue;
        Money late=invoice->late-invoice->paid_late;
        projection.debt+=late;
        projection.invoiced_outstanding+=late;
    }
    for (size_t i=0; i<state.captures_len; ++i)
        if (!strcmp(state.captures[i].account_id,account->id))
            projection.points+=state.captures[i].granted-state.captures[i].reversed;
    DailyTotals daily=core_daily_totals(state,account->id,state.now/1440);
    projection.daily_gross_principal=daily.gross;
    projection.daily_count=daily.count;
    projection.available_credit=core_available_credit(account->credit_limit,projection.debt,projection.held);
    return projection;
}
bool core_ledger_balanced(StateView state) {
    const char *previous=NULL;
    for (;;) {
        const char *next=NULL;
        for (size_t i=0; i<state.ledger_len; ++i) {
            const Entry *entry=&state.ledger[i];
            if (entry->amount < -MONEY_MAX || entry->amount > MONEY_MAX) return false;
            if (previous && strcmp(entry->event_id,previous) <= 0) continue;
            if (!next || strcmp(entry->event_id,next) < 0) next=entry->event_id;
        }
        if (!next) return true;
        Money sum=0;
        for (size_t i=0; i<state.ledger_len; ++i)
            if (!strcmp(state.ledger[i].event_id,next)) sum+=state.ledger[i].amount;
        if (sum) return false;
        previous=next;
    }
}
Reconciliation core_reconcile(StateView state, const char *account_id) {
    Reconciliation result={.valid=core_ledger_balanced(state)};
    for (size_t i=0; i<state.accounts_len; ++i) {
        const Account *account=&state.accounts[i];
        if (account_id[0] && strcmp(account->id,account_id)) continue;
        Projection projection=core_project_account(state,account);
        Money debt=account->cached_debt-projection.debt, held=account->cached_held-projection.held;
        Money ledger_debt=0, ledger_held=0;
        result.debt_difference+=debt;
        result.held_difference+=held;
        if (debt || held) result.valid=false;
        for (size_t j=0; j<state.ledger_len; ++j) {
            const Entry *entry=&state.ledger[j];
            if (strcmp(entry->account_id,account->id)) continue;
            if (entry->account == HOLD_ASSET) ledger_held+=entry->amount;
            if (entry->account == RECEIVABLE_PRINCIPAL || entry->account == RECEIVABLE_FEE ||
                entry->account == RECEIVABLE_LATE) ledger_debt+=entry->amount;
        }
        if (ledger_debt != projection.debt || ledger_held != projection.held || projection.points < 0) result.valid=false;
    }
    for (size_t i=0; i<state.lots_len; ++i) {
        const Lot *lot=&state.lots[i];
        if (account_id[0] && strcmp(lot->account_id,account_id)) continue;
        if (!core_money_valid(core_lot_principal(lot)) || !core_money_valid(core_lot_fee(lot)) ||
            !core_find_account(state,lot->account_id)) result.valid=false;
    }
    return result;
}
