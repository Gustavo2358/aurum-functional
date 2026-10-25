# Recompensas

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-REW-001 — Cliente regular ganha pontos por principal capturado

REGULAR,NORMAL recebe floor(D/10000) pontos antes do cap. Tarifa não gera pontos; autorização sem captura não gera pontos.

**Observação executada:** `rewards.raw`.

**Exemplos:** 2 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-001` e `SC-BR-REW-001-NN`.

## BR-REW-002 — Premium multiplica depois do truncamento

PREMIUM,NORMAL recebe2*floor(D/10000), não floor(2*D/10000). Essa diferença em frações é intencional.

**Observação executada:** `rewards.raw`.

**Exemplos:** 2 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-002` e `SC-BR-REW-002-NN`.

## BR-REW-003 — Categoria restrita não pontua

Uma captura com categoria de política RESTRICTED produz zero pontos. A autorização normal já a rejeita; esta regra defensiva tem escopo de cálculo/estados históricos válidos e não afirma alcance no fluxo normal sem bypass.

**Observação executada:** `rewards.raw`.

**Exemplos:** 2 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-003` e `SC-BR-REW-003-NN`.

## BR-REW-004 — Cap limita pontos efetivamente concedidos

granted=min(raw_points,max(0,1000-gross_granted_in_cycle)). Não conceder valor negativo nem aplicar cap ao principal monetário.

**Observação executada:** `rewards.cap`.

**Exemplos:** 3 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-004` e `SC-BR-REW-004-NN`.

## BR-REW-005 — Cap usa ciclo da captura

Gross concedido em ciclo anterior não reduz cap do ciclo atual. A autorização pode ter sido feita antes; período é da captura.

**Observação executada:** `rewards.cycle_cap`.

**Exemplos:** 2 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-005` e `SC-BR-REW-005-NN`.

## BR-REW-006 — Reversão proporcional usa concessão real

Pontos a reverter=floor(granted*(r+d)/D)-floor(granted*r/D). Usar granted pós-cap, não raw; reembolso total reverte todos os concedidos.

**Observação executada:** `rewards.refund`.

**Exemplos:** 3 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-006` e `SC-BR-REW-006-NN`.

## BR-REW-007 — Reembolso não libera cap bruto

Reverter pontos reduz saldo líquido, mas gross_granted_in_cycle permanece. Nova captura no mesmo ciclo não readquire o espaço devolvido.

**Observação executada:** `rewards.after_refund`.

**Exemplos:** 2 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-007` e `SC-BR-REW-007-NN`.

## BR-REW-008 — Arredondamento é por captura, não pela autorização

Duas capturas menores podem somar menos pontos que uma captura única, pois floor é aplicado em cada D. Não impor metamorfismo falso de invariância por divisão.

**Observação executada:** `rewards.capture_partition`.

**Exemplos:** 2 cenários em [Gherkin](../features/rew.feature), tags `BR-REW-008` e `SC-BR-REW-008-NN`.
