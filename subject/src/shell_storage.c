#include "aurum.h"
#include <stdlib.h>
#include <string.h>

/* Collection ownership is mechanical; policy and edit order belong to core. */
#define COLLECTIONS(X) \
 X(accounts, Account, TABLE_ACCOUNT, account) \
 X(cards, Card, TABLE_CARD, card) \
 X(merchants, Merchant, TABLE_MERCHANT, merchant) \
 X(auths, Authorization, TABLE_AUTH, auth) \
 X(captures, Capture, TABLE_CAPTURE, capture) \
 X(lots, Lot, TABLE_LOT, lot) \
 X(invoices, Invoice, TABLE_INVOICE, invoice) \
 X(history, History, TABLE_HISTORY, history) \
 X(ledger, Entry, TABLE_LEDGER, entry) \
 X(keys, Cached, TABLE_KEY, key) \
 X(payments, Payment, TABLE_PAYMENT, payment) \
 X(refunds, Refund, TABLE_REFUND, refund) \
 X(rewards, Reward, TABLE_REWARD, reward)

void engine_init(Engine *e) {
    memset(e, 0, sizeof *e);
    e->config = core_config_default();
}
void engine_free(Engine *e) {
#define RELEASE(name, type, table, member) free(e->name);
    COLLECTIONS(RELEASE)
#undef RELEASE
    memset(e, 0, sizeof *e);
}
StateView engine_view(const Engine *e) {
    StateView view = {.config = e->config, .now = e->now};
#define VIEW(name, type, table, member) view.name = e->name; view.name##_len = e->name##_len;
    COLLECTIONS(VIEW)
#undef VIEW
    return view;
}
bool array_append(void **data, size_t *len, size_t *alloc, size_t limit, size_t size, const void *value) {
    if (*len >= limit || size == 0 || *len == SIZE_MAX) return false;
    if (*len == *alloc) {
        size_t next = *alloc == 0 ? 8 : (*alloc > SIZE_MAX / 2 ? SIZE_MAX : *alloc * 2);
        if (next > limit) next = limit;
        if (next > SIZE_MAX / size) return false;
        void *grown = realloc(*data, next * size);
        if (!grown) return false;
        *data = grown;
        *alloc = next;
    }
    memcpy((unsigned char *)*data + *len * size, value, size);
    ++*len;
    return true;
}
bool engine_clone(const Engine *src, Engine *dst) {
    engine_init(dst);
    dst->config = src->config;
    dst->now = src->now;
#define CLONE(name, type, table, member) \
    if (src->name##_len) { \
        if (src->name##_len > SIZE_MAX / sizeof(type)) { engine_free(dst); return false; } \
        dst->name = malloc(src->name##_len * sizeof(type)); \
        if (!dst->name) { engine_free(dst); return false; } \
        memcpy(dst->name, src->name, src->name##_len * sizeof(type)); \
        dst->name##_len = src->name##_len; \
        dst->name##_alloc = src->name##_len; \
    }
    COLLECTIONS(CLONE)
#undef CLONE
    return true;
}
static bool apply_edit(Engine *e, Edit edit) {
    switch (edit.table) {
#define APPLY(name, type, table, member) \
        case table: \
            if (edit.append) return array_append((void **)&e->name, &e->name##_len, &e->name##_alloc, \
                e->config.name##_capacity, sizeof(type), &edit.value.member); \
            if (edit.index >= e->name##_len) return false; \
            e->name[edit.index] = edit.value.member; \
            return true;
        COLLECTIONS(APPLY)
#undef APPLY
        case TABLE_CLOCK: e->now = edit.value.now; return true;
    }
    return false;
}
Result execute(Engine *e, const Command *command) {
    Command input = command ? *command : core_command_default(INVALID_OP);
    Engine scratch;
    bool staged = false;
    Continuation continuation = core_begin();
    for (;;) {
        Step step = core_step(engine_view(staged ? &scratch : e), input, continuation);
        switch (step.status) {
            case CORE_READ:
            case CORE_ABORT:
                if (staged) engine_free(&scratch);
                return step.result;
            case CORE_COMMIT:
                if (staged) {
                    engine_free(e);
                    *e = scratch;
                }
                return step.result;
            case CORE_EDIT:
                if (!staged) {
                    if (!engine_clone(e, &scratch)) return core_capacity_failure(input);
                    staged = true;
                }
                if (!apply_edit(&scratch, step.edit)) {
                    engine_free(&scratch);
                    return core_capacity_failure(input);
                }
                continuation = step.next;
                break;
        }
    }
}
