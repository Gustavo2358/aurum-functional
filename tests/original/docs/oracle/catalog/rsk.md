# Risco determinístico

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-RSK-001 — Score começa no score-base válido

base_score em [0,100] participa do somatório; não substituir por probabilidade ou taxa externa. O resultado mínimo, sem componentes adicionais, é o score-base.

**Observação executada:** `risk.score`.

**Exemplos:** 2 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-001` e `SC-BR-RSK-001-NN`.

## BR-RSK-002 — País internacional acrescenta quinze pontos

country!=BR acrescenta15 ao score; moeda estrangeira sozinha não acrescenta esse componente.

**Observação executada:** `risk.score`.

**Exemplos:** 2 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-002` e `SC-BR-RSK-002-NN`.

## BR-RSK-003 — Cartão ausente acrescenta vinte pontos

card_present=false soma20, independentemente do canal nominal. Não inferir presença do enum do canal.

**Observação executada:** `risk.score`.

**Exemplos:** 2 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-003` e `SC-BR-RSK-003-NN`.

## BR-RSK-004 — Montante alto inclui a fronteira

P>=100000 acrescenta15; P=99999 não. Usar principal convertido, não amount nominal.

**Observação executada:** `risk.score`.

**Exemplos:** 2 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-004` e `SC-BR-RSK-004-NN`.

## BR-RSK-005 — Velocidade conta uma janela inclusiva de eventos anteriores

Contar APPROVED/REVIEW já persistidos com now-60<=minute<=now; excluir DECLINED. Três ou mais acrescentam20. A tentativa corrente não entra antes do resultado.

**Observação executada:** `risk.velocity`.

**Exemplos:** 2 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-005` e `SC-BR-RSK-005-NN`.

## BR-RSK-006 — Faixas de decisão não se sobrepõem

score<50→APPROVED; 50<=score<80→REVIEW; score>=80→DECLINED. Essas saídas classificam risco, não pulam os gates de autorização.

**Observação executada:** `risk.classify`.

**Exemplos:** 4 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-006` e `SC-BR-RSK-006-NN`.

## BR-RSK-007 — Somatório é saturado em cem

Somar todos os componentes e aplicar min(100,total); não saturar por wraparound nem permitir score>100.

**Observação executada:** `risk.score`.

**Exemplos:** 2 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-007` e `SC-BR-RSK-007-NN`.

## BR-RSK-008 — Componentes se acumulam mesmo quando nenhum basta

Não usar else-if entre componentes independentes. Internacional+remoto+alto valor com base10 produz 60, enquanto internacional+remoto abaixo do limite produz 45.

**Observação executada:** `risk.score`.

**Exemplos:** 2 cenários em [Gherkin](../features/rsk.feature), tags `BR-RSK-008` e `SC-BR-RSK-008-NN`.
