# Aurum funcional — especificação de reconstrução

## Objetivo e autoridade

Reconstruir integralmente o simulador financeiro sintético Aurum em C17, em um repositório novo, mantendo comportamento público e substituindo o núcleo imperativo por funções que recebem valores/snapshots constantes e retornam resultados e edições de estado por valor. Nenhuma política representa banco, legislação ou taxa real.

Origem somente leitura: `/home/gustavo/workspace/aurum-fixture`. Neste documento, caminhos com prefixo `original:` são relativos a essa origem. O workspace de desenvolvimento é `/home/gustavo/Documents/Codex/2026-10-25/task-5/aurum-functional`; a entrega coordenada será sincronizada com hashes em `/home/gustavo/Documents/PGC/aurum-functional`. As fontes congeladas para testes estão em `tests/original/`, com `tests/original-manifest.json`. Não alterar o original, o vault ou `/home/gustavo/Documents/PGC/experimentos/joern-slicing`.

Ordem de autoridade: pedido atual do usuário e restrições da reconstrução; contratos de comportamento, catálogo e Gherkin originais; especificação de CLI para formato público; código original como referência diferencial e esclarecimento de detalhes não fixados. Contratos, catálogo e Gherkin são normativos conjuntamente: não escolher silenciosamente um deles em caso de contradição material. Registrar divergência, produzir contraprova e manter a compatibilidade legada explicitamente identificada; não ajustar expectativas para fazer a implementação passar. ADRs explicam arquitetura, sem revogar regras financeiras.

Fontes examinadas: `AGENTS.md`, `START_HERE.md`, `README.md`, `docs/PRODUCT.md`, `DOMAIN.md`, `ARCHITECTURE.md`, `ACCEPTANCE.md`, `RULEBOOK.md`, `TESTING.md`, `COMPLEXITY.md`, todos os contratos funcionais, ADR-001..009, os 14 catálogos, os 15 arquivos feature, `docs/oracle/PROBES.md`, `FLOWS.md`, `harness/PROBES.md`, `subject/CLI.md`, backlog B00..B15 e produção `subject/src/*.c`. `original:.agents/skills` não existe. O estado DONE do backlog original não comprova conclusão da reconstrução.

## Escopo integral e fronteiras

São 111 regras BR e 12 requisitos FR em 14 capacidades; 284 cenários de contrato e 12 fluxos completos, total 296. São **13 operações públicas**: QUOTE, AUTHORIZE, CAPTURE, CANCEL, REFUND, CLOSE, PAY, ASSESS_LATE_FEE, TICK, GET_ACCOUNT, GET_AUTH, GET_INVOICE e RECONCILE. Não há redução para somente tarifas ou autorização.

O núcleo deve ser independente de CLI, harness, documentação, expectativas e aliases de observação. Recebe configuração, tempo e estado explicitamente. Não faz I/O, alocação dinâmica, mutação de argumento/snapshot ou escrita por parâmetro de saída; cálculos e transições devolvem valores. O shell possui memória e I/O, interpreta comandos e publica atomicamente o estado retornado. Mutabilidade estritamente local para construir o valor devolvido é detalhe de implementação, sem alias mutável ao snapshot de entrada. A representação decidida é `Step core_step(StateView readonly, Command valor, Continuation valor)`: retorna uma edição tipada com continuação ou terminal READ/COMMIT/ABORT e Result. O shell aplica cada edição em estado privado; a próxima chamada recebe uma view constante desse estado preparado. Somente COMMIT publica. Ordem, capacidade lógica, valores, cache e decisões permanecem no core; falha de alocação do shell descarta a transação. Arrays são views const e preservam capacidades de até um milhão; continuação de tamanho fixo não é limite de coleção. Ver [ARCHITECTURE-DESIGN.md](ARCHITECTURE-DESIGN.md). APIs de domínio usam prefixo `core_`. Compatibilidade binária com a ABI C mutável original está explicitamente fora do objetivo; um adapter exclusivo de testes pode preservar a ABI antiga para executar o oráculo, sem entrar na produção.

São excluídos: aplicação de slicing, Joern/CPG e metodologias do experimento PGC; modificação do histórico/vault/original; redes, HTTP, web, banco persistente, autenticação, concorrência, criptografia, dados reais, juros/rotativo/impostos reais, regras interpretadas de documentos, respostas por cenário e LLM em runtime. Rastreabilidade de implementação e testes continua obrigatória; não confundir esta entrega funcional com requalificação `PGC_FIXTURE_QUALIFIED`.

## Estado, números e configuração

Estado contém configuração imutável, minuto lógico e coleções de contas, cartões, lojistas, autorizações, capturas, lotes, faturas, histórico de decisões, pagamentos, reembolsos, recompensas, idempotência e diário. Não há estado global mutável nem entidades financeiras implícitas. Estado público inicial não contém dívida, reserva, pagamento ou pontos. Estados históricos de testes são preparados e validados fora do núcleo.

Dinheiro é centavo inteiro em `[0,1000000000000]` (`MONEY_MAX`), com sinais somente em deltas/lançamentos. Comandos monetários exigem positivo. Soma e subtração verificam domínio antes da operação; subtração exige saldo suficiente. Multiplicação não negativa int64 deve detectar excesso antes de executar overflow. Percentual inferior é `floor(v*bps/10000)`, superior é `ceil(v*bps/10000)` e FX é metade para cima: `floor((v*rate+5000)/10000)`. Fórmulas proporcionais precisam aceitar argumentos monetários válidos mesmo quando o produto intermediário não cabe em int64; dividir de modo seguro, sem float. Falha numérica é explícita, sem clamp silencioso ou efeitos parciais.

Tempo: `0 <= now <= 525600000` minutos, `day=now//1440`, `cycle=day//30`. Nenhum relógio de parede, locale, timezone, aleatoriedade ou calendário civil participa. TTL de autorização pode produzir expires_at além do limite de entrada de now; preservar a fórmula. Expiry_day do cartão e cycle de comando são decimais não negativos representáveis em int64; os gates verificam operações derivadas.

| Configuração | Default | Domínio |
|---|---:|---|
| fx_brl | 10000 | fixo 10000 |
| fx_usd / fx_eur | 50000 / 60000 | 1..100000 |
| international_bps | 200 | 0..10000 |
| installment_bps_per_extra | 50 | 0..10000 |
| minimum_due_bps | 1000 | 0..10000 |
| tariff_cap / pin_threshold | 5000 / 10000 | 0..MONEY_MAX |
| minimum_installment | 500 | 1..MONEY_MAX |
| minimum_due_floor / late_fee | 1000 / 1000 | 0..MONEY_MAX |
| hold_ttl_minutes | 1440 | 1..10080 |
| daily_count_limit | 10 | 1..100000 |
| review_score / decline_score | 50 / 80 | 0..100, review < decline |
| cycle_days / due_grace_days | 30 / 10 | fixos |
| reward_month_cap / reward_unit | 1000 / 10000 | 0..MONEY_MAX / 1..MONEY_MAX |
| cada capacidade | 100000 | 1..1000000 |

Capacidades: `accounts_capacity`, `cards_capacity`, `merchants_capacity`, `auths_capacity`, `captures_capacity`, `lots_capacity`, `invoices_capacity`, `history_capacity`, `ledger_capacity`, `keys_capacity`, `rewards_capacity`, `payments_capacity`, `refunds_capacity`. Limites contam registros, incluindo respostas memorizadas. Zero em lançamento não consome linha. Falta de capacidade não pode truncar coleções ou parcialmente aceitar operação. As constantes de risco +15/+20/+15/+20, janela 60, máximo de parcelas 12 e máximo externo 3 são fixas. Outros valores das fórmulas leem os parâmetros correspondentes.

## Entrada e seed

CLI: `aurum --seed seed.txt --commands commands.txt` ou `--commands -`. Exatamente dois pares de flags, em qualquer ordem, cada um uma única vez; seed deve ser arquivo e não `-`. Binário de produção independente de Python.

Protocolo ASCII: uma operação por linha `OP key=value ...`; separadores espaço/tab, LF e CRLF aceitos. Linha final sem LF é aceita quando válida. Limite físico 8192 bytes incluindo terminador: excesso ou byte inválido drena a linha inteira e emite uma falha local; próxima linha continua. NUL, DEL, não ASCII e controles além de tab/terminador são inválidos; CR isolado é inválido. Linha vazia ou primeiro caractere não branco `#` é ignorada, sem resposta. Não há comentário inline, quoting nem escapes. Campos repetidos, desconhecidos, vazios, ausentes, token malformado ou enum inválido são INVALID_INPUT.

IDs: 1..48 caracteres `[A-Za-z0-9_-]`, case-sensitive; nunca truncar. Country: duas letras ASCII maiúsculas; currency: três. Inteiros decimais sem sinal, fração ou expoente, aceitando zeros à esquerda. Booleanos exatamente `true`/`false`. Enums: tier REGULAR/PREMIUM, canal POS/ECOM/CONTACTLESS, status ACTIVE/BLOCKED, categoria NORMAL/RESTRICTED. `card_present` é explícito e não inferido do canal.

Seed em ordem: um CLOCK obrigatório, zero ou mais CONFIG, contas, cartões, lojistas. Pode haver zero entidades, mas referência exige existência. Conta repetida/cartão repetido/lojista repetido no próprio namespace, configuração repetida entre linhas ou seed inválido interrompem antes de comandos. Configuração cruzada é validada ao terminar sua seção; permite mudar ambos os scores em linhas separadas. Não há CONFIG em runtime.

| Registro | Obrigatórios | Defaults opcionais |
|---|---|---|
| CLOCK | now | — |
| CONFIG | um ou mais nomes reconhecidos | defaults da tabela acima |
| ACCOUNT | id | active=true, tier=REGULAR, credit_limit=1000000, daily_limit=300000, per_operation_limit=200000, base_score=10 |
| CARD | id, account_id | status=ACTIVE, expiry_day=500, allow_international=true, allow_contactless=true |
| MERCHANT | id | status=ACTIVE, category=NORMAL, country=BR |

Limites monetários de conta aceitam zero; base_score é 0..100. Primeiro ciclo a fechar é o ciclo do CLOCK. O perfil de testes base cria A1/C1/M1 explicitamente com now=144600; o programa não os cria por default.

| Operação | Campos obrigatórios | Opcionais/defaults |
|---|---|---|
| QUOTE | amount | currency=BRL, tier=REGULAR, installments=1, country=BR |
| AUTHORIZE | request_id, account_id, card_id, merchant_id, amount | currency=BRL, installments=1, country=BR, channel=POS, card_present=true, pin_ok=true |
| CAPTURE | request_id, auth_id, principal | — |
| CANCEL | request_id, auth_id | — |
| REFUND | request_id, capture_id, principal | — |
| CLOSE | request_id, account_id, cycle | — |
| PAY | request_id, account_id, amount | — |
| ASSESS_LATE_FEE | request_id, invoice_id | — |
| TICK | now | — |
| GET_ACCOUNT | account_id | — |
| GET_AUTH | auth_id | — |
| GET_INVOICE | invoice_id | — |
| RECONCILE | — | account_id, ausente=engine inteiro |

`installments=0` ou 13 passa o parser e é rejeitado por INSTALLMENTS; negativo falha no envelope. AUTH/CAPTURE/PAY/REFUND usam request_id como ID de registro no namespace tipado. Faturas são I1, I2, ... pela criação global, não por conta nem ordem ASCII.

## Preço, elegibilidade, limites e risco

QUOTE somente valida/cota; não verifica conta, elegibilidade, crédito, consumo ou risco, nem grava idempotência. Converte amount em P pela taxa explícita de BRL/USD/EUR; moeda válida sem taxa é DECLINED/UNSUPPORTED_CURRENCY. Valida n entre 1..12, n<=3 se country!=BR e, quando n>1, `floor(P/n)>=minimum_installment`. À vista exige P positivo. Tarifa internacional: zero se BRL ou PREMIUM; caso contrário `floor(P*international_bps/10000)`. Tarifa parcelada: `floor(P*installment_bps_per_extra*(n-1)/10000)` em um cálculo. F é o mínimo entre tariff_cap e a soma das duas componentes. PREMIUM só isenta internacional; país e moeda não são intercambiáveis.

AUTHORIZE aplica esta ordem, preservando somente a primeira causa:

1. Envelope, tipos, representação, IDs e enums: ERROR/INVALID_INPUT, sem chave.
2. Idempotência por operação e request_id.
3. Resolver conta, cartão, lojista, nessa ordem; falta é ERROR/NOT_FOUND; dono diferente é ERROR/OWNERSHIP.
4. Elegibilidade: ACCOUNT_INACTIVE; CARD_BLOCKED; CARD_EXPIRED (`day>expiry_day`); MERCHANT_BLOCKED; CATEGORY_RESTRICTED; INTERNATIONAL_DISABLED (`country!=BR`); CONTACTLESS_DISABLED; PIN_REQUIRED (`card_present && amount nominal>pin_threshold && !pin_ok`); PAST_DUE (qualquer fatura com `day>due_day && outstanding>0`).
5. Cotar e validar parcelas. Valores monetários antes de cotação permanecem zero; ausência de auth_id não é string vazia no JSON.
6. Limite por operação `P<=per_operation_limit`; crédito `P+F<=max(0,credit_limit-debt-held)`; principal bruto diário+P<=daily_limit; contagem diária anterior de APPROVED/REVIEW < daily_count_limit. Igualdades monetárias passam.
7. Score `min(100,base +15 se país externo +20 se cartão ausente +15 se P>=100000 +20 se velocidade>=3)`. Velocidade conta decisões anteriores APPROVED/REVIEW da conta em `[now-60,now]`, incluindo mesmo minuto, excluindo tentativa corrente. Score>=decline_score é DECLINED/RISK, >=review_score é REVIEW/RISK; restante APPROVED/NONE.
8. Preparar todos os efeitos, incluindo cache; capacidade insuficiente é ERROR/CAPACITY sem efeito.

APPROVED fixa P/F/n, tier/categoria e tempo; reserva P+F, cria ACTIVE, soma principal bruto/contagem/histórico. Não cria dívida ou pontos. REVIEW é terminal, sem autorização/hold/dívida/pontos, mas conta para quantidade/velocidade. DECLINED não altera finanças ou contadores. Cancelar/expirar/reembolsar/pagar não desfaz consumo bruto diário. Históricos permanecem; agregados filtram dia/conta.

## Idempotência, atomicidade e ordem

Chave `(op,request_id)`: mesma intenção canônica retorna a resposta original, sem executar efeitos novamente, mesmo após alterar tempo/estado. Canonicalização ignora ordem das chaves, zeros à esquerda, defaults omitidos versus explícitos e campos internos não semânticos. Outra intenção com mesma chave é ERROR/IDEMPOTENCY_CONFLICT; outro tipo de operação pode reutilizar o ID.

Memorizar APPROVED/REVIEW/DECLINED e mutações OK/NOOP válidas. ERROR não consome chave (inclui INVALID_INPUT, NOT_FOUND, OWNERSHIP, CAPACITY, NUMERIC_RANGE, CYCLE_ORDER, conflito). QUOTE/GET/RECONCILE/TICK não ocupam cache. TICK repetido no mesmo minuto não repete expiração. Erro deixa todo estado lógico igual; rejeição de domínio pode adicionar cache, mas preserva finanças; REVIEW pode adicionar histórico e cache. Falha de serialização nunca reaplica transição.

Atomicidade é por comando, não por batch. Processar ordem recebida e continuar após rejeição local. Falha no segundo comando preserva primeiro e permite terceiro. Ordenação ASCII é para relatórios e expiração, não para reordenar entrada. Operações de contas distintas isolam projeções, mas ordem global de eventos e IDs de faturas continua observável.

## Ciclo de autorização, captura e reversão

CAPTURE requer referência existente e autorização ACTIVE/PARTIAL com `now<expires_at`; terminal/expirada é DECLINED/AUTH_STATE. Valor d>0 e c+d<=P; excesso é DECLINED/CAPTURE_AMOUNT. Para n>1, cada captura exige `floor(d/n)>=minimum_installment`. Não reavaliar elegibilidade, país, moeda ou risco. Tarifa delta=`floor(F*(c+d)/P)-floor(F*c/P)`; captura total termina com tarifa total F.

Captura reduz reserva por d+delta, aumenta dívida no mesmo total, cria captura, n lotes, concessão de pontos e diário juntos. c=0 implica ACTIVE, 0<c<P PARTIAL, c=P CAPTURED. Parcelas dividem principal e tarifa separadamente: q=total//n, r=total%n, primeiros r índices recebem q+1. Parcela i é do ciclo da captura+i; due_day=(cycle_i+1)*30+10. Soma de partes conserva cada componente.

CANCEL em ACTIVE/PARTIAL libera somente reserva restante e muda para CANCELLED, preservando capturas/dívida/pontos. CAPTURED/CANCELLED/EXPIRED com nova chave é NOOP; referência ausente é erro. TICK exige monotonicidade e expira todos ACTIVE/PARTIAL com expires_at<=novo now em ordem ASCII; mantém capturas/dívida/pontos, não expira CAPTURED. Falha de capacidade/aritmética preserva relógio e todas as reservas.

REFUND usa captura D/G, não autorização inteira nem FX atual. Principal acumulado r+d<=D; excesso é DECLINED/REFUND_AMOUNT. Tarifa delta=`floor(G*(r+d)/D)-floor(G*r/D)`. Cancelar principal e tarifa devidos separadamente nos lotes dessa captura, ciclo mais distante e índice decrescente primeiro. Excedente de cada componente representa dinheiro já devolvido externamente e soma cash_refund_total; não cria crédito reutilizável nem dívida negativa. Não devolver multa. Não reabrir autorização, mudar consumo/histórico/hold nem reescrever emissão da fatura. Captura permanece reembolsável após cancelar/expirar autorização.

Pontos: `raw=floor(D/reward_unit)*(1 REGULAR ou 2 PREMIUM)`; categoria RESTRICTED gera zero em cálculo defensivo/histórico, embora nova autorização normal a recuse. Fixar tier/categoria na autorização. Concessão=`min(raw,max(0,reward_month_cap-gross_granted_no_ciclo_da_captura))`. Reembolso reverte diferença cumulativa proporcional ao granted real, após cap. Reversão reduz líquido, não libera cap bruto. Nova captura em novo ciclo usa novo cap. Dividir captura pode diminuir pontos pelo truncamento individual; não exigir falsa invariância por divisão.

## Faturas, pagamentos e multa

CLOSE exige conta existente, cycle=next_cycle_to_close e day>=(cycle+1)*30. Ciclo futuro é ERROR/CYCLE_ORDER; ciclo aberto é ERROR/CYCLE_NOT_ENDED. Ciclo já fechado com fatura existente e chave nova é NOOP; ciclo anterior à criação sem fatura é CYCLE_ORDER. Criar inclusive fatura zero, sem lançamento financeiro, e avançar next_cycle_to_close. Emissão contém principal/tarifa não cancelados de lotes agendados no ciclo; futuras parcelas não entram. Snapshot emitido é imutável; outstanding é projeção atual. due_day=(cycle+1)*30+10, mesmo se fechamento tardio. Mínimo=`min(total,max(minimum_due_floor,ceil(total*minimum_due_bps/10000)))`; zero se total=0.

PAY somente aloca saldo já faturado. Ordem global de classes: multa, tarifa, principal; dentro de classe vencimento/ciclo mais antigo, capture_id ASCII e índice crescente; multas empatadas usam ID de fatura estável. Valor acima do faturado é DECLINED/OVERPAYMENT integral, sem crédito credor. Parcela futura continua dívida. Pagamento parcial/minimum_due não elimina inadimplência enquanto houver saldo após due_day, nem reescreve emissão/mínimo.

ASSESS_LATE_FEE: posta late_fee uma única vez quando day>due_day e outstanding>0 e flag falsa; caso contrário NOOP. PREMIUM não isenta. Adiciona dívida/saldo faturado; não muda snapshot emitido/mínimo. Pagamento/reembolso não limpa flag. NOOP sem aplicação não marca flag; configuração late_fee=0 ainda permite aplicação bem-sucedida e flag, sem linha financeira zero.

## Projeções, diário e reconciliação

`debt` soma principal/tarifa/multa ainda não pagos/cancelados, inclusive futuro. `held` soma reservas ACTIVE/PARTIAL. `invoiced_outstanding` soma lotes faturados e multa restante; `future_outstanding` soma lotes não faturados. `available_credit=max(0,credit_limit-debt-held)`. Pontos líquidos somam granted-reversed. Caches de dívida/hold devem concordar com fontes reais e diário.

Diário sintético: débito positivo e crédito negativo; soma de cada event_id deve ser zero. Abrir reserva: HOLD_ASSET/HOLD_OFFSET. Liberar: HOLD_OFFSET/HOLD_ASSET. Capturar: RECEIVABLE_PRINCIPAL/MERCHANT_CLEARING e RECEIVABLE_FEE/FEE_REVENUE, além de liberar reserva. Multa: RECEIVABLE_LATE/LATE_REVENUE. Pagar: CASH/recebível alocado. Reembolsar: MERCHANT_CLEARING ou FEE_REVENUE/recebível cancelado ou CASH da parte já paga. Omitir linhas de valor zero de modo estável. IDs de evento são `OP:request_id`; expiração `EXPIRE:auth_id`. Esses nomes compostos não se sujeitam ao limite de IDs públicos. Testes históricos podem usar OPENING contra OPENING_OFFSET balanceado.

RECONCILE calcula fontes, verifica caches/saldos contábeis, limites de lotes e balanceamento por evento; nunca repara. Retorna diferenças tipadas debt_difference/held_difference. Diferença zero não garante êxito se outro invariante falhou. Consulta account_id inexistente é NOT_FOUND. No legado, balanceamento dos eventos é global mesmo quando account_id filtra as projeções; preservar e testar esse detalhe em estado corrompido de teste.

## Contrato byte exact e observáveis

A comparação diferencial usa a mesma entrada, argv e ambiente suportado para os dois binários e compara separadamente **stdout bytes**, **stderr bytes**, **status de processo** e **efeitos lógicos após cada comando**. Não normalizar espaços, campos JSON, terminadores, razões ou ordem para esconder diferenças. Comparação tipada adicional explica falhas, sem substituir bytes. Efeitos incluem relógio, configuração, entidades, autorizações, capturas, lotes, faturas/snapshots, histórico, chaves/respostas, pagamentos, reembolsos, recompensas, diário, caches e projeções. Excluir endereços, padding e capacidade física de buffer; incluir limites configurados e quantidades lógicas. Não exigir ABI/nomes internos iguais aos do legado.

stdout tem somente um JSON canônico por comando processado, sem espaços entre tokens, LF final. Prefixo sempre `op`, `request_id` opcional, `decision`, `reason`. Erros mostram somente prefixo, salvo erro de reconciliação que inclui status/diferenças. IDs opcionais ausentes são omitidos. Dinheiro/números são JSON integers; flags JSON booleans. Respostas memorizadas usam campos originais, nunca projeção atual.

| Operação | Campos depois do prefixo, em ordem |
|---|---|
| QUOTE | principal, fee |
| AUTHORIZE | principal, fee, reserved, auth_id e expires_at se aprovado |
| CAPTURE | auth_id se presente, capture_id se presente, principal, fee, points |
| CANCEL / TICK | released |
| REFUND | capture_id se presente, principal, fee, cash_refund, points_reversed |
| CLOSE | invoice_id se presente, issued_total, minimum_due |
| PAY | paid |
| ASSESS_LATE_FEE | fee |
| GET_ACCOUNT | id, held, debt, available_credit, invoiced_outstanding, future_outstanding, points, cash_refund_total, daily_gross_principal, daily_count, next_cycle_to_close, auth_ids |
| GET_AUTH | id, state, principal, fee, captured_principal, captured_fee, remaining_hold, approved_at, expires_at, installments |
| GET_INVOICE | id, account_id, cycle, issued_principal, issued_fee, issued_total, minimum_due, due_day, outstanding, late_fee_assessed |
| RECONCILE | status, debt_difference, held_difference |

`auth_ids` inclui todos os IDs da conta, inclusive terminais, em ordem ASCII. Decisions: APPROVED, REVIEW, DECLINED, ERROR, OK, NOOP. Razões públicas: NONE, INVALID_INPUT, NUMERIC_RANGE, NOT_FOUND, OWNERSHIP, ACCOUNT_INACTIVE, CARD_BLOCKED, CARD_EXPIRED, MERCHANT_BLOCKED, CATEGORY_RESTRICTED, INTERNATIONAL_DISABLED, CONTACTLESS_DISABLED, PIN_REQUIRED, PAST_DUE, UNSUPPORTED_CURRENCY, INSTALLMENTS, OPERATION_LIMIT, CREDIT_LIMIT, DAILY_AMOUNT, DAILY_COUNT, RISK, CAPACITY, IDEMPOTENCY_CONFLICT, AUTH_STATE, CAPTURE_AMOUNT, REFUND_AMOUNT, CYCLE_ORDER, CYCLE_NOT_ENDED, OVERPAYMENT, INVALID_TIME, INVARIANT_VIOLATION.

Exit 0 significa stream completo, inclusive ERROR/DECLINED locais; stderr vazio no fluxo normal. Exit 2 significa falha impeditiva de invocação/seed/I/O. Diagnósticos legados, cada qual seguido de LF: `usage: aurum --seed seed.txt --commands commands.txt|-`, `cannot configure output errors`, `cannot open seed`, `invalid or unreadable seed`, `cannot open command stream`, `cannot read command stream`, `cannot write results`, `cannot close command stream`, `cannot flush results`. Falhas combinadas de escrita/flush podem produzir ambos os diagnósticos, conforme o mesmo ambiente. SIGPIPE, quando disponível, é tratado para retornar falha I/O e não terminar silenciosamente pelo sinal.

## Divergências e ambiguidades registradas

| ID | Evidência | Tratamento desta reconstrução |
|---|---|---|
| D-01 | `original:docs/ARCHITECTURE.md` e ADR-009 admitem ponteiros mutáveis, helpers com out parameters e clone/alocação dentro do dispatcher. | Divergência arquitetural intencional autorizada: núcleo funcional com retorno por valor; shell possui memória/publicação. Preservar finanças e protocolo. Não copiar ADR-009 como requisito do novo core. |
| D-02 | `original:docs/ACCEPTANCE.md`, Makefile e FULL legados incluem Joern/CPG/exportações de experimento. | Gate experimental excluído do pedido atual. Não executar slicing/metodologias nem declarar qualificação PGC herdada. Testes funcionais, independência e rastreabilidade continuam exigidos. |
| D-03 | Contratos dizem request_id quando parseado e op INVALID quando desconhecida/ilegível; `protocol.c:tokenize/parse_command` distingue falha lexical global de validação sequencial. | Compatibilidade explícita: duplicata/token malformado/linha inválida retorna op INVALID sem request_id; erro de campo após tokenização preserva op reconhecida e somente request_id já lido antes do erro. Não “melhorar” erro para recapturar campos posteriores. |
| D-04 | CLOSE repetido com nova chave é NOOP; `billing.c:close_apply` retorna invoice_id existente e totais da resposta zerados. | Preservar esses bytes. Replay da chave original retorna a emissão original; GET_INVOICE consulta a fatura. Não preencher NOOP com snapshot por conveniência. |
| D-05 | `OPERATIONS.md` coloca preflight após gates de domínio; `engine.c:execute` clona estado antes de resolver entidades. | Possível precedência distinta somente sob falha real/injetada de alocação do clone. Não transferir clone para o novo core. Separar limite lógico configurado, falha de infraestrutura do shell e fault injection; registrar qualquer divergência observada com caso reproduzível. |
| D-06 | Enum interno `RESTRICTED_CATEGORY`, `CYCLE_OPEN`, `RECONCILIATION` difere de nomes externos. | Usar CATEGORY_RESTRICTED, CYCLE_NOT_ENDED, INVARIANT_VIOLATION nos bytes públicos; não expor identificador interno como nova razão. |

D-03/04/06 esclarecem detalhes, não são defeitos financeiros comprovados. Não há promessa de equivalência universal entre ocorrências ambientais de malloc failure em arquiteturas com alocação diferente; os limites de capacidade configurados e sua atomicidade devem ser equivalentes. Nenhuma divergência funcional material entre catálogo/Gherkin e código foi demonstrada por execução durante o inventário documental. A inspeção não substitui os gates diferenciais. Novas divergências exigem registro de entrada, saída normativa, saída original, saída nova e decisão, sem consertar silenciosamente o legado.

## Aceitação funcional

Executar todos os 296 cenários com parser oficial Gherkin, sem skip/passo indefinido; distinguir UNIT, COMPONENT e E2E. Os 12 fluxos reais usam todos seus comandos, inclusive o fluxo de cap com 80 comandos. Expectativas vêm de literais autorais ou referência matemática independente, nunca do novo SUT. O legado é referência diferencial de compatibilidade, não fonte única de correção.

Cobrir fronteiras abaixo/igual/acima; precedência com causas simultâneas; configurações não default; capacidade em cada coleção e falha tardia do cache; replay/conflito de cada mutação; ordenação; limites de linha; recuperação de stream; round trips de parsing; invariantes depois de cada passo; aritmética com máximo e produto intermediário grande. Inserir consultas não muda estado. Capturas cumulativas conservam F; reembolsos totais conservam G e granted. Não exigir invariância falsa de pontos por divisão nem de diário em TICKs segmentados.

Núcleo: testar mesma entrada duas vezes, ausência de alteração do snapshot de entrada e resultados completos iguais; nenhuma dependência de I/O/heap/tempo global e nenhum ponteiro de saída mutável. Shell: erros de seed/argv/I/O, status e bytes. Compilar C17 estrito com warnings como erros; executar build normal e ASan/UBSan quando suportados. Falta de ferramenta é NOT_RUN, nunca PASS. Rastreabilidade deve ligar todas as regras/requisitos a código real/teste, com inventário inverso de funções e decisões; não inventar spans antes de o código existir.
