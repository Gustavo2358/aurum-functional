# Captura

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-CAP-001 — Captura exige autorização aberta e não expirada

Somente ACTIVE/PARTIAL com now<expires_at pode capturar. CAPTURED/CANCELLED/EXPIRED é DECLINED/AUTH_STATE; não reabrir reserva.

**Observação executada:** `capture.admission`.

**Exemplos:** 3 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-001` e `SC-BR-CAP-001-NN`.

## BR-CAP-002 — Captura respeita principal remanescente

d>0 e c+d<=P. Zero é entrada inválida; exceder saldo é DECLINED/CAPTURE_AMOUNT. Não limitar silenciosamente o montante solicitado.

**Observação executada:** `capture.amount`.

**Exemplos:** 3 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-002` e `SC-BR-CAP-002-NN`.

## BR-CAP-003 — Tarifa é alocada cumulativamente

Para F da autorização, P original e c capturado, tarifa da nova captura=floor(F*(c+d)/P)-floor(F*c/P). O último fragmento absorve resíduo exato.

**Observação executada:** `capture.fee_delta`.

**Exemplos:** 3 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-003` e `SC-BR-CAP-003-NN`.

## BR-CAP-004 — Estado depende do total capturado

Após captura com 0<c<P, state=PARTIAL; quando c=P, state=CAPTURED e reserva restante zero.

**Observação executada:** `capture.next_state`.

**Exemplos:** 2 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-004` e `SC-BR-CAP-004-NN`.

## BR-CAP-005 — Captura troca reserva por dívida

Capturar d e tarifa delta reduz held por d+delta e aumenta debt pelo mesmo total. Sem outros eventos, disponibilidade de crédito não muda.

**Observação executada:** `capture.transition`.

**Exemplos:** 2 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-005` e `SC-BR-CAP-005-NN`.

## BR-CAP-006 — Captura tem contrapartidas contábeis

Principal capturado credita MERCHANT_CLEARING, tarifa credita FEE_REVENUE e os recebíveis correspondentes são debitados; também baixar reserva. Cada evento financeiro fecha em zero.

**Observação executada:** `capture.ledger`.

**Exemplos:** 2 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-006` e `SC-BR-CAP-006-NN`.

## BR-CAP-007 — Lotes e recompensa fazem parte do mesmo commit

Falha ao preparar lotes, diário ou recompensa cancela toda a captura. Não publicar recebível sem parcela/pontos nem consumir idempotência.

**Observação executada:** `capture.capacity`.

**Exemplos:** 2 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-007` e `SC-BR-CAP-007-NN`.

## BR-CAP-008 — Captura não reexecuta risco da autorização

Uma autorização aberta é um compromisso. Captura válida não deve ser recusada porque a janela de risco agora está alta; os gates de valor/estado/parcelas continuam obrigatórios.

**Observação executada:** `capture.no_reauthorization`.

**Exemplos:** 2 cenários em [Gherkin](../features/cap.feature), tags `BR-CAP-008` e `SC-BR-CAP-008-NN`.
