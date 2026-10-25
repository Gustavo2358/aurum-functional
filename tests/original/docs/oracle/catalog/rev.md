# Cancelamento expiração e reembolso

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-REV-001 — Cancelamento de reserva intacta libera todo hold

CANCEL em ACTIVE sem captura libera P+F e muda para CANCELLED, sem criar recebível ou reembolso em dinheiro.

**Observação executada:** `reversal.cancel`.

**Exemplos:** 2 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-001` e `SC-BR-REV-001-NN`.

## BR-REV-002 — Cancelamento parcial preserva a captura

CANCEL em PARTIAL libera apenas principal+tarifa remanescentes, sem reembolsar o que já virou dívida.

**Observação executada:** `reversal.cancel`.

**Exemplos:** 2 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-002` e `SC-BR-REV-002-NN`.

## BR-REV-003 — Cancelamento de terminal é NOOP

CANCEL de CAPTURED/CANCELLED/EXPIRED com chave nova retorna NOOP e não muda o diário. Referência ausente não éNOOP.

**Observação executada:** `reversal.cancel_terminal`.

**Exemplos:** 3 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-003` e `SC-BR-REV-003-NN`.

## BR-REV-004 — Expiração inclui o instante limite

TICK expira reservas abertas com expires_at<=now. Expirar reserva parcialmente capturada preserva dívida/pontos da captura e não altera CAPTURED.

**Observação executada:** `reversal.expire`.

**Exemplos:** 3 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-004` e `SC-BR-REV-004-NN`.

## BR-REV-005 — Reembolso respeita o principal da captura

Solicitação positiva e soma de reembolsos<=D da captura referenciada. Ultrapassar éDECLINED/REFUND_AMOUNT; zero éINVALID_INPUT. Não usar principal total da autorização como teto.

**Observação executada:** `reversal.refund_amount`.

**Exemplos:** 3 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-005` e `SC-BR-REV-005-NN`.

## BR-REV-006 — Tarifa devolvida é cumulativa por captura

Para tarifa G da captura D, devolver floor(G*(r+d)/D)-floor(G*r/D). Reembolso total devolve exatamente G, sem usar FX corrente.

**Observação executada:** `reversal.refund_fee`.

**Exemplos:** 2 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-006` e `SC-BR-REV-006-NN`.

## BR-REV-007 — Reembolso abate lotes mais distantes primeiro

Dentro da captura, cancelar principal e tarifa ainda devidos separadamente, por ciclo mais distante e índice de parcela decrescente. Outros recebíveis da conta não recebem essa baixa.

**Observação executada:** `reversal.refund_allocation`.

**Exemplos:** 2 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-007` e `SC-BR-REV-007-NN`.

## BR-REV-008 — Parte já paga é devolvida em dinheiro

Após abater o saldo ainda devido da mesma captura/componente, o resto do reembolso soma a cash_refund_total. Não tornar debt negativa nem criar crédito reutilizável.

**Observação executada:** `reversal.refund_paid`.

**Exemplos:** 2 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-008` e `SC-BR-REV-008-NN`.

## BR-REV-009 — Reembolso não reabre autorização nem desfaz consumo

Reembolsar captura preserva estado terminal da autorização, reserva zero e principal diário bruto. Referência válida pode ser reembolsada depois de cancelamento/expiração da reserva.

**Observação executada:** `reversal.refund_closed_auth`.

**Exemplos:** 2 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-009` e `SC-BR-REV-009-NN`.

## BR-REV-010 — Reembolso não devolve multa

Multa de atraso é ligada à fatura, não à captura; reembolso integral do principal/tarifa não elimina a multa já lançada.

**Observação executada:** `reversal.refund_with_late_fee`.

**Exemplos:** 2 cenários em [Gherkin](../features/rev.feature), tags `BR-REV-010` e `SC-BR-REV-010-NN`.
