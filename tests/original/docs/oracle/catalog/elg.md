# Elegibilidade

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-ELG-001 — Conta inativa impede autorização

No gate de elegibilidade, conta inativa impede autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

**Observação executada:** `eligibility.check`.

**Exemplos:** 2 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-001` e `SC-BR-ELG-001-NN`.

## BR-ELG-002 — Cartão bloqueado impede autorização

No gate de elegibilidade, cartão bloqueado impede autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

**Observação executada:** `eligibility.check`.

**Exemplos:** 2 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-002` e `SC-BR-ELG-002-NN`.

## BR-ELG-003 — Validade do cartão inclui o último dia

Cartão é válido quando day<=expiry_day. Expiração de cartão não se confunde com TTL da autorização.

**Observação executada:** `eligibility.check`.

**Exemplos:** 2 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-003` e `SC-BR-ELG-003-NN`.

## BR-ELG-004 — Lojista bloqueado impede autorização

No gate de elegibilidade, lojista bloqueado impede autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

**Observação executada:** `eligibility.check`.

**Exemplos:** 2 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-004` e `SC-BR-ELG-004-NN`.

## BR-ELG-005 — Categoria restrita impede nova autorização

No gate de elegibilidade, categoria restrita impede nova autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

**Observação executada:** `eligibility.check`.

**Exemplos:** 2 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-005` e `SC-BR-ELG-005-NN`.

## BR-ELG-006 — Permissão internacional depende do país

Se country!=BR e card.allow_international=false, rejeitar. Moeda USD em country=BR não aciona essa restrição de país.

**Observação executada:** `eligibility.check`.

**Exemplos:** 2 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-006` e `SC-BR-ELG-006-NN`.

## BR-ELG-007 — Permissão de aproximação é específica do canal

channel=CONTACTLESS exige card.allow_contactless=true. Proibição não bloqueia POS normal.

**Observação executada:** `eligibility.check`.

**Exemplos:** 2 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-007` e `SC-BR-ELG-007-NN`.

## BR-ELG-008 — PIN exigido acima do limite presencial

Quando card_present=true e amount nominal>10000, pin_ok deve ser true. O limiar é deliberadamente nominal na moeda de entrada, antes da cotação; não usar P convertido. Quando card_present=false, esta guarda de PIN presencial não se aplica.

**Observação executada:** `eligibility.check`.

**Exemplos:** 3 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-008` e `SC-BR-ELG-008-NN`.

## BR-ELG-009 — Saldo vencido impede nova autorização

Qualquer fatura da conta com day>due_day e outstanding>0 causa PAST_DUE. No dia do vencimento ainda não há atraso; pagamento mínimo não elimina saldo vencido.

**Observação executada:** `eligibility.check`.

**Exemplos:** 3 cenários em [Gherkin](../features/elg.feature), tags `BR-ELG-009` e `SC-BR-ELG-009-NN`.
