#include "aurum.h"
#include <string.h>

Result result_default(const Command *c) {
    Result r={.op=c ? c->op : INVALID_OP,.decision=OK,.reason=NONE};
    if (c && identifier_valid(c->request_id)) strcpy(r.request_id,c->request_id);
    return r;
}
bool command_same_key(const Command *a, const Command *b) {
    return a->op==b->op && strcmp(a->request_id,b->request_id)==0;
}
bool cacheable(const Result *r) {
    return r->decision!=ERROR && r->op!=QUOTE && r->op!=GET_ACCOUNT && r->op!=GET_AUTH && r->op!=GET_INVOICE && r->op!=RECONCILE && r->op!=TICK;
}
Result execute(Engine *e, const Command *c) {
    Result r=result_default(c);
    if (!command_valid(c)) { r.decision=ERROR; r.reason=INVALID_INPUT; return r; }
    if (c->op==QUOTE) {
        Price price=quote(&e->config,c);
        r.principal=price.principal; r.fee=price.fee; r.has_quote=true; r.reason=price.reason;
        if (r.reason!=NONE) r.decision=r.reason==NUMERIC_RANGE ? ERROR : DECLINED;
        return r;
    }
    if (c->op==GET_ACCOUNT || c->op==GET_AUTH || c->op==GET_INVOICE || c->op==RECONCILE) {
        bool found=true;
        if (c->op==GET_ACCOUNT) found=find_account(e,c->account_id)!=NULL;
        if (c->op==GET_AUTH) found=find_auth(e,c->auth_id)!=NULL;
        if (c->op==GET_INVOICE) found=find_invoice(e,c->invoice_id)!=NULL;
        if (c->op==RECONCILE && c->account_id[0]) found=find_account(e,c->account_id)!=NULL;
        if (!found) { r.decision=ERROR; r.reason=NOT_FOUND; return r; }
        if (c->op==RECONCILE) {
            Money d,h;
            if (!reconcile(e,c->account_id,&d,&h)) { r.decision=ERROR; r.reason=RECONCILIATION; }
        }
        return r;
    }
    if (c->op!=TICK) for (size_t i=0; i<e->keys_len; ++i) {
        const Cached *key=&e->keys[i];
        if (!command_same_key(&key->command,c)) continue;
        if (command_equal(&key->command,c)) return key->result;
        r.decision=ERROR; r.reason=IDEMPOTENCY_CONFLICT; return r;
    }
    Engine prepared;
    if (!engine_clone(e,&prepared)) { r.decision=ERROR; r.reason=CAPACITY; return r; }
    switch (c->op) {
        case AUTHORIZE: r=authorize_apply(&prepared,c); break;
        case CAPTURE: r=capture_apply(&prepared,c); break;
        case CANCEL: r=cancel_apply(&prepared,c); break;
        case REFUND: r=refund_apply(&prepared,c); break;
        case CLOSE: r=close_apply(&prepared,c); break;
        case PAY: r=pay_apply(&prepared,c); break;
        case ASSESS_LATE_FEE: r=assess_apply(&prepared,c); break;
        case TICK: r=tick_apply(&prepared,c); break;
        default: r.decision=ERROR; r.reason=INVALID_INPUT; break;
    }
    if (cacheable(&r)) {
        Cached cached={.command=*c,.result=r};
        if (!array_append((void **)&prepared.keys,&prepared.keys_len,&prepared.keys_alloc,prepared.config.keys_capacity,sizeof cached,&cached)) { r=result_default(c); r.decision=ERROR; r.reason=CAPACITY; }
    }
    if (r.decision!=ERROR && !refresh_projections(&prepared)) { r=result_default(c); r.decision=ERROR; r.reason=NUMERIC_RANGE; }
    if (r.decision==ERROR) engine_free(&prepared);
    else { engine_free(e); *e=prepared; }
    return r;
}
