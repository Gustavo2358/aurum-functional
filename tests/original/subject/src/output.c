#include "aurum.h"
#include <inttypes.h>
#include <string.h>

const char *operation_name(Operation op) {
 static const char *const names[] = { "QUOTE", "AUTHORIZE", "CAPTURE", "CANCEL", "REFUND", "CLOSE", "PAY",
  "ASSESS_LATE_FEE", "TICK", "GET_ACCOUNT", "GET_AUTH", "GET_INVOICE", "RECONCILE", "INVALID" };
 return op >= QUOTE && op <= INVALID_OP ? names[op] : "INVALID";
}
const char *decision_name(Decision d) {
 static const char *const names[] = { "APPROVED", "REVIEW", "DECLINED", "ERROR", "OK", "NOOP" };
 return d >= APPROVED && d <= NOOP ? names[d] : "ERROR";
}
const char *reason_name(Reason r) {
 static const char *const names[] = {
  "NONE", "INVALID_INPUT", "NUMERIC_RANGE", "NOT_FOUND", "OWNERSHIP", "ACCOUNT_INACTIVE",
  "CARD_BLOCKED", "CARD_EXPIRED", "MERCHANT_BLOCKED", "CATEGORY_RESTRICTED", "INTERNATIONAL_DISABLED",
  "CONTACTLESS_DISABLED", "PIN_REQUIRED", "PAST_DUE", "UNSUPPORTED_CURRENCY", "INSTALLMENTS",
  "OPERATION_LIMIT", "CREDIT_LIMIT", "DAILY_AMOUNT", "DAILY_COUNT", "RISK", "CAPACITY",
  "IDEMPOTENCY_CONFLICT", "AUTH_STATE", "CAPTURE_AMOUNT", "REFUND_AMOUNT", "CYCLE_ORDER",
  "CYCLE_NOT_ENDED", "OVERPAYMENT", "INVALID_TIME", "INVARIANT_VIOLATION"
 };
 return r >= NONE && r <= RECONCILIATION ? names[r] : "INVALID_INPUT";
}
const char *state_name(AuthState s) {
 static const char *const names[] = { "ACTIVE", "PARTIAL", "CAPTURED", "CANCELLED", "EXPIRED" };
 return s >= ACTIVE && s <= EXPIRED ? names[s] : "INVALID";
}
const char *ledger_name(LedgerAccount a) {
 static const char *const names[] = { "HOLD_ASSET", "HOLD_OFFSET", "RECEIVABLE_PRINCIPAL", "MERCHANT_CLEARING",
  "RECEIVABLE_FEE", "FEE_REVENUE", "RECEIVABLE_LATE", "LATE_REVENUE", "CASH", "OPENING_OFFSET" };
 return a >= HOLD_ASSET && a <= OPENING_OFFSET ? names[a] : "INVALID";
}
/* JSON escaping stays correct if a future public identifier grammar broadens. */
static void json_string(FILE *f, const char *s) {
 fputc('"',f);
 for (; *s; ++s) {
  unsigned char c = (unsigned char)*s;
  if (c == '"' || c == '\\') { fputc('\\',f); fputc(c,f); }
  else if (c < 32 || c > 126) fprintf(f,"\\u%04x",(unsigned)c);
  else fputc(c,f);
 }
 fputc('"',f);
}
static void string_field(FILE *f, const char *key, const char *value) {
 fputc(',',f); json_string(f,key); fputc(':',f); json_string(f,value);
}
static void number_field(FILE *f, const char *key, int64_t value) {
 fputc(',',f); json_string(f,key); fprintf(f,":%" PRId64,value);
}
static void bool_field(FILE *f, const char *key, bool value) {
 fputc(',',f); json_string(f,key); fputs(value ? ":true" : ":false",f);
}
/* Ordered traversal uses bounded temporary state and leaves the engine untouched. */
static void write_auth_ids(FILE *f, const Engine *e, const char *account_id) {
 const char *previous = NULL;
 bool first = true;
 fputs(",\"auth_ids\":[",f);
 for (;;) {
  const char *next = NULL;
  for (size_t i = 0; i < e->auths_len; ++i) {
   const Authorization *a = &e->auths[i];
   if (strcmp(a->account_id,account_id) || (previous && strcmp(a->id,previous) <= 0)) continue;
   if (!next || strcmp(a->id,next) < 0) next = a->id;
  }
  if (!next) break;
  if (!first) fputc(',',f);
  json_string(f,next); first = false; previous = next;
 }
 fputc(']',f);
}
static void write_account(FILE *f, const Engine *e, const Account *a) {
 Projection p = project_account(e,a);
 string_field(f,"id",a->id);
 number_field(f,"held",p.held);
 number_field(f,"debt",p.debt);
 number_field(f,"available_credit",p.available_credit);
 number_field(f,"invoiced_outstanding",p.invoiced_outstanding);
 number_field(f,"future_outstanding",p.future_outstanding);
 number_field(f,"points",p.points);
 number_field(f,"cash_refund_total",p.cash_refund_total);
 number_field(f,"daily_gross_principal",p.daily_gross_principal);
 number_field(f,"daily_count",p.daily_count);
 number_field(f,"next_cycle_to_close",p.next_cycle_to_close);
 write_auth_ids(f,e,a->id);
}
static void write_auth(FILE *f, const Authorization *a) {
 string_field(f,"id",a->id);
 string_field(f,"state",state_name(a->state));
 number_field(f,"principal",a->principal);
 number_field(f,"fee",a->fee);
 number_field(f,"captured_principal",a->captured_principal);
 number_field(f,"captured_fee",a->captured_fee);
 number_field(f,"remaining_hold",authorization_hold(a));
 number_field(f,"approved_at",a->approved_at);
 number_field(f,"expires_at",a->expires_at);
 number_field(f,"installments",a->installments);
}
static void write_invoice(FILE *f, const Engine *e, const Invoice *in) {
 string_field(f,"id",in->id);
 string_field(f,"account_id",in->account_id);
 number_field(f,"cycle",in->cycle);
 number_field(f,"issued_principal",in->issued_principal);
 number_field(f,"issued_fee",in->issued_fee);
 number_field(f,"issued_total",in->issued_total);
 number_field(f,"minimum_due",in->minimum_due);
 number_field(f,"due_day",in->due_day);
 number_field(f,"outstanding",invoice_outstanding(e,in));
 bool_field(f,"late_fee_assessed",in->late_fee_assessed);
}
/* Mutation responses contain only saved Result data: replay is byte-for-byte stable. */
void write_result(FILE *f, const Engine *e, const Command *c, const Result *r) {
 fputs("{\"op\":",f); json_string(f,operation_name(r->op));
 if (r->request_id[0]) string_field(f,"request_id",r->request_id);
 string_field(f,"decision",decision_name(r->decision));
 string_field(f,"reason",reason_name(r->reason));
 if (r->decision != ERROR) {
  switch (r->op) {
   case QUOTE:
    number_field(f,"principal",r->principal); number_field(f,"fee",r->fee); break;
   case AUTHORIZE:
    number_field(f,"principal",r->principal); number_field(f,"fee",r->fee); number_field(f,"reserved",r->reserved);
    if (r->auth_id[0]) { string_field(f,"auth_id",r->auth_id); number_field(f,"expires_at",r->expires_at); }
    break;
   case CAPTURE:
    if (r->auth_id[0]) string_field(f,"auth_id",r->auth_id);
    if (r->capture_id[0]) string_field(f,"capture_id",r->capture_id);
    number_field(f,"principal",r->principal); number_field(f,"fee",r->fee); number_field(f,"points",r->points); break;
   case CANCEL: case TICK: number_field(f,"released",r->released); break;
   case REFUND:
    if (r->capture_id[0]) string_field(f,"capture_id",r->capture_id);
    number_field(f,"principal",r->principal); number_field(f,"fee",r->fee);
    number_field(f,"cash_refund",r->cash_refund); number_field(f,"points_reversed",r->points_reversed); break;
   case CLOSE:
    if (r->invoice_id[0]) string_field(f,"invoice_id",r->invoice_id);
    number_field(f,"issued_total",r->issued_total); number_field(f,"minimum_due",r->minimum_due); break;
   case PAY: number_field(f,"paid",r->principal); break;
   case ASSESS_LATE_FEE: number_field(f,"fee",r->fee); break;
   case GET_ACCOUNT: { const Account *a = find_account(e,c->account_id); if (a) write_account(f,e,a); break; }
   case GET_AUTH: { const Authorization *a = find_auth(e,c->auth_id); if (a) write_auth(f,a); break; }
   case GET_INVOICE: { const Invoice *in = find_invoice(e,c->invoice_id); if (in) write_invoice(f,e,in); break; }
   case RECONCILE: case INVALID_OP: break;
  }
 }
 if (r->op == RECONCILE && (r->decision == OK || r->reason == RECONCILIATION)) {
  Money debt_difference = 0, held_difference = 0;
  bool valid = reconcile(e,c->account_id,&debt_difference,&held_difference);
  string_field(f,"status",valid ? "OK" : "INVARIANT_VIOLATION");
  number_field(f,"debt_difference",debt_difference); number_field(f,"held_difference",held_difference);
 }
 fputs("}\n",f);
}
