# Batch diário e reconciliação

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-BAT-001 — Batch preserva ordem de comandos

Executar na ordem recebida. Autorização que consumiu crédito afeta a seguinte; ordenar por request_id mudaria o comportamento e é proibido.

**Observação executada:** `batch.order`.

**Exemplos:** 2 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-001` e `SC-BR-BAT-001-NN`.

## BR-BAT-002 — Rejeição local não interrompe nem desfaz batch

Cada comando é atômico, o batch não. Uma rejeição financeira no meio preserva o anterior e permite processar o posterior.

**Observação executada:** `batch.continue`.

**Exemplos:** 2 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-002` e `SC-BR-BAT-002-NN`.

## BR-BAT-003 — Replay em lote não conta como nova operação financeira

Repetir request_id/payload em posições diferentes retorna resposta original e não consome saldo adicional. A ordem das respostas ainda inclui o replay.

**Observação executada:** `batch.replay`.

**Exemplos:** 2 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-003` e `SC-BR-BAT-003-NN`.

## BR-BAT-004 — Contas independentes têm projeções isoladas

Operação de A1 não altera dívida/reserva/pontos de A2. Agregações de risco, limite diário e pagamento são por conta, nunca globais.

**Observação executada:** `batch.account_isolation`.

**Exemplos:** 2 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-004` e `SC-BR-BAT-004-NN`.

## BR-BAT-005 — Reconciliação soma recebíveis e reservas reais

RECONCILE deriva debt de todos os componentes de lotes ainda devidos e held de autorizações abertas; confere com projeções. Divergência é erro, não ajustada silenciosamente.

**Observação executada:** `batch.reconcile`.

**Exemplos:** 2 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-005` e `SC-BR-BAT-005-NN`.

## BR-BAT-006 — Todo evento financeiro é balanceado

Soma algébrica das linhas de cada event_id deve ser zero. Soma global zero não basta para esconder dois eventos individualmente quebrados.

**Observação executada:** `batch.balance_check`.

**Exemplos:** 2 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-006` e `SC-BR-BAT-006-NN`.

## BR-BAT-007 — Relatório canônico ordena entidades e lotes

Resultados de consulta usam ordem ASCII de IDs e ordem normativa de lotes; não expor ordem de hash/endereços. Isso não ordena a execução do batch.

**Observação executada:** `batch.report_order`.

**Exemplos:** 2 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-007` e `SC-BR-BAT-007-NN`.

## BR-BAT-008 — TICK é monótono e atomicamente expira todas as reservas

TICK para trás éINVALID_TIME. Mesmo instante não repete expirações. Falha de capacidade não altera nem relógio nem parte das reservas.

**Observação executada:** `batch.tick`.

**Exemplos:** 3 cenários em [Gherkin](../features/bat.feature), tags `BR-BAT-008` e `SC-BR-BAT-008-NN`.
