# Cotação e tarifas

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-PRC-001 — Moeda base não altera principal

BRL usa fator10000; o principal cotado é exatamente o montante solicitado, sem reconversão adicional.

**Observação executada:** `pricing.quote`.

**Exemplos:** 2 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-001` e `SC-BR-PRC-001-NN`.

## BR-PRC-002 — Cotação estrangeira usa taxa explícita

USD usa50000 e EUR60000 no perfil base. A conversão produz principal em BRL com half-up; taxas são fixas sintéticas.

**Observação executada:** `pricing.quote`.

**Exemplos:** 2 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-002` e `SC-BR-PRC-002-NN`.

## BR-PRC-003 — Moeda sem taxa não recebe fallback

Uma moeda sem taxa não pode ser tratada como BRL nem inferida de country. A cotação falha com UNSUPPORTED_CURRENCY; AUTHORIZE traduz isso para DECLINED.

**Observação executada:** `pricing.quote`.

**Exemplos:** 2 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-003` e `SC-BR-PRC-003-NN`.

## BR-PRC-004 — Tarifa cambial de cliente regular

REGULAR em currency!=BRL paga floor(P*200/10000) na componente internacional. O país não ativa essa tarifa sozinho.

**Observação executada:** `pricing.components`.

**Exemplos:** 2 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-004` e `SC-BR-PRC-004-NN`.

## BR-PRC-005 — Isenção premium é específica

PREMIUM zera somente international_fee. Não zera installment_fee, principal nem multa.

**Observação executada:** `pricing.components`.

**Exemplos:** 2 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-005` e `SC-BR-PRC-005-NN`.

## BR-PRC-006 — Tarifa por parcelas adicionais

installment_fee=floor(P*50*(n-1)/10000), em cálculo único. Não arredondar separadamente cada parcela adicional.

**Observação executada:** `pricing.components`.

**Exemplos:** 3 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-006` e `SC-BR-PRC-006-NN`.

## BR-PRC-007 — Teto incide depois da soma de componentes

F=min(5000, international_fee+installment_fee). As componentes calculadas continuam observáveis, sem aplicar teto em cada uma.

**Observação executada:** `pricing.components`.

**Exemplos:** 2 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-007` e `SC-BR-PRC-007-NN`.

## BR-PRC-008 — Cotação não compromete a conta

QUOTE produz preço, mas não reserva limite, cria dívida, soma consumo diário ou concede pontos. Consultas repetidas no mesmo estado/configuração são iguais.

**Observação executada:** `pricing.purity`.

**Exemplos:** 2 cenários em [Gherkin](../features/prc.feature), tags `BR-PRC-008` e `SC-BR-PRC-008-NN`.
