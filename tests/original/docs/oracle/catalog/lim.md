# Limites e consumo

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-LIM-001 — Disponibilidade considera dívida e reservas

available=max(0,credit_limit-debt-held); parcelas futuras fazem parte de debt e limite reduzido não pode gerar saldo disponível negativo.

**Observação executada:** `limits.available`.

**Exemplos:** 2 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-001` e `SC-BR-LIM-001-NN`.

## BR-LIM-002 — Crédito cobre principal mais tarifa

A autorização só cabe no crédito quando P+F<=available. Igualdade é permitida; validar só P é erro.

**Observação executada:** `limits.credit`.

**Exemplos:** 2 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-002` e `SC-BR-LIM-002-NN`.

## BR-LIM-003 — Limite por operação usa principal convertido

P<=per_operation_limit; taxa não entra nessa comparação, mas a moeda precisa já estar convertida. Igualdade passa.

**Observação executada:** `limits.operation`.

**Exemplos:** 2 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-003` e `SC-BR-LIM-003-NN`.

## BR-LIM-004 — Limite diário usa consumo bruto de principal

daily_gross_principal+P<=daily_limit. Tarifa não consome o limite diário de principal.

**Observação executada:** `limits.daily_amount`.

**Exemplos:** 2 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-004` e `SC-BR-LIM-004-NN`.

## BR-LIM-005 — Limite diário de quantidade verifica histórico anterior

Se o número anterior de APPROVED/REVIEW no dia for menor que10, a nova tentativa pode passar esse gate; com 10 já existentes, é recusada. DECLINED não entra.

**Observação executada:** `limits.daily_count`.

**Exemplos:** 2 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-005` e `SC-BR-LIM-005-NN`.

## BR-LIM-006 — Consumo é separado por dia lógico

Agregações diárias selecionam somente eventos APPROVED/REVIEW do day corrente. Registros de outro dia não desaparecem, apenas ficam fora do agregado.

**Observação executada:** `limits.day_projection`.

**Exemplos:** 2 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-006` e `SC-BR-LIM-006-NN`.

## BR-LIM-007 — Reversões não restauram consumo bruto diário

Cancelar, expirar ou reembolsar não subtrai principal do agregado diário de autorizações aprovadas. Limite de crédito é liberado por regras próprias, não pelo contador diário.

**Observação executada:** `limits.after_reversal`.

**Exemplos:** 2 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-007` e `SC-BR-LIM-007-NN`.

## BR-LIM-008 — Review consome quantidade mas não principal diário

Uma decisão REVIEW adiciona1 ao contador diário e zero ao principal bruto; DECLINED adiciona zero a ambos.

**Observação executada:** `limits.record_decision`.

**Exemplos:** 3 cenários em [Gherkin](../features/lim.feature), tags `BR-LIM-008` e `SC-BR-LIM-008-NN`.
