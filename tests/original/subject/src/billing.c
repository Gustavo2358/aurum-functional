#include "aurum.h"
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

int64_t invoice_due_day(const Config *c, int64_t cycle) {
    if (cycle < 0 || cycle > (INT64_MAX-c->due_grace_days)/c->cycle_days-1) return -1;
    return (cycle+1)*c->cycle_days+c->due_grace_days;
}
Money invoice_minimum(const Config *c, Money total) {
    Money minimum;
    if (!ceil_rate(total,c->minimum_due_bps,&minimum)) return -1;
    if (minimum < c->minimum_due_floor) minimum=c->minimum_due_floor;
    return minimum < total ? minimum : total;
}
Reason close_admission(const Config *c, int64_t day, int64_t next, int64_t cycle) {
    if (cycle < 0 || cycle > next) return CYCLE_ORDER;
    if (cycle < next) return NONE;
    if (cycle > INT64_MAX/c->cycle_days-1 || day < (cycle+1)*c->cycle_days) return CYCLE_OPEN;
    return NONE;
}
Reason payment_admission(Money amount, Money outstanding) {
    if (!money_valid(amount) || amount == 0) return INVALID_INPUT;
    return amount <= outstanding ? NONE : OVERPAYMENT;
}
Result close_apply(Engine *e, const Command *c) {
    Result r=result_default(c);
    Account *a=find_account(e,c->account_id);
    if (!a) { r.decision=ERROR; r.reason=NOT_FOUND; return r; }
    for (size_t i=0; i<e->invoices_len; ++i) if (strcmp(e->invoices[i].account_id,a->id)==0 && e->invoices[i].cycle==c->cycle) {
        r.decision=NOOP; strcpy(r.invoice_id,e->invoices[i].id); return r;
    }
    if (c->cycle < a->next_cycle_to_close) { r.decision=ERROR; r.reason=CYCLE_ORDER; return r; }
    r.reason=close_admission(&e->config,e->now/1440,a->next_cycle_to_close,c->cycle);
    if (r.reason!=NONE) { r.decision=ERROR; return r; }
    Invoice in={.cycle=c->cycle,.due_day=invoice_due_day(&e->config,c->cycle)};
    strcpy(in.account_id,a->id);
    snprintf(in.id,sizeof in.id,"I%zu",e->invoices_len+1);
    for (size_t i=0; i<e->lots_len; ++i) {
        const Lot *l=&e->lots[i];
        if (strcmp(l->account_id,a->id)!=0 || l->cycle!=c->cycle) continue;
        if (!money_add(in.issued_principal,l->principal-l->cancelled_principal,&in.issued_principal) || !money_add(in.issued_fee,l->fee-l->cancelled_fee,&in.issued_fee)) { r.decision=ERROR; r.reason=NUMERIC_RANGE; return r; }
    }
    if (!money_add(in.issued_principal,in.issued_fee,&in.issued_total)) { r.decision=ERROR; r.reason=NUMERIC_RANGE; return r; }
    in.minimum_due=invoice_minimum(&e->config,in.issued_total);
    if (!array_append((void **)&e->invoices,&e->invoices_len,&e->invoices_alloc,e->config.invoices_capacity,sizeof in,&in)) { r.decision=ERROR; r.reason=CAPACITY; return r; }
    for (size_t i=0; i<e->lots_len; ++i) if (strcmp(e->lots[i].account_id,a->id)==0 && e->lots[i].cycle==c->cycle) strcpy(e->lots[i].invoice_id,in.id);
    ++a->next_cycle_to_close;
    strcpy(r.invoice_id,in.id); r.principal=in.issued_principal; r.fee=in.issued_fee;
    r.issued_total=in.issued_total; r.minimum_due=in.minimum_due;
    return r;
}
static int invoice_order(const void *left, const void *right) {
    const Invoice *const *a=left,*const *b=right;
    if ((*a)->due_day != (*b)->due_day) return (*a)->due_day < (*b)->due_day ? -1 : 1;
    if ((*a)->cycle != (*b)->cycle) return (*a)->cycle < (*b)->cycle ? -1 : 1;
    return strcmp((*a)->id,(*b)->id);
}
static int lot_order(const void *left, const void *right) {
    const Lot *const *a=left,*const *b=right;
    if ((*a)->cycle != (*b)->cycle) return (*a)->cycle < (*b)->cycle ? -1 : 1;
    int cmp=strcmp((*a)->capture_id,(*b)->capture_id);
    if (cmp) return cmp;
    return ((*a)->index > (*b)->index)-((*a)->index < (*b)->index);
}
static bool allocate_late(Engine *e, const char *event, const char *account_id, Money *remaining) {
    Invoice **order=malloc((e->invoices_len+1)*sizeof *order);
    if (!order) return false;
    size_t count=0;
    for (size_t i=0; i<e->invoices_len; ++i) if (strcmp(e->invoices[i].account_id,account_id)==0) order[count++]=&e->invoices[i];
    qsort(order,count,sizeof *order,invoice_order);
    bool ok=true;
    for (size_t i=0; i<count; ++i) {
        if (*remaining==0) break;
        Money due=order[i]->late-order[i]->paid_late;
        Money paid=due < *remaining ? due : *remaining;
        if (!ledger_pair(e,event,account_id,CASH,RECEIVABLE_LATE,paid)) { ok=false; break; }
        order[i]->paid_late+=paid; *remaining-=paid;
    }
    free(order);
    return ok;
}
static bool allocate_lots(Engine *e, const char *event, const char *account_id, Money *remaining) {
    Lot **order=malloc((e->lots_len+1)*sizeof *order);
    if (!order) return false;
    size_t count=0;
    for (size_t i=0; i<e->lots_len; ++i) if (strcmp(e->lots[i].account_id,account_id)==0 && e->lots[i].invoice_id[0]) order[count++]=&e->lots[i];
    qsort(order,count,sizeof *order,lot_order);
    bool ok=true;
    for (int component=0; component<2 && ok; ++component) {
        for (size_t i=0; i<count; ++i) {
            if (*remaining==0) break;
            Lot *l=order[i];
            Money due=component==0 ? lot_fee(l) : lot_principal(l);
            Money paid=due < *remaining ? due : *remaining;
            LedgerAccount receivable=component==0 ? RECEIVABLE_FEE : RECEIVABLE_PRINCIPAL;
            if (!ledger_pair(e,event,account_id,CASH,receivable,paid)) { ok=false; break; }
            if (component==0) l->paid_fee+=paid;
            else l->paid_principal+=paid;
            *remaining-=paid;
        }
    }
    free(order);
    return ok;
}
Result pay_apply(Engine *e, const Command *c) {
    Result r=result_default(c);
    Account *a=find_account(e,c->account_id);
    if (!a) { r.decision=ERROR; r.reason=NOT_FOUND; return r; }
    Projection p=project_account(e,a);
    r.reason=payment_admission(c->amount,p.invoiced_outstanding);
    if (r.reason!=NONE) { r.decision=r.reason==INVALID_INPUT ? ERROR : DECLINED; return r; }
    Money remaining=c->amount;
    char event[128]; snprintf(event,sizeof event,"PAY:%s",c->request_id);
    if (!allocate_late(e,event,a->id,&remaining) || !allocate_lots(e,event,a->id,&remaining)) { r.decision=ERROR; r.reason=CAPACITY; return r; }
    if (remaining!=0) { r.decision=ERROR; r.reason=RECONCILIATION; return r; }
    r.principal=c->amount;
    Payment payment={.amount=c->amount}; strcpy(payment.id,c->request_id); strcpy(payment.account_id,a->id);
    if (!array_append((void **)&e->payments,&e->payments_len,&e->payments_alloc,e->config.payments_capacity,sizeof payment,&payment)) { r.decision=ERROR; r.reason=CAPACITY; }
    return r;
}
Result assess_apply(Engine *e, const Command *c) {
    Result r=result_default(c);
    Invoice *in=find_invoice(e,c->invoice_id);
    if (!in) { r.decision=ERROR; r.reason=NOT_FOUND; return r; }
    strcpy(r.invoice_id,in->id);
    if (in->late_fee_assessed || e->now/1440 <= in->due_day || invoice_outstanding(e,in)==0) { r.decision=NOOP; return r; }
    char event[128]; snprintf(event,sizeof event,"ASSESS_LATE_FEE:%s",c->request_id);
    if (!ledger_pair(e,event,in->account_id,RECEIVABLE_LATE,LATE_REVENUE,e->config.late_fee)) { r.decision=ERROR; r.reason=CAPACITY; return r; }
    in->late=e->config.late_fee; in->late_fee_assessed=true; r.fee=e->config.late_fee;
    return r;
}
