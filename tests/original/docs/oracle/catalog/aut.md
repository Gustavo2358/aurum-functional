# Autorização

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-AUT-001 — Primeira causa determina a rejeição

Seguir a precedência de OPERATIONS.md. Conta inativa vence insuficiência de crédito; limite de crédito vence score alto. Não informar a última guarda avaliada.

**Observação executada:** `auth.decide`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-001` e `SC-BR-AUT-001-NN`.

## BR-AUT-002 — Aprovação cria reserva total

Aprovação cria uma autorização e reserva P+F, com state=ACTIVE e capturado zero. Reserva deve aparecer no crédito disponível.

**Observação executada:** `auth.approve`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-002` e `SC-BR-AUT-002-NN`.

## BR-AUT-003 — Autorização sozinha não cria recebível nem pontos

Em APPROVED, debt e pontos permanecem inalterados; a dívida e recompensa surgem na captura.

**Observação executada:** `auth.approve`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-003` e `SC-BR-AUT-003-NN`.

## BR-AUT-004 — Rejeição não produz efeitos financeiros

DECLINED não altera reserva, dívida, diário financeiro, pontos nem contadores de valor/quantidade. Pode persistir resposta de idempotência e log operacional.

**Observação executada:** `auth.decide`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-004` e `SC-BR-AUT-004-NN`.

## BR-AUT-005 — Review é terminal sem reserva

REVIEW não cria autorização nem dívida/hold/pontos; soma1 à quantidade diária e à velocidade. Não é aprovação condicional financeira.

**Observação executada:** `auth.decide`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-005` e `SC-BR-AUT-005-NN`.

## BR-AUT-006 — Prazo de reserva deriva do instante explícito

expires_at=approved_at+1440 minutos. O relógio de parede não participa e a mesma política vale na virada do dia.

**Observação executada:** `auth.approve`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-006` e `SC-BR-AUT-006-NN`.

## BR-AUT-007 — Aprovação usa a mesma cotação observável

No mesmo estado/configuração, P e F aprovados devem corresponder ao QUOTE da mesma entrada; armazenar os valores para a captura cumulativa, sem recotação.

**Observação executada:** `auth.quote_consistency`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-007` e `SC-BR-AUT-007-NN`.

## BR-AUT-008 — Preflight torna a aprovação indivisível

Se não houver capacidade para autorização, diário ou idempotência, retornar ERROR/CAPACITY sem reserva, contador, chave ou resultado parcialmente publicado. Reserva de memória ocorre antes do commit.

**Observação executada:** `auth.capacity`.

**Exemplos:** 2 cenários em [Gherkin](../features/aut.feature), tags `BR-AUT-008` e `SC-BR-AUT-008-NN`.
