#include "aurum.h"
#include <limits.h>

bool money_valid(Money v) { return v >= 0 && v <= MONEY_MAX; }
bool checked_mul(int64_t a, int64_t b, int64_t *out) {
    if (a < 0 || b < 0 || (b != 0 && a > INT64_MAX / b)) return false;
    *out = a * b;
    return true;
}
bool money_add(Money a, Money b, Money *out) {
    if (!money_valid(a) || !money_valid(b) || a > MONEY_MAX - b) return false;
    *out = a + b;
    return true;
}
bool money_sub(Money a, Money b, Money *out) {
    if (!money_valid(a) || !money_valid(b) || a < b) return false;
    *out = a - b;
    return true;
}
bool floor_rate(Money v, int64_t bps, Money *out) {
    int64_t product;
    if (!money_valid(v) || bps < 0 || bps > 10000 || !checked_mul(v, bps, &product)) return false;
    *out = product / 10000;
    return true;
}
bool ceil_rate(Money v, int64_t bps, Money *out) {
    int64_t product;
    if (!money_valid(v) || bps < 0 || bps > 10000 || !checked_mul(v, bps, &product)) return false;
    *out = product / 10000 + (product % 10000 != 0);
    return true;
}
bool money_fx(Money v, int64_t rate, Money *out) {
    int64_t product;
    if (!money_valid(v) || rate < 1 || rate > 100000 || !checked_mul(v, rate, &product)) return false;
    Money converted = product / 10000 + (product % 10000 >= 5000);
    if (!money_valid(converted)) return false;
    *out = converted;
    return true;
}
bool proportional(Money total, Money part, Money whole, Money *out) {
    if (!money_valid(total) || !money_valid(part) || !money_valid(whole) || whole == 0 || part > whole) return false;
    Money quotient = 0, remainder = 0;
    for (int bit = 39; bit >= 0; --bit) {
        Money next = remainder * 2 + (((uint64_t)total >> bit) & 1U ? part : 0);
        quotient = quotient * 2 + next / whole;
        remainder = next % whole;
    }
    *out = quotient;
    return true;
}
bool cumulative_delta(Money total, Money before, Money amount, Money whole, Money *out) {
    Money after, previous, current;
    if (!money_add(before, amount, &after) || !proportional(total, before, whole, &previous) ||
        !proportional(total, after, whole, &current)) return false;
    *out = current - previous;
    return true;
}
Money split_part(Money total, int64_t n, int64_t index) {
    if (!money_valid(total) || n < 1 || n > 12 || index < 0 || index >= n) return -1;
    return total / n + (index < total % n);
}
