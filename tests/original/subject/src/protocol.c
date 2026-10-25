#include "aurum.h"
#include <limits.h>
#include <string.h>

/* Input grammar and public envelope validation implement the input schema. */
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
bool identifier_valid(const char *s) { return s && bounded_id(s, ID_SIZE); }
static bool upper_code(const char *s, size_t n) {
 for (size_t i = 0; i < n; ++i) if (s[i] < 'A' || s[i] > 'Z') return false;
 return s[n] == '\0';
}
bool decimal_parse(const char *s, int64_t max, int64_t *value) {
 int64_t n = 0;
 if (!s || !*s || max < 0 || !value) return false;
 for (; *s; ++s) {
  if (*s < '0' || *s > '9') return false;
  int64_t d = *s - '0';
  if (d > max || n > (max - d) / 10) return false;
  n = n * 10 + d;
 }
 *value = n;
 return true;
}
void command_default(Command *c, Operation op) {
 memset(c, 0, sizeof(*c));
 c->op = op;
 memcpy(c->currency, "BRL", 4);
 memcpy(c->country, "BR", 3);
 c->tier = REGULAR;
 c->channel = POS;
 c->card_present = true;
 c->pin_ok = true;
 c->installments = 1;
}
static bool positive_money(Money v) { return v > 0 && money_valid(v); }
bool command_valid(const Command *c) {
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
bool command_equal(const Command *a, const Command *b) {
 if (!command_valid(a) || !command_valid(b) || a->op != b->op) return false;
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

/* The reader drains a rejected physical line, including its terminator. */
int read_line(FILE *f, char *line, size_t size) {
 size_t n = 0, bytes = 0;
 bool invalid = false;
 int ch;
 if (!f || !line || size == 0) return -2;
 while ((ch = fgetc(f)) != EOF) {
  if (bytes <= LINE_MAX_BYTES) ++bytes;
  if (bytes > LINE_MAX_BYTES) invalid = true;
  if (ch == 0 || ch > 127 || (ch < 32 && ch != '\t' && ch != '\r' && ch != '\n') || ch == 127) invalid = true;
  if (n + 1 < size) line[n++] = (char)ch; else invalid = true;
  if (ch == '\n') break;
 }
 line[n] = '\0';
 if (ferror(f)) return -2;
 if (bytes == 0) return 0;
 if (n && line[n - 1] == '\n') {
  line[--n] = '\0';
  if (n && line[n - 1] == '\r') line[--n] = '\0';
 }
 for (size_t i = 0; i < n; ++i) if (line[i] == '\r' || line[i] == '\n') invalid = true;
 return invalid ? -1 : 1;
}

typedef struct { char *key; char *value; } Token;
typedef struct { char text[LINE_MAX_BYTES + 1]; char *op; Token tokens[40]; size_t count; } Parsed;
static bool separator(char c) { return c == ' ' || c == '\t'; }
/* Shared lexical tokenizer, with no domain dispatch or configuration mutation. */
static int tokenize(const char *line, Parsed *p) {
 size_t n = 0;
 if (!line) return -1;
 while (n <= LINE_MAX_BYTES && line[n]) ++n;
 if (n > LINE_MAX_BYTES) return -1;
 memcpy(p->text, line, n + 1);
 if (n && p->text[n - 1] == '\n') {
  p->text[--n] = '\0';
  if (n && p->text[n - 1] == '\r') p->text[--n] = '\0';
 }
 for (size_t i = 0; i < n; ++i) {
  unsigned char c = (unsigned char)p->text[i];
  if (c < 32 && c != '\t') return -1;
  if (c > 126) return -1;
 }
 p->count = 0;
 char *cursor = p->text;
 while (separator(*cursor)) ++cursor;
 if (!*cursor || *cursor == '#') return 0;
 p->op = cursor;
 while (*cursor && !separator(*cursor)) ++cursor;
 if (*cursor) *cursor++ = '\0';
 while (*cursor) {
  while (separator(*cursor)) ++cursor;
  if (!*cursor) break;
  if (p->count == sizeof(p->tokens) / sizeof(p->tokens[0])) return -1;
  char *key = cursor;
  while (*cursor && !separator(*cursor)) ++cursor;
  if (*cursor) *cursor++ = '\0';
  char *equal = strchr(key, '=');
  if (!equal || equal == key || !equal[1] || strchr(equal + 1, '=')) return -1;
  *equal = '\0';
  for (size_t i = 0; i < p->count; ++i) if (!strcmp(p->tokens[i].key,key)) return -1;
  p->tokens[p->count++] = (Token){ key, equal + 1 };
 }
 return 1;
}
static bool parse_bool(const char *text, bool *v) {
 if (!strcmp(text,"true")) { *v = true; return true; }
 if (!strcmp(text,"false")) { *v = false; return true; }
 return false;
}
static bool copy_id(char *out, const char *text) {
 if (!identifier_valid(text)) return false;
 memcpy(out,text,strlen(text) + 1);
 return true;
}
static bool copy_code(char *out, const char *text, size_t n) {
 if (strlen(text) != n || !upper_code(text,n)) return false;
 memcpy(out,text,n + 1);
 return true;
}
static bool parse_tier(const char *text, Tier *tier) {
 if (!strcmp(text,"REGULAR")) { *tier = REGULAR; return true; }
 if (!strcmp(text,"PREMIUM")) { *tier = PREMIUM; return true; }
 return false;
}
static bool parse_status(const char *text, bool *active) {
 if (!strcmp(text,"ACTIVE")) { *active = true; return true; }
 if (!strcmp(text,"BLOCKED")) { *active = false; return true; }
 return false;
}
static bool parse_channel(const char *text, Channel *v) {
 if (!strcmp(text,"POS")) { *v = POS; return true; }
 if (!strcmp(text,"ECOM")) { *v = ECOM; return true; }
 if (!strcmp(text,"CONTACTLESS")) { *v = CONTACTLESS; return true; }
 return false;
}
enum Field { F_REQUEST, F_ACCOUNT, F_CARD, F_MERCHANT, F_AUTH, F_CAPTURE, F_INVOICE, F_AMOUNT,
 F_PRINCIPAL, F_CURRENCY, F_COUNTRY, F_TIER, F_CHANNEL, F_PRESENT, F_PIN, F_INSTALLMENTS, F_CYCLE, F_NOW, F_COUNT };
#define BIT(f) (UINT64_C(1) << (f))
static const char *const fields[F_COUNT] = { "request_id", "account_id", "card_id", "merchant_id", "auth_id",
 "capture_id", "invoice_id", "amount", "principal", "currency", "country", "tier", "channel", "card_present",
 "pin_ok", "installments", "cycle", "now" };
static uint64_t required_fields(Operation op) {
 switch (op) {
  case QUOTE: return BIT(F_AMOUNT);
  case AUTHORIZE: return BIT(F_REQUEST)|BIT(F_ACCOUNT)|BIT(F_CARD)|BIT(F_MERCHANT)|BIT(F_AMOUNT);
  case CAPTURE: return BIT(F_REQUEST)|BIT(F_AUTH)|BIT(F_PRINCIPAL);
  case CANCEL: return BIT(F_REQUEST)|BIT(F_AUTH);
  case REFUND: return BIT(F_REQUEST)|BIT(F_CAPTURE)|BIT(F_PRINCIPAL);
  case CLOSE: return BIT(F_REQUEST)|BIT(F_ACCOUNT)|BIT(F_CYCLE);
  case PAY: return BIT(F_REQUEST)|BIT(F_ACCOUNT)|BIT(F_AMOUNT);
  case ASSESS_LATE_FEE: return BIT(F_REQUEST)|BIT(F_INVOICE);
  case TICK: return BIT(F_NOW);
  case GET_ACCOUNT: return BIT(F_ACCOUNT);
  case GET_AUTH: return BIT(F_AUTH);
  case GET_INVOICE: return BIT(F_INVOICE);
  case RECONCILE: case INVALID_OP: return 0;
 }
 return 0;
}
static uint64_t allowed_fields(Operation op) {
 uint64_t fields_mask = required_fields(op);
 if (op == QUOTE) fields_mask |= BIT(F_CURRENCY)|BIT(F_TIER)|BIT(F_INSTALLMENTS)|BIT(F_COUNTRY);
 if (op == AUTHORIZE) fields_mask |= BIT(F_CURRENCY)|BIT(F_INSTALLMENTS)|BIT(F_COUNTRY)|BIT(F_CHANNEL)|BIT(F_PRESENT)|BIT(F_PIN);
 if (op == RECONCILE) fields_mask |= BIT(F_ACCOUNT);
 return fields_mask;
}
static bool command_field(Command *c, enum Field field, const char *value) {
 switch (field) {
  case F_REQUEST: return copy_id(c->request_id,value);
  case F_ACCOUNT: return copy_id(c->account_id,value);
  case F_CARD: return copy_id(c->card_id,value);
  case F_MERCHANT: return copy_id(c->merchant_id,value);
  case F_AUTH: return copy_id(c->auth_id,value);
  case F_CAPTURE: return copy_id(c->capture_id,value);
  case F_INVOICE: return copy_id(c->invoice_id,value);
  case F_AMOUNT: return decimal_parse(value,MONEY_MAX,&c->amount);
  case F_PRINCIPAL: return decimal_parse(value,MONEY_MAX,&c->principal);
  case F_CURRENCY: return copy_code(c->currency,value,3);
  case F_COUNTRY: return copy_code(c->country,value,2);
  case F_TIER: return parse_tier(value,&c->tier);
  case F_CHANNEL: return parse_channel(value,&c->channel);
  case F_PRESENT: return parse_bool(value,&c->card_present);
  case F_PIN: return parse_bool(value,&c->pin_ok);
  case F_INSTALLMENTS: return decimal_parse(value,INT64_MAX,&c->installments);
  case F_CYCLE: return decimal_parse(value,INT64_MAX,&c->cycle);
  case F_NOW: return decimal_parse(value,TIME_MAX,&c->now);
  case F_COUNT: return false;
 }
 return false;
}
int parse_command(const char *line, Command *c) {
 Parsed p;
 command_default(c,INVALID_OP);
 int status = tokenize(line,&p);
 if (status != 1) return status;
 for (Operation op = QUOTE; op < INVALID_OP; op = (Operation)(op + 1))
  if (!strcmp(operation_name(op),p.op)) { c->op = op; break; }
 if (c->op == INVALID_OP) return -1;
 uint64_t seen = 0, allowed = allowed_fields(c->op), required = required_fields(c->op);
 for (size_t i = 0; i < p.count; ++i) {
  enum Field field = F_COUNT;
  for (enum Field j = F_REQUEST; j < F_COUNT; j = (enum Field)(j + 1))
   if (!strcmp(fields[j],p.tokens[i].key)) { field = j; break; }
  if (field == F_COUNT || !(allowed & BIT(field)) || !command_field(c,field,p.tokens[i].value)) return -1;
  seen |= BIT(field);
 }
 return (seen & required) == required && command_valid(c) ? 1 : -1;
}

/* Named configuration fields are a schema, not a runtime business-rule table. */
static const char *const config_names[] = {
 "fx_brl", "fx_usd", "fx_eur", "international_bps", "installment_bps_per_extra",
 "tariff_cap", "pin_threshold", "minimum_installment", "minimum_due_floor", "late_fee",
 "hold_ttl_minutes", "daily_count_limit", "review_score", "decline_score", "cycle_days",
 "due_grace_days", "minimum_due_bps", "reward_month_cap", "reward_unit",
 "accounts_capacity", "cards_capacity", "merchants_capacity", "auths_capacity", "captures_capacity",
 "lots_capacity", "invoices_capacity", "history_capacity", "ledger_capacity", "keys_capacity",
 "rewards_capacity", "payments_capacity", "refunds_capacity"
};
static bool config_field(Config *c, size_t field, const char *value) {
 int64_t n;
 if (!decimal_parse(value,field >= 19 ? 1000000 : MONEY_MAX,&n)) return false;
 switch (field) {
  case 0: c->fx_brl = n; break; case 1: c->fx_usd = n; break; case 2: c->fx_eur = n; break;
  case 3: c->international_bps = n; break; case 4: c->installment_bps_per_extra = n; break;
  case 5: c->tariff_cap = n; break; case 6: c->pin_threshold = n; break;
  case 7: c->minimum_installment = n; break; case 8: c->minimum_due_floor = n; break;
  case 9: c->late_fee = n; break; case 10: c->hold_ttl_minutes = n; break;
  case 11: c->daily_count_limit = n; break; case 12: c->review_score = n; break;
  case 13: c->decline_score = n; break; case 14: c->cycle_days = n; break;
  case 15: c->due_grace_days = n; break; case 16: c->minimum_due_bps = n; break;
  case 17: c->reward_month_cap = n; break; case 18: c->reward_unit = n; break;
  case 19: c->accounts_capacity = (size_t)n; break; case 20: c->cards_capacity = (size_t)n; break;
  case 21: c->merchants_capacity = (size_t)n; break; case 22: c->auths_capacity = (size_t)n; break;
  case 23: c->captures_capacity = (size_t)n; break; case 24: c->lots_capacity = (size_t)n; break;
  case 25: c->invoices_capacity = (size_t)n; break; case 26: c->history_capacity = (size_t)n; break;
  case 27: c->ledger_capacity = (size_t)n; break; case 28: c->keys_capacity = (size_t)n; break;
  case 29: c->rewards_capacity = (size_t)n; break; case 30: c->payments_capacity = (size_t)n; break;
  case 31: c->refunds_capacity = (size_t)n; break;
  default: return false;
 }
 return true;
}
static bool seed_config(Engine *e, const Parsed *p, uint64_t *seen) {
 if (!p->count) return false;
 for (size_t i = 0; i < p->count; ++i) {
  size_t j;
  for (j = 0; j < sizeof(config_names)/sizeof(config_names[0]); ++j)
   if (!strcmp(config_names[j],p->tokens[i].key)) break;
  if (j == sizeof(config_names)/sizeof(config_names[0]) || (*seen & BIT(j))) return false;
  if (!config_field(&e->config,j,p->tokens[i].value)) return false;
  *seen |= BIT(j);
 }
 return true;
}
static bool seed_account(Engine *e, const Parsed *p) {
 Account a = {0};
 a.active = true; a.tier = REGULAR; a.credit_limit = 1000000;
 a.daily_limit = 300000; a.per_operation_limit = 200000; a.base_score = 10;
 a.next_cycle_to_close = e->now / 1440 / e->config.cycle_days;
 for (size_t i = 0; i < p->count; ++i) {
  const char *key = p->tokens[i].key, *v = p->tokens[i].value;
  bool ok;
  if (!strcmp(key,"id")) ok = copy_id(a.id,v);
  else if (!strcmp(key,"active")) ok = parse_bool(v,&a.active);
  else if (!strcmp(key,"tier")) ok = parse_tier(v,&a.tier);
  else if (!strcmp(key,"credit_limit")) ok = decimal_parse(v,MONEY_MAX,&a.credit_limit);
  else if (!strcmp(key,"daily_limit")) ok = decimal_parse(v,MONEY_MAX,&a.daily_limit);
  else if (!strcmp(key,"per_operation_limit")) ok = decimal_parse(v,MONEY_MAX,&a.per_operation_limit);
  else if (!strcmp(key,"base_score")) ok = decimal_parse(v,100,&a.base_score);
  else return false;
  if (!ok) return false;
 }
 return a.id[0] && !find_account(e,a.id) && array_append((void **)&e->accounts,&e->accounts_len,&e->accounts_alloc,e->config.accounts_capacity,sizeof(a),&a);
}
static bool seed_card(Engine *e, const Parsed *p) {
 Card card = {0};
 card.active = true; card.expiry_day = 500; card.allow_international = true; card.allow_contactless = true;
 for (size_t i = 0; i < p->count; ++i) {
  const char *key = p->tokens[i].key, *v = p->tokens[i].value;
  bool ok;
  if (!strcmp(key,"id")) ok = copy_id(card.id,v);
  else if (!strcmp(key,"account_id")) ok = copy_id(card.account_id,v);
  else if (!strcmp(key,"status")) ok = parse_status(v,&card.active);
  else if (!strcmp(key,"expiry_day")) ok = decimal_parse(v,INT64_MAX,&card.expiry_day);
  else if (!strcmp(key,"allow_international")) ok = parse_bool(v,&card.allow_international);
  else if (!strcmp(key,"allow_contactless")) ok = parse_bool(v,&card.allow_contactless);
  else return false;
  if (!ok) return false;
 }
 return card.id[0] && card.account_id[0] && find_account(e,card.account_id) && !find_card(e,card.id) &&
  array_append((void **)&e->cards,&e->cards_len,&e->cards_alloc,e->config.cards_capacity,sizeof(card),&card);
}
static bool seed_merchant(Engine *e, const Parsed *p) {
 Merchant m = {0};
 m.active = true; m.category = NORMAL; memcpy(m.country,"BR",3);
 for (size_t i = 0; i < p->count; ++i) {
  const char *key = p->tokens[i].key, *v = p->tokens[i].value;
  bool ok;
  if (!strcmp(key,"id")) ok = copy_id(m.id,v);
  else if (!strcmp(key,"status")) ok = parse_status(v,&m.active);
  else if (!strcmp(key,"country")) ok = copy_code(m.country,v,2);
  else if (!strcmp(key,"category")) {
   if (!strcmp(v,"NORMAL")) { m.category = NORMAL; ok = true; }
   else if (!strcmp(v,"RESTRICTED")) { m.category = RESTRICTED; ok = true; }
   else ok = false;
  } else return false;
  if (!ok) return false;
 }
 return m.id[0] && !find_merchant(e,m.id) && array_append((void **)&e->merchants,&e->merchants_len,&e->merchants_alloc,e->config.merchants_capacity,sizeof(m),&m);
}
bool seed_load(FILE *f, Engine *e) {
 Engine pending;
 engine_init(&pending);
 char line[LINE_MAX_BYTES + 1];
 uint64_t config_seen = 0;
 int phase = 0, status;
 bool ok = true;
 while ((status = read_line(f,line,sizeof(line))) != 0) {
  if (status < 0) { ok = false; break; }
  Parsed p;
  status = tokenize(line,&p);
  if (!status) continue;
  if (status < 0) { ok = false; break; }
  if (!strcmp(p.op,"CLOCK")) {
   if (phase != 0 || p.count != 1 || strcmp(p.tokens[0].key,"now") || !decimal_parse(p.tokens[0].value,TIME_MAX,&pending.now)) { ok = false; break; }
   phase = 1;
  } else if (!strcmp(p.op,"CONFIG")) {
   if (phase != 1 || !seed_config(&pending,&p,&config_seen)) { ok = false; break; }
  } else {
   int next = !strcmp(p.op,"ACCOUNT") ? 2 : !strcmp(p.op,"CARD") ? 3 : !strcmp(p.op,"MERCHANT") ? 4 : -1;
   if (phase == 0 || next < phase || !config_valid(&pending.config)) { ok = false; break; }
   phase = next;
   if ((next == 2 && !seed_account(&pending,&p)) || (next == 3 && !seed_card(&pending,&p)) ||
       (next == 4 && !seed_merchant(&pending,&p))) { ok = false; break; }
  }
 }
 if (!phase || !config_valid(&pending.config)) ok = false;
 if (ok) { engine_free(e); *e = pending; }
 else engine_free(&pending);
 return ok;
}
