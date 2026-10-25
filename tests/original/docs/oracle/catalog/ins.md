# Parcelamento

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-INS-001 — Parcelamento tem quantidade limitada

n inteiro em [1,12]. Valores fora do intervalo produzem INSTALLMENTS no gate de domínio, sem corrigir para o limite mais próximo.

**Observação executada:** `installment.admission`.

**Exemplos:** 4 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-001` e `SC-BR-INS-001-NN`.

## BR-INS-002 — Compra em país externo admite no máximo três parcelas

country!=BR implica n<=3, ainda que currency=BRL. País base permite até 12 se outros requisitos passarem.

**Observação executada:** `installment.admission`.

**Exemplos:** 3 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-002` e `SC-BR-INS-002-NN`.

## BR-INS-003 — Principal mínimo por parcela

Para n>1, floor(P/n)>=500. n=1 permite principal positivo abaixo de 500. Tarifa não pode completar o mínimo.

**Observação executada:** `installment.admission`.

**Exemplos:** 3 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-003` e `SC-BR-INS-003-NN`.

## BR-INS-004 — Resto do principal vai às primeiras parcelas

Dividir D emn partes: q=D//n, r=D%n, parte i=q+1 para i<r e q para as de mais, com índice começando em 0.

**Observação executada:** `installment.split`.

**Exemplos:** 2 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-004` e `SC-BR-INS-004-NN`.

## BR-INS-005 — Tarifa é dividida independentemente do principal

Aplicar q/r à tarifa da captura separadamente; não calcular tarifa percentual por parcela. Zero pode ocorrer em parcela de tarifa.

**Observação executada:** `installment.split_components`.

**Exemplos:** 2 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-005` e `SC-BR-INS-005-NN`.

## BR-INS-006 — Agenda começa no ciclo da captura

Parcela i pertence a cycle(capture_day)+i; due_day=(cycle_i+1)*30+10. Autorização anterior não ancora a agenda.

**Observação executada:** `installment.schedule`.

**Exemplos:** 2 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-006` e `SC-BR-INS-006-NN`.

## BR-INS-007 — Parcelas conservam componentes exatos

Somatórios de parcelas de principal e tarifa devem igualar os respectivos totais da captura. Não criar centavo de diferença contábil.

**Observação executada:** `installment.conservation`.

**Exemplos:** 2 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-007` e `SC-BR-INS-007-NN`.

## BR-INS-008 — Captura parcial parcelada também respeita mínimo

Para autorização com n>1, cada captura d gera n lotes e exige floor(d/n)>=500; não usar P original para aprovar captura pequena.

**Observação executada:** `installment.capture_admission`.

**Exemplos:** 2 cenários em [Gherkin](../features/ins.feature), tags `BR-INS-008` e `SC-BR-INS-008-NN`.
