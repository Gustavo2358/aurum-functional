# Esquema mínimo de seed e comandos

Este documento fecha defaults e campos; o agente ainda escolhe structs e assinaturas.

## Seed

`CLOCK now=N` é obrigatório e único. `0<=now<=525600000` minutos. A conta começa com next_cycle_to_close=day(CLOCK)//30. Dívida, reservas, pagamentos e concessões começam vazios no seed público. O harness unitário pode construir preestado histórico validado conforme PROBES.

`ACCOUNT id=ID [active=true] [tier=REGULAR] [credit_limit=1000000] [daily_limit=300000] [per_operation_limit=200000] [base_score=10]`.

`CARD id=ID account_id=ID [status=ACTIVE] [expiry_day=500] [allow_international=true] [allow_contactless=true]`.

`MERCHANT id=ID [status=ACTIVE] [category=NORMAL] [country=BR]`.

Campos entre colchetes são opcionais com default. Ordem seed: CLOCK, zero ou mais CONFIG, contas, cartões, lojistas. Default de pedido não cria A1/C1/M1 implicitamente; os perfis do harness escrevem essas entidades no seed. COUNTRY aceita dois caracteres A–Z; currency aceita três caracteres A–Z, e uma moeda sintaticamente válida sem taxa é rejeição de cotação, não erro lexical. Canal: POS/ECOM/CONTACTLESS. Coerência card_present/canal não é inferida: o campo explícito é a verdade do modelo.

Configuração é imutável depois do seed. `CONFIG` aceita somente nomes de DOMAIN.md e capacidades de armazenamento; nomes repetidos no seed são erro. Os números nos catálogos são defaults do perfil base, salvo overrides explícitos. Fórmulas devem ler a configuração apropriada, não hardcodar o default onde há parâmetro configurável.

Limites de configuração: bps0..10000; fx1..100000; hold_ttl_minutes1..10080; daily_count_limit1..100000; review_score e decline_score0..100 com review_score<decline_score; cycle_days=30 e due_grace_days=10 fixos nesta fixture; reward_unit>=1; valores monetários0..MONEY_MAX, exceto minimum_installment e reward_unit positivos. As componentes de risco +15/+20/+15/+20 e janela60 são fixas. Quantidade máxima de parcelas12 e externo3 são fixas. Cada capacidade de coleção1..1000000; default100000 para registros, salvo escolha explícita documentada de capacidade mais específica sem reduzir os casos obrigatórios. O programa pode usar armazenamento dinâmico limitado, não precisa pré-alocar1000000 elementos de cada tipo.

FX keys no seed: fx_brl=10000 obrigatório quando especificado (BRL identidade); fx_usd=50000 e fx_eur=60000 default. Outras moedas não são configuráveis nesta entrega. Não alterar BRL de 10000.

## Comandos

| Comando | Obrigatórios | Opcionais / default |
|---|---|---|
| QUOTE | amount | currency=BRL,tier=REGULAR,installments=1,country=BR |
| AUTHORIZE | request_id,account_id,card_id,merchant_id,amount | currency=BRL,installments=1,country=BR,channel=POS,card_present=true,pin_ok=true |
| CAPTURE | request_id,auth_id,principal | nenhum |
| CANCEL | request_id,auth_id | nenhum |
| REFUND | request_id,capture_id,principal | nenhum |
| CLOSE | request_id,account_id,cycle | nenhum |
| PAY | request_id,account_id,amount | nenhum |
| ASSESS_LATE_FEE | request_id,invoice_id | nenhum |
| TICK | now | nenhum |
| GET_ACCOUNT | account_id | nenhum |
| GET_AUTH | auth_id | nenhum |
| GET_INVOICE | invoice_id | nenhum |
| RECONCILE | nenhum | account_id, ausente=engine inteiro |

Todos os montantes de comando são positivos; QUOTE também exige positivo. Valores zero em fórmulas de fatura são permitidos fora desses comandos. Parcelas são inteiros lexicais não negativos: n=0/13 passa parse e é rejeitado no gate INSTALLMENTS; negativo éINVALID_INPUT. QUOTE valida moeda/parcelas e computa preço, mas não aplica elegibilidade, crédito, valor diário ou risco; cálculo puro de componentes pode ser testado fora dessa admissão.

ID de autorização/captura é o request_id no namespace tipado. Faturas usam ID determinístico `I1`, `I2`, ... na ordem de criação global, dentro do limite de 48 caracteres. Evento financeiro identifica operação+request_id; expiração usa autorização+tipo EXPIRE; chaves compostas internas não são IDs públicos restritos a 48. Ordenação ASCII em relatórios não muda a sequência de criação.

## Campos mínimos de consulta

GET_ACCOUNT: id,held,debt,available_credit,invoiced_outstanding,future_outstanding,points,cash_refund_total,daily_gross_principal,daily_count,next_cycle_to_close. GET_AUTH: id,state,principal,fee,captured_principal,captured_fee,remaining_hold,approved_at,expires_at,installments. GET_INVOICE: id,account_id,cycle,issued_principal,issued_fee,issued_total,minimum_due,due_day,outstanding,late_fee_assessed. RECONCILE: estado de reconciliação e diferenças tipadas, nunca reparo automático.

Campos opcionais ausentes são omitidos, não confundidos com zero. Padronizar JSON canônico por operação em documentação gerada antes de congelar testes de bytes. Respostas de erro têm pelo menos op,request_id quando parseado,decision,reason; nunca dependem de texto livre.
