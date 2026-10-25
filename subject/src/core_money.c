#include "core.h"
#include <limits.h>

bool core_money_valid(Money value) { return value >= 0 && value <= MONEY_MAX; }
Number core_checked_mul(int64_t a, int64_t b) {
    if (a < 0 || b < 0 || (b && a > INT64_MAX / b)) return (Number){0};
    return (Number){true, a * b};
}
Number core_money_add(Money a, Money b) {
    if (!core_money_valid(a) || !core_money_valid(b) || a > MONEY_MAX - b) return (Number){0};
    return (Number){true, a + b};
}
Number core_money_sub(Money a, Money b) {
    if (!core_money_valid(a) || !core_money_valid(b) || a < b) return (Number){0};
    return (Number){true, a - b};
}
Number core_floor_rate(Money value, int64_t bps) {
    if (!core_money_valid(value) || bps < 0 || bps > 10000) return (Number){0};
    Number product = core_checked_mul(value, bps);
    return product.ok ? (Number){true, product.value / 10000} : product;
}
Number core_ceil_rate(Money value, int64_t bps) {
    if (!core_money_valid(value) || bps < 0 || bps > 10000) return (Number){0};
    Number product = core_checked_mul(value, bps);
    return product.ok ? (Number){true, product.value / 10000 + (product.value % 10000 != 0)} : product;
}
Number core_money_fx(Money value, int64_t rate) {
    if (!core_money_valid(value) || rate < 1 || rate > 100000) return (Number){0};
    Number product = core_checked_mul(value, rate);
    if (!product.ok) return product;
    Money converted = product.value / 10000 + (product.value % 10000 >= 5000);
    return core_money_valid(converted) ? (Number){true, converted} : (Number){0};
}
Number core_proportional(Money total, Money part, Money whole) {
    if (!core_money_valid(total) || !core_money_valid(part) || !core_money_valid(whole) ||
        !whole || part > whole) return (Number){0};
    Money quotient = 0, remainder = 0;
    for (int bit = 39; bit >= 0; --bit) {
        Money next = remainder * 2 + (((uint64_t)total >> bit) & 1U ? part : 0);
        quotient = quotient * 2 + next / whole;
        remainder = next % whole;
    }
    return (Number){true, quotient};
}
Number core_cumulative_delta(Money total, Money before, Money amount, Money whole) {
    Number after = core_money_add(before, amount);
    if (!after.ok) return after;
    Number previous = core_proportional(total, before, whole);
    Number current = core_proportional(total, after.value, whole);
    if (!previous.ok || !current.ok) return (Number){0};
    return (Number){true, current.value - previous.value};
}
Money core_split_part(Money total, int64_t n, int64_t index) {
    if (!core_money_valid(total) || n < 1 || n > 12 || index < 0 || index >= n) return -1;
    return total / n + (index < total % n);
}
