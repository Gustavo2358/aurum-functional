# Contrato dos probes de teste

Os aliases abaixo especificam observações, não implementações alternativas. O driver pode ser fino e agrupar aliases que chamam a mesma função C. Não obrigamos o core a exportar funções com esses nomes; o binding registra o símbolo real. Proibido selecionar saída pelo ID do cenário.

## Tipos e defaults

DataTables contêm literais JSON. `value`, `amount`, `principal`, `fee`, `debt`, `held`, `used`, `available`, `payment`, `refunded`, `total` são centavos, salvo nomes explicitamente de contagem/score. `n` é alias de installments; `day` é dia lógico e deriva/define `now=day*1440+600` quando necessário. `now` explícito vence esse default. `tier` é alias de account.tier; `base_score` de account.base_score. `country`, `currency`, `card_present`, `pin_ok`, `channel` são campos de pedido. `principal` em probe de cálculo significa **P já convertido**, não uma entrada que deve passar novamente pelo FX.

Defaults de probes puros: `principal=10000`, `fee=0`, `n=installments=1`, `tier=REGULAR`, `category=NORMAL`, `country=BR`, `currency=BRL`, `card_present=true`, `pin_ok=true`, `base_score=10`, `recent_count=0`. Campos estruturais omitidos mantêm o perfil base. Um array explicitamente passado substitui o histórico correspondente. Não inferir dívida a partir do resultado esperado.

Campos `fail_at`, `cached_debt`, `cached_held` e modos de erro são controles de fault injection do harness **depois** de criar preestado válido. Não são parâmetros públicos da CLI nem políticas de produção. Probes de idempotência com estado anterior/posterior divergente testam o subsistema de resposta memorizada: não autorizam adicionar uma operação pública que altere taxas em runtime.

## Observações por família

| Família / aliases | Preparação e chamada real | Observações |
|---|---|---|
| `money.*` | Valores escalares; chamar operadores de produção | value, ok, error; limites monetários e int64 são distintos |
| `pricing.quote` | Pedido e configuração; cotação real | ok, reason, principal e fee |
| `pricing.components` | P já convertido e flags; fórmula de componentes | international_fee, installment_fee, fee; não executa admissão de crédito/parcelas |
| `pricing.purity` | Cotar repetidamente engine real | snapshots financeiros e respostas |
| `eligibility.check` | Campos de entidade/pedido, faturas quando presentes | eligible e primeira razão; não roda limites/risco |
| `limits.available/credit/operation/daily_amount` | Escalares de projeção calculada/limites | available ou allowed/reason |
| `limits.daily_count/day_projection/record_decision` | Históricos ou contagens tipadas, conforme caso | contagem e consumo calculados pelo core |
| `limits.after_reversal` | Aprovação e reversão reais; REFUND requer captura prévia | projeção do dia original da autorização |
| `risk.score/classify/velocity` | Componentes ou histórico anterior; classificador real | score/decision/recent_count/component |
| `auth.decide/approve/quote_consistency` | Engine e pedido; AUTHORIZE real, com QUOTE quando indicado | resposta, reservas, saldo, dívida, estado, contador |
| `auth.capacity/capture.capacity/io.capacity` | Preestado válido e falha controlada antes do commit | resposta e comparação total de estado |
| `idempotency.replay/conflict/capture_replay` | Processar comandos reais, depois repetir/alterar payload | cache, respostas, efeitos de domínio |
| `idempotency.canonical/namespace/error_policy` | Rotinas reais de canonicalização/chave/política | same_payload, same_key, cache_response |
| `idempotency.declined_replay` | Memorizar primeira resposta real; lookup com mesmo payload após variar contexto no teste de subsistema | resposta original, sem redecidir |
| `idempotency.query` | Consultas sem chave | delta de registros e snapshots |
| `capture.admission/amount/fee_delta/next_state` | Dados de autorização validáveis; auxiliares reais | decisão de gate, alocação e próximo estado |
| `capture.transition/ledger` | Estado aberto e histórico c, se houver; CAPTURE real | held/debt/deltas de diário; available_delta compara snapshots |
| `capture.no_reauthorization` | Autorização aberta com preços já fixados; contexto atual de risco diferente | CAPTURE continua válida sem chamar classificador de autorização |
| `reversal.cancel/cancel_terminal/expire` | Autorizações/capturas coerentes; cancelar ou TICK real | reserva, estado, dívida e ausência de duplicação |
| `reversal.refund_amount/refund_fee` | D,G,r,d explícitos | gate e alocação cumulativa de tarifa |
| `reversal.refund_allocation` | Lotes ordenados do mesmo capture, sem pagamentos, ciclos sucessivos | principal restante por índice e cash_refund |
| `reversal.refund_paid` | Captura D/G, pagos derivados de original menos unpaid; reembolso real | dívida da captura após e dinheiro devolvido |
| `reversal.refund_closed_auth` | Captura d sem tarifa em autorização P maior, com resto cancelado/expirado | estado da autorização preservado e reembolso permitido |
| `reversal.refund_with_late_fee` | Captura com principal/tarifa ainda devidos e multa de fatura | componentes da dívida pós-reembolso |
| `installment.admission/capture_admission` | P/d, n, country | admissão e razão; sem verificar crédito |
| `installment.split/split_components/conservation` | Total/is componentes e n | arrays/somas calculados pelo C |
| `installment.schedule` | capture_day e n | ciclos e due_days |
| `billing.close_time/close_order/due_day/minimum_due` | Escalares e controle de fechamento | gates e fórmulas reais |
| `billing.issue/empty` | Lotes do ciclo e próximos; cancelamentos explícitos | emissão e snapshot, nenhum pagamento prévio implícito |
| `billing.allocate_payment/pay_invoiced/partial_payment` | Recebíveis faturados e futuros válidos, prioridades do contrato | componentes e snapshots antes/depois |
| `billing.payment_admission` | Saldo elegível e payment | admissão antes de mutar |
| `billing.assess_late_fee` | Fatura, saldo, flag assessed e dia | decisão, delta e flag após |
| `rewards.raw/cap/cycle_cap/refund/after_refund/capture_partition` | Valores, concessões anteriores, r/d; respectivas rotinas C | concessão ou reversão e saldo; componentes puros não fingem autorização alcançável |
| `batch.order/continue/replay/account_isolation` | Listas de autorizações reais; IDs ausentes gerados pelo harness em sequência determinística | decisões e projeções; duas contas independentes têm cartões próprios |
| `batch.reconcile/balance_check/report_order` | Estado/diário real, projeção corrompida só no teste negativo | reconciliação, equilíbrio por evento, ordenação de consulta |
| `batch.tick` | Instantes, reservas e eventual falha preparada | TICK real e snapshots |
| `io.*` restantes | Binário/parser de produção; stream/seed/ambiente preparados | exit, JSON, comprimento, recuperação de linha e validação |
| `flow.run` | Seed perfil base com overrides; **todas** as linhas de commands passam pelo caminho público real | projeções finais e respostas indexadas |

## Estados sintéticos e alcançabilidade

Um teste de fórmula pode usar F=3 ou n/P que não seria aprovado pelo perfil comercial base. Isso é válido para testar a função parametrizada, mas não prova que tal combinação é alcançável por AUTHORIZE naquele perfil. Registrar classe UNIT/COMPONENT/E2E; nunca misturar contagens. Cenários `flow.run` usam somente seed válido, configurações admitidas e comandos públicos, servindo de contrapeso às preparações diretas de estado.

Regra defensiva de categoria RESTRICTED em pontos é uma unidade funcional separada, não regra alcançável de autorização normal. Casos de cache corrompido são fault injection, não estados financeiros válidos que a aplicação deveria produzir.

## Asserções derivadas do avaliador

`same_response`, `same_quote`, `financial_unchanged`, `unchanged`, `balanced`, `reconciled`, `identical_bytes`, `identical_results` devem ser conferidos no harness a partir de valores/fontes reais. Não aceitar um booleano autodeclarado pela aplicação como único teste. `decision_sequence` em flow inclui cada linha em commands. Caminhos `auths.<id>.state` e `invoices.<id>.*` são consultas/projeções de produção. `daily_*` em flow é agregado do dia final corrente.
