# Operações, precedência e transações

## API de aplicação

Operações obrigatórias: QUOTE, AUTHORIZE, CAPTURE, CANCEL, REFUND, CLOSE, PAY, ASSESS_LATE_FEE, TICK, GET_ACCOUNT, GET_AUTH, GET_INVOICE e RECONCILE. Cada mutação financeira tem `request_id`; QUOTE/GET/RECONCILE são consultas. TICK é comando de relógio e sua repetição no mesmo minuto é naturalmente sem efeito adicional.

Assinaturas C e nomes internos são decisão do agente. Deve existir uma API testável independentemente da CLI. Os probes do oráculo chamam cálculos reais dessa API ou auxiliares de produção por driver; não são endpoints de um DSL nem obrigações de nomear funções com pontos.

## AUTHORIZE

1. Validar campos, tipos, intervalos de representação, enums e formato de IDs. Não antecipar limites de negócio como n=0/13: sua rejeição ocorre no gate de parcelamento. Erro → ERROR/INVALID_INPUT sem consumir chave.
2. Consultar chave `(op, request_id)`. Corpo canônico igual → resposta original idêntica, sem novo efeito. Corpo diferente → ERROR/IDEMPOTENCY_CONFLICT.
3. Resolver conta, cartão e lojista, nessa ordem. Ausente → ERROR/NOT_FOUND. Dono diferente → ERROR/OWNERSHIP. Essas falhas não consomem chave.
4. Aplicar elegibilidade na ordem: conta inativa; cartão bloqueado; cartão expirado; lojista bloqueado; categoria restrita; internacional não permitido; contactless não permitido; PIN requerido ausente; fatura vencida não paga.
5. Calcular P, componentes, F e validar parcelas. Moeda sem taxa → DECLINED/UNSUPPORTED_CURRENCY. Parcelamento inválido → DECLINED/INSTALLMENTS.
6. Limites: por operação → crédito (P+F) → valor diário → quantidade diária.
7. Risco e decisão final.
8. Verificar capacidade para tudo que será gravado, inclusive idempotência. Falha → ERROR/CAPACITY sem chave nem efeitos.
9. APPROVED: criar autorização, reservar P+F, contabilizar valor e contagem diária, registrar instante para velocidade. REVIEW: sem autorização/hold; conta para velocidade e quantidade diária. DECLINED: não conta para esses limites. Memorizar a resposta de domínio.

`primary_reason` é só a primeira causa por precedência; não alegar que outras condições seriam satisfeitas. Resultados retornam `decision`, `reason`, `principal`, `fee`, `reserved`, `auth_id` quando aplicável. Em rejeição antes de cotar, valores monetários de resposta são zero e auth_id é ausente. APPROVED retorna reason=NONE.

## CAPTURE

Entradas: request_id, auth_id, principal positivo em BRL. A validade da autorização usa o relógio já avançado por TICK. ACTIVE/PARTIAL e `now < expires_at` são exigidos; dono não é um segundo parâmetro. Somar principal capturado nunca ultrapassa P. Para n>1, cada captura precisa de `floor(d/n)>=500`; n=1 permite um centavo.

Calcular delta da tarifa por alocação cumulativa; criar captura e seus lotes; baixar hold; criar dívida; conceder pontos da captura; gravar lançamentos; atualizar estado e idempotência em um commit. Nada é feito se qualquer preflight falhar. Captura não refaz elegibilidade ou risco: conta/cartão bloqueados depois da autorização não alteram esse compromisso, desde que a autorização siga válida.

## CANCEL e TICK

CANCEL libera a reserva restante de ACTIVE/PARTIAL e muda para CANCELLED; não estorna dívida capturada. CAPTURED/CANCELLED/EXPIRED com nova chave → NOOP, sem lançamento financeiro. Repetição da mesma chave → resposta original por idempotência. Referência ausente continua ERROR/NOT_FOUND.

TICK rejeita regressão temporal. Expira ACTIVE/PARTIAL com `expires_at<=now`, liberando reserva restante e mantendo capturas/dívida/pontos. Processa expiradas em ordem de ID. Preflight cobre toda a transição do TICK; não deixar meia expiração nem relógio avançado quando faltar capacidade. CAPTURED nunca expira.

## REFUND

Entradas: request_id, capture_id e principal positivo. Soma de principal reembolsado `r` não excede `capture_principal D`. Para tarifa própria da captura G, tarifa cumulativa devolvida `g(r)=floor(G*r/D)`; nova devolução é diferença dos cumulativos. Não usar tarifa da autorização inteira nem FX novo.

Aplicar principal e tarifa separadamente: cancelar valores ainda não pagos nos lotes da captura, por vencimento mais distante primeiro, depois índice de parcela decrescente. Eventual excedente de cada componente vira `cash_refund_total` (dinheiro já devolvido externamente no modelo), não saldo reutilizável nem dívida negativa. Não devolver multas. Faturas já fechadas não têm o snapshot de emissão reescrito; sua visão de saldo restante muda.

Reverter pontos proporcionais aos pontos **efetivamente concedidos** naquela captura, por diferença cumulativa `floor(points_granted*r/D)`. Não mudar limite diário bruto, contador de risco ou hold. Reembolsos são válidos após expiração/cancelamento da autorização, pois referenciam captura.

## CLOSE

Entradas: request_id, account_id, cycle. Conta começa no ciclo de sua criação; fechar exige cycle=next_cycle_to_close e `day >= (cycle+1)*30`. Fechar ciclo sem débitos produz fatura zero válida. Emissão inclui lotes agendados nesse ciclo, descontados reembolsos anteriores; principal/tarifa/multa mantêm componentes. Parcelas futuras não entram.

`due_day=(cycle+1)*30+10`; `minimum_due=min(total,max(1000,ceil(total*1000/10000)))` e zero quando total=0. Ao fechar, copiar snapshot e avançar next_cycle_to_close; fechar o mesmo ciclo com outra chave → NOOP se já fechado. Tentativa de pular ciclo → ERROR/CYCLE_ORDER. Repetição mesma chave → resposta original.

## PAY e ASSESS_LATE_FEE

PAY recebe conta e montante positivo. Aloca apenas em lotes já faturados, não em parcelas futuras. Primeiro multas, depois tarifas, depois principal; em cada classe, vencimento mais antigo, ciclo, capture_id e índice da parcela crescente. Exceder o saldo faturado ainda devido → DECLINED/OVERPAYMENT sem alteração. Pagamento parcial válido reduz debt; pagamento mínimo não quita nem impede inadimplência após due_day se restar saldo.

ASSESS_LATE_FEE recebe invoice_id. Posta 1000 uma única vez quando `day>due_day` e saldo restante>0; caso contrário NOOP. Tier não dá isenção de multa. Multa entra no saldo faturado e debt, mas não muda o snapshot de emissão nem mínimo original. Pagamentos/reembolsos não desmarcam `late_fee_assessed`. Sem juros recorrentes ou capitalização.

## Saídas e erros

`decision`: APPROVED/REVIEW/DECLINED/ERROR/OK/NOOP conforme operação. Mutação concluída usa OK, AUTHORIZE usa os três resultados de negócio; CAPTURE OK. Código/razão estável, sem mensagem textual livre como único contrato. ERROR nunca tem efeito; DECLINED muda no máximo o registro de resposta de domínio/log operacional. Falha de serialização não pode reinvocar o comando.

Resultados de CANCEL/CLOSE/ASSESS NOOP são memorizáveis. Falhas de referência, envelope ou capacidade não são. CAPTURE/REFUND/PAY com limites excedidos são DECLINED e memorizáveis. Estado incompatível de CAPTURE é DECLINED/AUTH_STATE. O cliente precisa de outra chave para uma nova intenção depois de uma resposta memorizada.
