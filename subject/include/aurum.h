#ifndef AURUM_H
#define AURUM_H
#include "core.h"
#include <stdio.h>
#define LINE_MAX_BYTES 8192

/* Engine owns every array. Callers own Command/Result values. No pointers survive a mutation. */
typedef struct {
 Config config; int64_t now;
 Account *accounts; size_t accounts_len, accounts_alloc;
 Card *cards; size_t cards_len, cards_alloc;
 Merchant *merchants; size_t merchants_len, merchants_alloc;
 Authorization *auths; size_t auths_len, auths_alloc;
 Capture *captures; size_t captures_len, captures_alloc;
 Lot *lots; size_t lots_len, lots_alloc;
 Invoice *invoices; size_t invoices_len, invoices_alloc;
 History *history; size_t history_len, history_alloc;
 Entry *ledger; size_t ledger_len, ledger_alloc;
 Cached *keys; size_t keys_len, keys_alloc;
 Payment *payments; size_t payments_len, payments_alloc;
 Refund *refunds; size_t refunds_len, refunds_alloc;
 Reward *rewards; size_t rewards_len, rewards_alloc;
} Engine;

/* The shell owns storage and translates between streams and domain values. */
void engine_init(Engine *e);
void engine_free(Engine *e);
bool engine_clone(const Engine *src, Engine *dst);
StateView engine_view(const Engine *e);
bool array_append(void **data, size_t *len, size_t *alloc, size_t limit, size_t size, const void *value);
Result execute(Engine *e, const Command *c);
bool decimal_parse(const char *s, int64_t max, int64_t *value);
int parse_command(const char *line, Command *c);
bool seed_load(FILE *f, Engine *e);
int read_line(FILE *f, char *line, size_t size);
const char *operation_name(Operation op);
const char *decision_name(Decision d);
const char *reason_name(Reason r);
const char *state_name(AuthState s);
const char *ledger_name(LedgerAccount a);
void write_result(FILE *f, const Engine *e, const Command *c, const Result *r);
#endif
