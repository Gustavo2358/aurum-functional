# Idempotência

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-IDM-001 — Replay exato devolve a resposta original

Mesma operação, request_id e payload canônico devolvem a resposta armazenada exatamente, sem novo processamento financeiro.

**Observação executada:** `idempotency.replay`.

**Exemplos:** 2 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-001` e `SC-BR-IDM-001-NN`.

## BR-IDM-002 — Chave repetida com outra intenção é conflito

Alterar qualquer campo semântico do payload para uma chave existente retorna IDEMPOTENCY_CONFLICT e preserva o primeiro resultado/estado.

**Observação executada:** `idempotency.conflict`.

**Exemplos:** 2 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-002` e `SC-BR-IDM-002-NN`.

## BR-IDM-003 — Canonicalização elimina diferenças só de escrita

Ordem de chaves e zeros à esquerda não mudam o payload depois de parse/validação. Campos default omitidos e seus valores explícitos são equivalentes.

**Observação executada:** `idempotency.canonical`.

**Exemplos:** 2 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-003` e `SC-BR-IDM-003-NN`.

## BR-IDM-004 — Namespace de chave inclui o tipo de operação

A chave é(op,request_id), não apenas request_id. Mesmo identificador textual pode ser usado em AUTHORIZE e CAPTURE; referências têm o tipo da entidade.

**Observação executada:** `idempotency.namespace`.

**Exemplos:** 2 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-004` e `SC-BR-IDM-004-NN`.

## BR-IDM-005 — Erros técnicos e de envelope não consomem chave

INVALID_INPUT,NOT_FOUND,OWNERSHIP,CAPACITY e IDEMPOTENCY_CONFLICT não criam nova entrada no cache. Uma tentativa corrigida pode reutilizar a chave que nunca foi registrada.

**Observação executada:** `idempotency.error_policy`.

**Exemplos:** 3 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-005` e `SC-BR-IDM-005-NN`.

## BR-IDM-006 — Rejeições válidas também são memorizadas

Uma requisição estruturalmente válida recusada por regra de domínio fica registrada; repetir com a mesma chave retorna a mesma decisão, não tenta aproveitar mudança posterior de saldo/tempo.

**Observação executada:** `idempotency.declined_replay`.

**Exemplos:** 2 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-006` e `SC-BR-IDM-006-NN`.

## BR-IDM-007 — Replay não duplica efeitos indiretos

Repetição não duplica contagem de risco, consumo diário, pontos, lotes ou lançamentos. Verificar o conjunto de efeitos, não apenas o saldo principal.

**Observação executada:** `idempotency.capture_replay`.

**Exemplos:** 2 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-007` e `SC-BR-IDM-007-NN`.

## BR-IDM-008 — Consulta não precisa de chave de mutação

QUOTE,GET e RECONCILE não consomem idempotência. Repeti-los não altera a capacidade disponível para chaves financeiras.

**Observação executada:** `idempotency.query`.

**Exemplos:** 2 cenários em [Gherkin](../features/idm.feature), tags `BR-IDM-008` e `SC-BR-IDM-008-NN`.
