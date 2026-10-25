/* Generated field-wise fingerprints. They are not behavioral expectations. */
static uint64_t hash_Config(uint64_t h, const Config *v) {
 h=hash_bytes(h, &((*v).fx_brl), sizeof((*v).fx_brl));
 h=hash_bytes(h, &((*v).fx_usd), sizeof((*v).fx_usd));
 h=hash_bytes(h, &((*v).fx_eur), sizeof((*v).fx_eur));
 h=hash_bytes(h, &((*v).international_bps), sizeof((*v).international_bps));
 h=hash_bytes(h, &((*v).installment_bps_per_extra), sizeof((*v).installment_bps_per_extra));
 h=hash_bytes(h, &((*v).tariff_cap), sizeof((*v).tariff_cap));
 h=hash_bytes(h, &((*v).pin_threshold), sizeof((*v).pin_threshold));
 h=hash_bytes(h, &((*v).minimum_installment), sizeof((*v).minimum_installment));
 h=hash_bytes(h, &((*v).minimum_due_floor), sizeof((*v).minimum_due_floor));
 h=hash_bytes(h, &((*v).late_fee), sizeof((*v).late_fee));
 h=hash_bytes(h, &((*v).hold_ttl_minutes), sizeof((*v).hold_ttl_minutes));
 h=hash_bytes(h, &((*v).daily_count_limit), sizeof((*v).daily_count_limit));
 h=hash_bytes(h, &((*v).review_score), sizeof((*v).review_score));
 h=hash_bytes(h, &((*v).decline_score), sizeof((*v).decline_score));
 h=hash_bytes(h, &((*v).cycle_days), sizeof((*v).cycle_days));
 h=hash_bytes(h, &((*v).due_grace_days), sizeof((*v).due_grace_days));
 h=hash_bytes(h, &((*v).minimum_due_bps), sizeof((*v).minimum_due_bps));
 h=hash_bytes(h, &((*v).reward_month_cap), sizeof((*v).reward_month_cap));
 h=hash_bytes(h, &((*v).reward_unit), sizeof((*v).reward_unit));
 h=hash_bytes(h, &((*v).accounts_capacity), sizeof((*v).accounts_capacity));
 h=hash_bytes(h, &((*v).cards_capacity), sizeof((*v).cards_capacity));
 h=hash_bytes(h, &((*v).merchants_capacity), sizeof((*v).merchants_capacity));
 h=hash_bytes(h, &((*v).auths_capacity), sizeof((*v).auths_capacity));
 h=hash_bytes(h, &((*v).captures_capacity), sizeof((*v).captures_capacity));
 h=hash_bytes(h, &((*v).lots_capacity), sizeof((*v).lots_capacity));
 h=hash_bytes(h, &((*v).invoices_capacity), sizeof((*v).invoices_capacity));
 h=hash_bytes(h, &((*v).history_capacity), sizeof((*v).history_capacity));
 h=hash_bytes(h, &((*v).ledger_capacity), sizeof((*v).ledger_capacity));
 h=hash_bytes(h, &((*v).keys_capacity), sizeof((*v).keys_capacity));
 h=hash_bytes(h, &((*v).rewards_capacity), sizeof((*v).rewards_capacity));
 h=hash_bytes(h, &((*v).payments_capacity), sizeof((*v).payments_capacity));
 h=hash_bytes(h, &((*v).refunds_capacity), sizeof((*v).refunds_capacity));
 return h;
}
static uint64_t hash_Account(uint64_t h, const Account *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_bytes(h, &((*v).active), sizeof((*v).active));
 h=hash_bytes(h, &((*v).tier), sizeof((*v).tier));
 h=hash_bytes(h, &((*v).credit_limit), sizeof((*v).credit_limit));
 h=hash_bytes(h, &((*v).daily_limit), sizeof((*v).daily_limit));
 h=hash_bytes(h, &((*v).per_operation_limit), sizeof((*v).per_operation_limit));
 h=hash_bytes(h, &((*v).base_score), sizeof((*v).base_score));
 h=hash_bytes(h, &((*v).next_cycle_to_close), sizeof((*v).next_cycle_to_close));
 h=hash_bytes(h, &((*v).cached_debt), sizeof((*v).cached_debt));
 h=hash_bytes(h, &((*v).cached_held), sizeof((*v).cached_held));
 h=hash_bytes(h, &((*v).cash_refund_total), sizeof((*v).cash_refund_total));
 return h;
}
static uint64_t hash_Card(uint64_t h, const Card *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_bytes(h, &((*v).active), sizeof((*v).active));
 h=hash_bytes(h, &((*v).expiry_day), sizeof((*v).expiry_day));
 h=hash_bytes(h, &((*v).allow_international), sizeof((*v).allow_international));
 h=hash_bytes(h, &((*v).allow_contactless), sizeof((*v).allow_contactless));
 return h;
}
static uint64_t hash_Merchant(uint64_t h, const Merchant *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_bytes(h, &((*v).active), sizeof((*v).active));
 h=hash_bytes(h, &((*v).category), sizeof((*v).category));
 h=hash_string(h, (*v).country, sizeof((*v).country));
 return h;
}
static uint64_t hash_Command(uint64_t h, const Command *v) {
 h=hash_bytes(h, &((*v).op), sizeof((*v).op));
 h=hash_string(h, (*v).request_id, sizeof((*v).request_id));
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_string(h, (*v).card_id, sizeof((*v).card_id));
 h=hash_string(h, (*v).merchant_id, sizeof((*v).merchant_id));
 h=hash_string(h, (*v).auth_id, sizeof((*v).auth_id));
 h=hash_string(h, (*v).capture_id, sizeof((*v).capture_id));
 h=hash_string(h, (*v).invoice_id, sizeof((*v).invoice_id));
 h=hash_bytes(h, &((*v).amount), sizeof((*v).amount));
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 h=hash_string(h, (*v).currency, sizeof((*v).currency));
 h=hash_string(h, (*v).country, sizeof((*v).country));
 h=hash_bytes(h, &((*v).tier), sizeof((*v).tier));
 h=hash_bytes(h, &((*v).channel), sizeof((*v).channel));
 h=hash_bytes(h, &((*v).card_present), sizeof((*v).card_present));
 h=hash_bytes(h, &((*v).pin_ok), sizeof((*v).pin_ok));
 h=hash_bytes(h, &((*v).installments), sizeof((*v).installments));
 h=hash_bytes(h, &((*v).cycle), sizeof((*v).cycle));
 h=hash_bytes(h, &((*v).now), sizeof((*v).now));
 return h;
}
static uint64_t hash_Result(uint64_t h, const Result *v) {
 h=hash_bytes(h, &((*v).op), sizeof((*v).op));
 h=hash_string(h, (*v).request_id, sizeof((*v).request_id));
 h=hash_bytes(h, &((*v).decision), sizeof((*v).decision));
 h=hash_bytes(h, &((*v).reason), sizeof((*v).reason));
 h=hash_string(h, (*v).auth_id, sizeof((*v).auth_id));
 h=hash_string(h, (*v).capture_id, sizeof((*v).capture_id));
 h=hash_string(h, (*v).invoice_id, sizeof((*v).invoice_id));
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 h=hash_bytes(h, &((*v).fee), sizeof((*v).fee));
 h=hash_bytes(h, &((*v).reserved), sizeof((*v).reserved));
 h=hash_bytes(h, &((*v).released), sizeof((*v).released));
 h=hash_bytes(h, &((*v).cash_refund), sizeof((*v).cash_refund));
 h=hash_bytes(h, &((*v).issued_total), sizeof((*v).issued_total));
 h=hash_bytes(h, &((*v).minimum_due), sizeof((*v).minimum_due));
 h=hash_bytes(h, &((*v).points), sizeof((*v).points));
 h=hash_bytes(h, &((*v).points_reversed), sizeof((*v).points_reversed));
 h=hash_bytes(h, &((*v).expires_at), sizeof((*v).expires_at));
 h=hash_bytes(h, &((*v).has_quote), sizeof((*v).has_quote));
 return h;
}
static uint64_t hash_Authorization(uint64_t h, const Authorization *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_bytes(h, &((*v).state), sizeof((*v).state));
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 h=hash_bytes(h, &((*v).fee), sizeof((*v).fee));
 h=hash_bytes(h, &((*v).captured_principal), sizeof((*v).captured_principal));
 h=hash_bytes(h, &((*v).captured_fee), sizeof((*v).captured_fee));
 h=hash_bytes(h, &((*v).approved_at), sizeof((*v).approved_at));
 h=hash_bytes(h, &((*v).expires_at), sizeof((*v).expires_at));
 h=hash_bytes(h, &((*v).installments), sizeof((*v).installments));
 h=hash_bytes(h, &((*v).tier), sizeof((*v).tier));
 h=hash_bytes(h, &((*v).category), sizeof((*v).category));
 return h;
}
static uint64_t hash_Capture(uint64_t h, const Capture *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_string(h, (*v).auth_id, sizeof((*v).auth_id));
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 h=hash_bytes(h, &((*v).fee), sizeof((*v).fee));
 h=hash_bytes(h, &((*v).refunded_principal), sizeof((*v).refunded_principal));
 h=hash_bytes(h, &((*v).refunded_fee), sizeof((*v).refunded_fee));
 h=hash_bytes(h, &((*v).cycle), sizeof((*v).cycle));
 h=hash_bytes(h, &((*v).granted), sizeof((*v).granted));
 h=hash_bytes(h, &((*v).reversed), sizeof((*v).reversed));
 return h;
}
static uint64_t hash_Lot(uint64_t h, const Lot *v) {
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_string(h, (*v).capture_id, sizeof((*v).capture_id));
 h=hash_string(h, (*v).invoice_id, sizeof((*v).invoice_id));
 h=hash_bytes(h, &((*v).index), sizeof((*v).index));
 h=hash_bytes(h, &((*v).cycle), sizeof((*v).cycle));
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 h=hash_bytes(h, &((*v).fee), sizeof((*v).fee));
 h=hash_bytes(h, &((*v).paid_principal), sizeof((*v).paid_principal));
 h=hash_bytes(h, &((*v).paid_fee), sizeof((*v).paid_fee));
 h=hash_bytes(h, &((*v).cancelled_principal), sizeof((*v).cancelled_principal));
 h=hash_bytes(h, &((*v).cancelled_fee), sizeof((*v).cancelled_fee));
 return h;
}
static uint64_t hash_Invoice(uint64_t h, const Invoice *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_bytes(h, &((*v).cycle), sizeof((*v).cycle));
 h=hash_bytes(h, &((*v).due_day), sizeof((*v).due_day));
 h=hash_bytes(h, &((*v).issued_principal), sizeof((*v).issued_principal));
 h=hash_bytes(h, &((*v).issued_fee), sizeof((*v).issued_fee));
 h=hash_bytes(h, &((*v).issued_total), sizeof((*v).issued_total));
 h=hash_bytes(h, &((*v).minimum_due), sizeof((*v).minimum_due));
 h=hash_bytes(h, &((*v).late), sizeof((*v).late));
 h=hash_bytes(h, &((*v).paid_late), sizeof((*v).paid_late));
 h=hash_bytes(h, &((*v).late_fee_assessed), sizeof((*v).late_fee_assessed));
 return h;
}
static uint64_t hash_History(uint64_t h, const History *v) {
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_bytes(h, &((*v).minute), sizeof((*v).minute));
 h=hash_bytes(h, &((*v).decision), sizeof((*v).decision));
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 return h;
}
static uint64_t hash_Entry(uint64_t h, const Entry *v) {
 h=hash_string(h, (*v).event_id, sizeof((*v).event_id));
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_bytes(h, &((*v).account), sizeof((*v).account));
 h=hash_bytes(h, &((*v).amount), sizeof((*v).amount));
 return h;
}
static uint64_t hash_Cached(uint64_t h, const Cached *v) {
 h=hash_bytes(h, &((*v).command.op), sizeof((*v).command.op));
 h=hash_string(h, (*v).command.request_id, sizeof((*v).command.request_id));
 h=hash_string(h, (*v).command.account_id, sizeof((*v).command.account_id));
 h=hash_string(h, (*v).command.card_id, sizeof((*v).command.card_id));
 h=hash_string(h, (*v).command.merchant_id, sizeof((*v).command.merchant_id));
 h=hash_string(h, (*v).command.auth_id, sizeof((*v).command.auth_id));
 h=hash_string(h, (*v).command.capture_id, sizeof((*v).command.capture_id));
 h=hash_string(h, (*v).command.invoice_id, sizeof((*v).command.invoice_id));
 h=hash_bytes(h, &((*v).command.amount), sizeof((*v).command.amount));
 h=hash_bytes(h, &((*v).command.principal), sizeof((*v).command.principal));
 h=hash_string(h, (*v).command.currency, sizeof((*v).command.currency));
 h=hash_string(h, (*v).command.country, sizeof((*v).command.country));
 h=hash_bytes(h, &((*v).command.tier), sizeof((*v).command.tier));
 h=hash_bytes(h, &((*v).command.channel), sizeof((*v).command.channel));
 h=hash_bytes(h, &((*v).command.card_present), sizeof((*v).command.card_present));
 h=hash_bytes(h, &((*v).command.pin_ok), sizeof((*v).command.pin_ok));
 h=hash_bytes(h, &((*v).command.installments), sizeof((*v).command.installments));
 h=hash_bytes(h, &((*v).command.cycle), sizeof((*v).command.cycle));
 h=hash_bytes(h, &((*v).command.now), sizeof((*v).command.now));
 h=hash_bytes(h, &((*v).result.op), sizeof((*v).result.op));
 h=hash_string(h, (*v).result.request_id, sizeof((*v).result.request_id));
 h=hash_bytes(h, &((*v).result.decision), sizeof((*v).result.decision));
 h=hash_bytes(h, &((*v).result.reason), sizeof((*v).result.reason));
 h=hash_string(h, (*v).result.auth_id, sizeof((*v).result.auth_id));
 h=hash_string(h, (*v).result.capture_id, sizeof((*v).result.capture_id));
 h=hash_string(h, (*v).result.invoice_id, sizeof((*v).result.invoice_id));
 h=hash_bytes(h, &((*v).result.principal), sizeof((*v).result.principal));
 h=hash_bytes(h, &((*v).result.fee), sizeof((*v).result.fee));
 h=hash_bytes(h, &((*v).result.reserved), sizeof((*v).result.reserved));
 h=hash_bytes(h, &((*v).result.released), sizeof((*v).result.released));
 h=hash_bytes(h, &((*v).result.cash_refund), sizeof((*v).result.cash_refund));
 h=hash_bytes(h, &((*v).result.issued_total), sizeof((*v).result.issued_total));
 h=hash_bytes(h, &((*v).result.minimum_due), sizeof((*v).result.minimum_due));
 h=hash_bytes(h, &((*v).result.points), sizeof((*v).result.points));
 h=hash_bytes(h, &((*v).result.points_reversed), sizeof((*v).result.points_reversed));
 h=hash_bytes(h, &((*v).result.expires_at), sizeof((*v).result.expires_at));
 h=hash_bytes(h, &((*v).result.has_quote), sizeof((*v).result.has_quote));
 return h;
}
static uint64_t hash_Payment(uint64_t h, const Payment *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_bytes(h, &((*v).amount), sizeof((*v).amount));
 return h;
}
static uint64_t hash_Refund(uint64_t h, const Refund *v) {
 h=hash_string(h, (*v).id, sizeof((*v).id));
 h=hash_string(h, (*v).capture_id, sizeof((*v).capture_id));
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 h=hash_bytes(h, &((*v).fee), sizeof((*v).fee));
 h=hash_bytes(h, &((*v).cash), sizeof((*v).cash));
 h=hash_bytes(h, &((*v).points), sizeof((*v).points));
 return h;
}
static uint64_t hash_Reward(uint64_t h, const Reward *v) {
 h=hash_string(h, (*v).account_id, sizeof((*v).account_id));
 h=hash_string(h, (*v).capture_id, sizeof((*v).capture_id));
 h=hash_bytes(h, &((*v).cycle), sizeof((*v).cycle));
 h=hash_bytes(h, &((*v).granted), sizeof((*v).granted));
 return h;
}
static uint64_t hash_Price(uint64_t h, const Price *v) {
 h=hash_bytes(h, &((*v).principal), sizeof((*v).principal));
 h=hash_bytes(h, &((*v).international_fee), sizeof((*v).international_fee));
 h=hash_bytes(h, &((*v).installment_fee), sizeof((*v).installment_fee));
 h=hash_bytes(h, &((*v).fee), sizeof((*v).fee));
 h=hash_bytes(h, &((*v).reason), sizeof((*v).reason));
 return h;
}
static uint64_t hash_Projection(uint64_t h, const Projection *v) {
 h=hash_bytes(h, &((*v).debt), sizeof((*v).debt));
 h=hash_bytes(h, &((*v).held), sizeof((*v).held));
 h=hash_bytes(h, &((*v).available_credit), sizeof((*v).available_credit));
 h=hash_bytes(h, &((*v).invoiced_outstanding), sizeof((*v).invoiced_outstanding));
 h=hash_bytes(h, &((*v).future_outstanding), sizeof((*v).future_outstanding));
 h=hash_bytes(h, &((*v).cash_refund_total), sizeof((*v).cash_refund_total));
 h=hash_bytes(h, &((*v).daily_gross_principal), sizeof((*v).daily_gross_principal));
 h=hash_bytes(h, &((*v).points), sizeof((*v).points));
 h=hash_bytes(h, &((*v).daily_count), sizeof((*v).daily_count));
 h=hash_bytes(h, &((*v).next_cycle_to_close), sizeof((*v).next_cycle_to_close));
 return h;
}
