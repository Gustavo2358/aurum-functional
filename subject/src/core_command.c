#include "core.h"
#include <string.h>

static bool ascii_letter(unsigned char c) {
 return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}
static bool bounded_id(const char *s, size_t size) {
 size_t n = 0;
 while (n < size && s[n]) {
  unsigned char c = (unsigned char)s[n];
  if (!ascii_letter(c) && !(c >= '0' && c <= '9') && c != '_' && c != '-') return false;
  ++n;
 }
 return n > 0 && n < size && n < ID_SIZE;
}
bool core_identifier_valid(const char *s) { return s && bounded_id(s, ID_SIZE); }
static bool upper_code(const char *s, size_t n) {
 for (size_t i = 0; i < n; ++i) if (s[i] < 'A' || s[i] > 'Z') return false;
 return s[n] == '\0';
}
Command core_command_default(Operation op) {
    Command command = {.op=op,.tier=REGULAR,.channel=POS,.card_present=true,
                       .pin_ok=true,.installments=1};
    memcpy(command.currency,"BRL",4);
    memcpy(command.country,"BR",3);
    return command;
}
Result core_result_default(const Command *command) {
    Result result = {.op=command ? command->op : INVALID_OP,.decision=OK,.reason=NONE};
    if (command && core_identifier_valid(command->request_id)) strcpy(result.request_id,command->request_id);
    return result;
}
bool core_command_same_key(const Command *a, const Command *b) {
    return a->op == b->op && strcmp(a->request_id,b->request_id) == 0;
}
bool core_cacheable(const Result *result) {
    return result->decision != ERROR && result->op != QUOTE && result->op != GET_ACCOUNT &&
        result->op != GET_AUTH && result->op != GET_INVOICE && result->op != RECONCILE && result->op != TICK;
}
static bool positive_money(Money v) { return v > 0 && core_money_valid(v); }
bool core_command_valid(const Command *c) {
 if (!c || c->op < QUOTE || c->op >= INVALID_OP) return false;
 if (c->op >= AUTHORIZE && c->op <= ASSESS_LATE_FEE && !bounded_id(c->request_id, sizeof(c->request_id))) return false;
 switch (c->op) {
  case QUOTE:
   return positive_money(c->amount) && upper_code(c->currency, 3) && upper_code(c->country, 2) &&
    c->installments >= 0 && (c->tier == REGULAR || c->tier == PREMIUM);
  case AUTHORIZE:
   return bounded_id(c->account_id, sizeof(c->account_id)) && bounded_id(c->card_id, sizeof(c->card_id)) &&
    bounded_id(c->merchant_id, sizeof(c->merchant_id)) && positive_money(c->amount) &&
    upper_code(c->currency, 3) && upper_code(c->country, 2) && c->installments >= 0 &&
    (c->channel == POS || c->channel == ECOM || c->channel == CONTACTLESS);
  case CAPTURE: return bounded_id(c->auth_id, sizeof(c->auth_id)) && positive_money(c->principal);
  case CANCEL: case GET_AUTH: return bounded_id(c->auth_id, sizeof(c->auth_id));
  case REFUND: return bounded_id(c->capture_id, sizeof(c->capture_id)) && positive_money(c->principal);
  case CLOSE: return bounded_id(c->account_id, sizeof(c->account_id)) && c->cycle >= 0;
  case PAY: return bounded_id(c->account_id, sizeof(c->account_id)) && positive_money(c->amount);
  case ASSESS_LATE_FEE: case GET_INVOICE: return bounded_id(c->invoice_id, sizeof(c->invoice_id));
  case TICK: return c->now >= 0 && c->now <= TIME_MAX;
  case GET_ACCOUNT: return bounded_id(c->account_id, sizeof(c->account_id));
  case RECONCILE: return !c->account_id[0] || bounded_id(c->account_id, sizeof(c->account_id));
  case INVALID_OP: return false;
 }
 return false;
}
/* Equality is semantic: padding, unused fields and omitted defaults never matter. */
bool core_command_equal(const Command *a, const Command *b) {
 if (!core_command_valid(a) || !core_command_valid(b) || a->op != b->op) return false;
 if (a->op >= AUTHORIZE && a->op <= ASSESS_LATE_FEE && strcmp(a->request_id,b->request_id)) return false;
 switch (a->op) {
  case QUOTE: return a->amount == b->amount && !strcmp(a->currency,b->currency) && a->tier == b->tier &&
   a->installments == b->installments && !strcmp(a->country,b->country);
  case AUTHORIZE: return !strcmp(a->account_id,b->account_id) && !strcmp(a->card_id,b->card_id) &&
   !strcmp(a->merchant_id,b->merchant_id) && a->amount == b->amount && !strcmp(a->currency,b->currency) &&
   a->installments == b->installments && !strcmp(a->country,b->country) && a->channel == b->channel &&
   a->card_present == b->card_present && a->pin_ok == b->pin_ok;
  case CAPTURE: return !strcmp(a->auth_id,b->auth_id) && a->principal == b->principal;
  case CANCEL: case GET_AUTH: return !strcmp(a->auth_id,b->auth_id);
  case REFUND: return !strcmp(a->capture_id,b->capture_id) && a->principal == b->principal;
  case CLOSE: return !strcmp(a->account_id,b->account_id) && a->cycle == b->cycle;
  case PAY: return !strcmp(a->account_id,b->account_id) && a->amount == b->amount;
  case ASSESS_LATE_FEE: case GET_INVOICE: return !strcmp(a->invoice_id,b->invoice_id);
  case TICK: return a->now == b->now;
  case GET_ACCOUNT: case RECONCILE: return !strcmp(a->account_id,b->account_id);
  case INVALID_OP: return false;
 }
 return false;
}

