#include "aurum.h"
#include <stdlib.h>
#include <string.h>

void engine_init(Engine *e) {
    memset(e,0,sizeof *e);
    config_default(&e->config);
}
bool array_append(void **data, size_t *len, size_t *alloc, size_t limit, size_t size, const void *value) {
    if (*len >= limit || size == 0 || *len == SIZE_MAX) return false;
    if (*len == *alloc) {
        size_t next=*alloc == 0 ? 8 : *alloc*2;
        if (next > limit) next=limit;
        if (next > SIZE_MAX/size) return false;
        void *p=realloc(*data,next*size);
        if (!p) return false;
        *data=p; *alloc=next;
    }
    memcpy((unsigned char *)*data+*len*size,value,size);
    ++*len;
    return true;
}
void engine_free(Engine *e) {
    free(e->accounts);
    free(e->cards);
    free(e->merchants);
    free(e->auths);
    free(e->captures);
    free(e->lots);
    free(e->invoices);
    free(e->history);
    free(e->ledger);
    free(e->keys);
    free(e->payments);
    free(e->refunds);
    free(e->rewards);
    memset(e,0,sizeof *e);
}
bool engine_clone(const Engine *src, Engine *dst) {
    engine_init(dst);
    dst->config=src->config; dst->now=src->now;
    if (src->accounts_len) {
        dst->accounts=malloc(src->accounts_len*sizeof *src->accounts);
        if (!dst->accounts) { engine_free(dst); return false; }
        memcpy(dst->accounts,src->accounts,src->accounts_len*sizeof *src->accounts);
        dst->accounts_len=src->accounts_len; dst->accounts_alloc=src->accounts_len;
    }
    if (src->cards_len) {
        dst->cards=malloc(src->cards_len*sizeof *src->cards);
        if (!dst->cards) { engine_free(dst); return false; }
        memcpy(dst->cards,src->cards,src->cards_len*sizeof *src->cards);
        dst->cards_len=src->cards_len; dst->cards_alloc=src->cards_len;
    }
    if (src->merchants_len) {
        dst->merchants=malloc(src->merchants_len*sizeof *src->merchants);
        if (!dst->merchants) { engine_free(dst); return false; }
        memcpy(dst->merchants,src->merchants,src->merchants_len*sizeof *src->merchants);
        dst->merchants_len=src->merchants_len; dst->merchants_alloc=src->merchants_len;
    }
    if (src->auths_len) {
        dst->auths=malloc(src->auths_len*sizeof *src->auths);
        if (!dst->auths) { engine_free(dst); return false; }
        memcpy(dst->auths,src->auths,src->auths_len*sizeof *src->auths);
        dst->auths_len=src->auths_len; dst->auths_alloc=src->auths_len;
    }
    if (src->captures_len) {
        dst->captures=malloc(src->captures_len*sizeof *src->captures);
        if (!dst->captures) { engine_free(dst); return false; }
        memcpy(dst->captures,src->captures,src->captures_len*sizeof *src->captures);
        dst->captures_len=src->captures_len; dst->captures_alloc=src->captures_len;
    }
    if (src->lots_len) {
        dst->lots=malloc(src->lots_len*sizeof *src->lots);
        if (!dst->lots) { engine_free(dst); return false; }
        memcpy(dst->lots,src->lots,src->lots_len*sizeof *src->lots);
        dst->lots_len=src->lots_len; dst->lots_alloc=src->lots_len;
    }
    if (src->invoices_len) {
        dst->invoices=malloc(src->invoices_len*sizeof *src->invoices);
        if (!dst->invoices) { engine_free(dst); return false; }
        memcpy(dst->invoices,src->invoices,src->invoices_len*sizeof *src->invoices);
        dst->invoices_len=src->invoices_len; dst->invoices_alloc=src->invoices_len;
    }
    if (src->history_len) {
        dst->history=malloc(src->history_len*sizeof *src->history);
        if (!dst->history) { engine_free(dst); return false; }
        memcpy(dst->history,src->history,src->history_len*sizeof *src->history);
        dst->history_len=src->history_len; dst->history_alloc=src->history_len;
    }
    if (src->ledger_len) {
        dst->ledger=malloc(src->ledger_len*sizeof *src->ledger);
        if (!dst->ledger) { engine_free(dst); return false; }
        memcpy(dst->ledger,src->ledger,src->ledger_len*sizeof *src->ledger);
        dst->ledger_len=src->ledger_len; dst->ledger_alloc=src->ledger_len;
    }
    if (src->keys_len) {
        dst->keys=malloc(src->keys_len*sizeof *src->keys);
        if (!dst->keys) { engine_free(dst); return false; }
        memcpy(dst->keys,src->keys,src->keys_len*sizeof *src->keys);
        dst->keys_len=src->keys_len; dst->keys_alloc=src->keys_len;
    }
    if (src->payments_len) {
        dst->payments=malloc(src->payments_len*sizeof *src->payments);
        if (!dst->payments) { engine_free(dst); return false; }
        memcpy(dst->payments,src->payments,src->payments_len*sizeof *src->payments);
        dst->payments_len=src->payments_len; dst->payments_alloc=src->payments_len;
    }
    if (src->refunds_len) {
        dst->refunds=malloc(src->refunds_len*sizeof *src->refunds);
        if (!dst->refunds) { engine_free(dst); return false; }
        memcpy(dst->refunds,src->refunds,src->refunds_len*sizeof *src->refunds);
        dst->refunds_len=src->refunds_len; dst->refunds_alloc=src->refunds_len;
    }
    if (src->rewards_len) {
        dst->rewards=malloc(src->rewards_len*sizeof *src->rewards);
        if (!dst->rewards) { engine_free(dst); return false; }
        memcpy(dst->rewards,src->rewards,src->rewards_len*sizeof *src->rewards);
        dst->rewards_len=src->rewards_len; dst->rewards_alloc=src->rewards_len;
    }
    return true;
}
Account *find_account(const Engine *e, const char *id) {
    for (size_t i=0; i<e->accounts_len; ++i) if (strcmp(e->accounts[i].id,id)==0) return &e->accounts[i];
    return NULL;
}
Card *find_card(const Engine *e, const char *id) {
    for (size_t i=0; i<e->cards_len; ++i) if (strcmp(e->cards[i].id,id)==0) return &e->cards[i];
    return NULL;
}
Merchant *find_merchant(const Engine *e, const char *id) {
    for (size_t i=0; i<e->merchants_len; ++i) if (strcmp(e->merchants[i].id,id)==0) return &e->merchants[i];
    return NULL;
}
Authorization *find_auth(const Engine *e, const char *id) {
    for (size_t i=0; i<e->auths_len; ++i) if (strcmp(e->auths[i].id,id)==0) return &e->auths[i];
    return NULL;
}
Capture *find_capture(const Engine *e, const char *id) {
    for (size_t i=0; i<e->captures_len; ++i) if (strcmp(e->captures[i].id,id)==0) return &e->captures[i];
    return NULL;
}
Invoice *find_invoice(const Engine *e, const char *id) {
    for (size_t i=0; i<e->invoices_len; ++i) if (strcmp(e->invoices[i].id,id)==0) return &e->invoices[i];
    return NULL;
}
Money lot_principal(const Lot *l) { return l->principal-l->paid_principal-l->cancelled_principal; }
Money lot_fee(const Lot *l) { return l->fee-l->paid_fee-l->cancelled_fee; }
Money invoice_outstanding(const Engine *e, const Invoice *in) {
    Money total=in->late-in->paid_late;
    for (size_t i=0; i<e->lots_len; ++i) if (strcmp(e->lots[i].invoice_id,in->id)==0) total+=lot_principal(&e->lots[i])+lot_fee(&e->lots[i]);
    return total;
}
Projection project_account(const Engine *e, const Account *a) {
    Projection p={.cash_refund_total=a->cash_refund_total,.next_cycle_to_close=a->next_cycle_to_close};
    for (size_t i=0; i<e->auths_len; ++i) if (strcmp(e->auths[i].account_id,a->id)==0) p.held+=authorization_hold(&e->auths[i]);
    for (size_t i=0; i<e->lots_len; ++i) {
        const Lot *l=&e->lots[i];
        if (strcmp(l->account_id,a->id)!=0) continue;
        Money total=lot_principal(l)+lot_fee(l);
        p.debt+=total;
        if (l->invoice_id[0]) p.invoiced_outstanding+=total;
        else p.future_outstanding+=total;
    }
    for (size_t i=0; i<e->invoices_len; ++i) if (strcmp(e->invoices[i].account_id,a->id)==0) {
        Money late=e->invoices[i].late-e->invoices[i].paid_late;
        p.debt+=late; p.invoiced_outstanding+=late;
    }
    for (size_t i=0; i<e->captures_len; ++i) if (strcmp(e->captures[i].account_id,a->id)==0) p.points+=e->captures[i].granted-e->captures[i].reversed;
    daily_totals(e,a->id,e->now/1440,&p.daily_gross_principal,&p.daily_count);
    p.available_credit=available_credit(a->credit_limit,p.debt,p.held);
    return p;
}
bool refresh_projections(Engine *e) {
    for (size_t i=0; i<e->accounts_len; ++i) {
        Account *a=&e->accounts[i];
        Projection p=project_account(e,a);
        if (!money_valid(p.debt) || !money_valid(p.held) || !money_valid(p.cash_refund_total)) return false;
        a->cached_debt=p.debt; a->cached_held=p.held;
    }
    return true;
}
bool ledger_post(Engine *e, const char *event, const char *account_id, LedgerAccount account, Money amount) {
    if (amount==0) return true;
    if (amount < -MONEY_MAX || amount > MONEY_MAX || strlen(event)>=sizeof ((Entry *)0)->event_id || !identifier_valid(account_id)) return false;
    Entry entry={.account=account,.amount=amount};
    strcpy(entry.event_id,event); strcpy(entry.account_id,account_id);
    return array_append((void **)&e->ledger,&e->ledger_len,&e->ledger_alloc,e->config.ledger_capacity,sizeof entry,&entry);
}
bool ledger_pair(Engine *e, const char *event, const char *account_id, LedgerAccount debit, LedgerAccount credit, Money value) {
    if (!money_valid(value)) return false;
    return ledger_post(e,event,account_id,debit,value) && ledger_post(e,event,account_id,credit,-value);
}
static int compare_event(const void *left, const void *right) {
    const Entry *const *a=left, *const *b=right;
    return strcmp((*a)->event_id,(*b)->event_id);
}
bool ledger_balanced(const Engine *e) {
    if (e->ledger_len==0) return true;
    const Entry **order=malloc(e->ledger_len*sizeof *order);
    if (!order) return false;
    for (size_t i=0; i<e->ledger_len; ++i) order[i]=&e->ledger[i];
    qsort(order,e->ledger_len,sizeof *order,compare_event);
    Money sum=0; bool valid=true;
    for (size_t i=0; i<e->ledger_len; ++i) {
        if (order[i]->amount < -MONEY_MAX || order[i]->amount > MONEY_MAX) { valid=false; break; }
        sum+=order[i]->amount;
        if (i+1==e->ledger_len || strcmp(order[i]->event_id,order[i+1]->event_id)!=0) {
            if (sum!=0) { valid=false; break; }
            sum=0;
        }
    }
    free(order);
    return valid;
}
bool reconcile(const Engine *e, const char *account_id, Money *debt_difference, Money *held_difference) {
    *debt_difference=0; *held_difference=0;
    bool valid=ledger_balanced(e);
    for (size_t i=0; i<e->accounts_len; ++i) {
        const Account *a=&e->accounts[i];
        if (account_id[0] && strcmp(a->id,account_id)!=0) continue;
        Projection p=project_account(e,a);
        Money d=a->cached_debt-p.debt, h=a->cached_held-p.held, ld=0, lh=0;
        *debt_difference+=d; *held_difference+=h;
        if (d || h) valid=false;
        for (size_t j=0; j<e->ledger_len; ++j) {
            const Entry *l=&e->ledger[j];
            if (strcmp(l->account_id,a->id)!=0) continue;
            if (l->account==HOLD_ASSET) lh+=l->amount;
            if (l->account==RECEIVABLE_PRINCIPAL || l->account==RECEIVABLE_FEE || l->account==RECEIVABLE_LATE) ld+=l->amount;
        }
        if (ld!=p.debt || lh!=p.held || p.points<0) valid=false;
    }
    for (size_t i=0; i<e->lots_len; ++i) {
        const Lot *l=&e->lots[i];
        if (account_id[0] && strcmp(l->account_id,account_id)!=0) continue;
        if (!money_valid(lot_principal(l)) || !money_valid(lot_fee(l)) || !find_account(e,l->account_id)) valid=false;
    }
    return valid;
}
