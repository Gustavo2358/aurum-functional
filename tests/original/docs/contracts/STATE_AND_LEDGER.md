# Estado, lotes e diário

## Fonte e projeções

Escolha diário+projeções ou estruturas normalizadas equivalentes, mas deve haver um diário financeiro verificável e fontes únicas para dívida/reserva. É proibido alterar acumuladores desconectados do registro que permite reconciliá-los. Contadores derivados podem ser cacheados se reconciliados.

Cada captura cria n lotes; cada lote identifica conta, captura, parcela, ciclo de vencimento, principal original, tarifa original e valores pagos/cancelados por componente. Multa identifica fatura, não captura. Valores pagos/cancelados nunca excedem valores devidos. `debt` inclui principal/tarifa futuro; `invoiced_outstanding` é a parte já faturada mais multas. `cash_refund_total` registra excedente do reembolso após abater o devido daquela captura.

Autorizações com reserva restante positiva são ACTIVE ou PARTIAL. c=0 → ACTIVE; 0<c<P → PARTIAL; c=P → CAPTURED. CANCELLED/EXPIRED podem ter captura parcial histórica, mas reserva zero. Estado terminal não implica captura reembolsada.

## Diário de partidas balanceadas, em centavos

Débito é positivo; crédito, negativo. Toda operação financeira usa um `event_id` e soma zero. Estes nomes são contas contábeis **sintéticas**, não plano contábil de instituição real.

| Evento | Débitos | Créditos |
|---|---|---|
| Abrir reserva | `HOLD_ASSET` P+F | `HOLD_OFFSET` P+F |
| Liberar reserva | `HOLD_OFFSET` liberado | `HOLD_ASSET` liberado |
| Capturar principal | `RECEIVABLE_PRINCIPAL` d | `MERCHANT_CLEARING` d |
| Capturar tarifa | `RECEIVABLE_FEE` delta | `FEE_REVENUE` delta |
| Multa | `RECEIVABLE_LATE` 1000 | `LATE_REVENUE` 1000 |
| Pagar | `CASH` recebido | recebíveis alocados por componente |
| Reembolsar principal | `MERCHANT_CLEARING` principal devolvido | recebível cancelado + `CASH` parcela já paga |
| Reembolsar tarifa | `FEE_REVENUE` tarifa devolvida | recebível cancelado + `CASH` parcela já paga |

Uma captura contém também a baixa correspondente de reserva. Um reembolso de valor totalmente pago tem crédito em CASH sem tornar recebível negativo. Não existe arredondamento residual escondido no diário. Linhas de valor zero podem ser omitidas; o formato escolhido é estável. Logs técnicos não pertencem a esse diário e não entram na soma.

Histórico inicial de testes pode usar lançamentos `OPENING` balanceados contra `OPENING_OFFSET`; o seed público usual inicia sem histórico financeiro. Um estado de teste inválido deve falhar no setup, não gerar uma expectativa falsa.

## Recompensas

Pontos são inteiros, não centavos. `raw_points=floor(capture_principal/10000)*multiplier`, multiplier=1 REGULAR e 2 PREMIUM, salvo categoria RESTRICTED (zero). `granted=min(raw_points,max(0,1000-gross_granted_in_cycle))`. Tier e categoria foram fixados na autorização; período de cap é o da captura.

Reembolso devolve pontos por proporção cumulativa sobre `granted`, não `raw_points`. Pontos líquidos da conta podem cair até zero; não há resgate nesta fixture. O cap usa pontos brutos concedidos, portanto reembolso não libera espaço no cap. Nova captura em outro ciclo usa cap novo.

## Ordenação e transações

IDs alfanuméricos têm ordenação lexicográfica ASCII. Um batch processa ordem recebida, nunca ordena comandos. Dentro de agregações/relatórios, ordenar coleções por ID e lotes por chaves explícitas. Uma consulta não altera estado, exceto buffers temporários fora do modelo.

Cada mutação é atômica; batch não é uma transação global. Falha do segundo comando não desfaz o primeiro nem impede o terceiro, salvo impossibilidade de continuar lendo o stream. Idempotência e resposta fazem parte do mesmo commit lógico. Planejar alocações antes de alterar qualquer estrutura.
