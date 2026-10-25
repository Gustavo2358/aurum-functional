# Faturas pagamentos e multa

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-BIL-001 — Ciclo só fecha depois de terminar

CLOSE de cycle exige day>=(cycle+1)*30; equality passa. Não antecipar fechamento de ciclo em andamento.

**Observação executada:** `billing.close_time`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-001` e `SC-BR-BIL-001-NN`.

## BR-BIL-002 — Fechamento respeita ordem e é único

Primeiro fechamento usa ciclo de criação; depois só next_cycle_to_close. Ciclo anterior já fechado com nova chave éNOOP; pular ciclo éCYCLE_ORDER.

**Observação executada:** `billing.close_order`.

**Exemplos:** 3 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-002` e `SC-BR-BIL-002-NN`.

## BR-BIL-003 — Fatura inclui apenas lotes do ciclo líquido de reembolso anterior

Emissão soma principal/tarifa não cancelados dos lotes agendados no ciclo; futuras parcelas não entram, embora já componham debt.

**Observação executada:** `billing.issue`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-003` e `SC-BR-BIL-003-NN`.

## BR-BIL-004 — Ciclo vazio produz fatura zero válida

Não omitir fechamento sem consumo: criar emissão com total/minimum_due zero, avançando o ciclo. Isso não cria lançamento financeiro.

**Observação executada:** `billing.empty`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-004` e `SC-BR-BIL-004-NN`.

## BR-BIL-005 — Vencimento deriva do ciclo, não da hora do fechamento

due_day=(cycle+1)*30+10. Fechamento tardio não prorroga vencimento.

**Observação executada:** `billing.due_day`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-005` e `SC-BR-BIL-005-NN`.

## BR-BIL-006 — Pagamento mínimo usa teto, piso e total

minimum_due=min(total,max(1000,ceil(total*1000/10000))); totalzero resulta emzero. Não exceder o saldo emitido nem usar arredondamento inferior.

**Observação executada:** `billing.minimum_due`.

**Exemplos:** 4 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-006` e `SC-BR-BIL-006-NN`.

## BR-BIL-007 — Pagamento prioriza multa, tarifa e principal

Aplicar montante por classes: multa→tarifa→principal; dentro de classe, vencimento mais antigo e chaves estáveis. Não ratear proporcionalmente entre classes.

**Observação executada:** `billing.allocate_payment`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-007` e `SC-BR-BIL-007-NN`.

## BR-BIL-008 — Pagamento não antecipa parcelas ainda não faturadas

Só saldo já faturado é elegível. Parcela futura permanece na dívida e no crédito comprometido após quitação da fatura atual.

**Observação executada:** `billing.pay_invoiced`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-008` e `SC-BR-BIL-008-NN`.

## BR-BIL-009 — Pagamento excedente é recusado integralmente

payment>invoiced_outstanding → DECLINED/OVERPAYMENT, sem consumo parcial ou saldo credor. Zero éINVALID_INPUT.

**Observação executada:** `billing.payment_admission`.

**Exemplos:** 3 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-009` e `SC-BR-BIL-009-NN`.

## BR-BIL-010 — Pagamento parcial reduz saldo mas não reescreve emissão

Snapshot de emissão e minimum_due original ficam estáveis; outstanding diminui. Saldo não quitado após due_day permanece inadimplente mesmo com mínimo pago.

**Observação executada:** `billing.partial_payment`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-010` e `SC-BR-BIL-010-NN`.

## BR-BIL-011 — Multa por atraso é fixa e única por fatura

ASSESS com day>due_day,outstanding>0 e ainda não aplicada posta1000. Repetição com outra chave retorna NOOP; PREMIUM não isenta. Snapshot emitido permanece igual.

**Observação executada:** `billing.assess_late_fee`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-011` e `SC-BR-BIL-011-NN`.

## BR-BIL-012 — Sem atraso ou saldo não há multa

No due_day a fatura não está atrasada. Saldozero impede multa mesmo depois do vencimento; não marcar a flag assessed num NOOP sem multa.

**Observação executada:** `billing.assess_late_fee`.

**Exemplos:** 2 cenários em [Gherkin](../features/bil.feature), tags `BR-BIL-012` e `SC-BR-BIL-012-NN`.
