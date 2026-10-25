# Domínio normativo

Todos os números são políticas **inventadas para a fixture**, sem relação com legislação, taxas de mercado ou políticas de qualquer banco. Moedas são rótulos de entradas; as taxas abaixo são constantes sintéticas.

## Entidades

`Engine` contém configuração imutável, relógio lógico e coleções de contas, cartões, lojistas, autorizações, capturas, lotes a receber, faturas, pagamentos, reembolsos, recompensas, registros de idempotência e diário financeiro.

Uma conta tem ID, status ACTIVE/INACTIVE, tier REGULAR/PREMIUM, limite de crédito, limite diário, limite por operação/canal, histórico de decisões e score-base. Cartão tem ID, dono, status ACTIVE/BLOCKED, dia de validade inclusive e permissões. Lojista tem ID, status ACTIVE/BLOCKED, categoria NORMAL/RESTRICTED e país. Não há contas conjuntas, cotação consultada em rede ou corridas concorrentes.

Uma autorização aprovada fixa principal convertido `P`, tarifa total `F`, parcelas `n`, instante, expiração e política de pontos. Mantém principal capturado `c`, tarifa capturada `f(c)`, estado ACTIVE/PARTIAL/CAPTURED/CANCELLED/EXPIRED e reserva remanescente. Revisão manual é um **resultado terminal sem autorização financeira**; não existe fluxo de aprovação humana nesta fixture.

Uma captura identifica a autorização e gera lotes de principal/tarifa por parcela. Reembolso referencia uma captura específica. IDs de operações financeiras são os respectivos `request_id` (autorização, captura, pagamento e reembolso), sem gerador aleatório.

## Perfil base dos exemplos

O perfil `base` cria um engine válido com relógio no dia 100 às 10:00 (`now=144600` minutos), conta `A1` ACTIVE/REGULAR, crédito `1000000`, dívida e reservas zero, limite diário `300000`, limite de principal por operação `200000`, score-base `10`; cartão `C1` ACTIVE, dono A1, validade dia 500, permissões internacionais/contactless habilitadas; lojista `M1` ACTIVE/NORMAL/BR. Histórico, diário e coleções financeiras vazios.

Pedido base: `request_id=R1`, `account_id=A1`, `card_id=C1`, `merchant_id=M1`, `amount=10000`, `currency=BRL`, `country=BR`, `channel=POS`, `card_present=true`, `pin_ok=true`, `installments=1`. Métodos de consulta/probes não precisam de request_id. Alterações declaradas em cada cenário substituem esse perfil; coleções informadas substituem a coleção inteira, não a estendem implicitamente.

Entradas abstratas como `outstanding_principal`, `capture.principal` ou `history` representam estado lógico validamente construído pelo harness, e não nomes obrigatórios de campos em C. O harness deve mapear esses dados para estruturas reais, inclusive lançamentos de abertura balanceados quando houver dívida inicial. Não deve fabricar resultados. Casos de integração de ciclo inteiro precisam alcançar estados por comandos reais, não só por injeção de memória.

## Dinheiro e configuração

Dinheiro inteiro em centavos, intervalo `0..1000000000000`; valores assinados só em lançamentos e deltas. Bps: inteiro `0..10000`. FX em unidades de `1/10000 BRL` por unidade monetária, `1..100000`. Configuração base: BRL=10000, USD=50000, EUR=60000; international_bps=200; installment_bps_per_extra=50; tariff_cap=5000; hold_ttl_minutes=1440; pin_threshold=10000; daily_count_limit=10; review_score=50; decline_score=80; minimum_installment=500; cycle_days=30; due_grace_days=10; minimum_due_bps=1000; minimum_due_floor=1000; late_fee=1000; reward_month_cap=1000; reward_unit=10000. Quantidades em dinheiro não têm separadores em entradas/saídas máquina.

Tarifa internacional: zero para BRL ou PREMIUM; caso contrário `floor(P*200/10000)`. Tarifa de parcelamento: `floor(P*50*(n-1)/10000)` para qualquer tier, inclusive PREMIUM. Tarifa total `F=min(5000, international_fee+installment_fee)`. O desconto PREMIUM vale **somente** para a componente internacional. Cotação não muta estado.

## Parâmetros e PIN

Números de configuração nos enunciados são os defaults do perfil base; overrides admitidos no seed substituem o parâmetro correspondente. Regras fixas e limites de configuração estão em `contracts/INPUT_SCHEMA.md`. O limiar de PIN presencial compara **amount nominal** com pin_threshold, antes da cotação, deliberadamente na moeda de entrada. Não usar P convertido nesse gate.

## Tempo

Tempo é inteiro em minutos; dia `now//1440`; ciclo `day//30`; um “mês” de recompensa é o mesmo índice de 30 dias, não mês civil. País internacional significa `country != BR`; tarifa cambial depende de `currency != BRL`, não do país. Essa distinção é intencional e deve ser testada.

`TICK` avança o relógio, nunca retrocede. No mesmo instante, processa expirações com `expires_at <= now`, ordenadas por ID de autorização. Validade do cartão é inclusiva no dia. Não existem fins de semana, feriados, timezone externo ou horário de verão.

## Equações financeiras

`debt = soma de principal, tarifas e multas ainda não pagos nem cancelados em todos os lotes`, incluindo parcelas futuras.

`held = soma de reservas das autorizações ACTIVE/PARTIAL`.

`available_credit = max(0, credit_limit - debt - held)`; crédito já comprometido acima do limite produz disponibilidade zero, não valor negativo nem perdão da dívida.

Aprovação requer `P+F <= available_credit`, `P <= per_operation_limit`, `daily_gross_principal(day)+P <= daily_limit` e contagem diária de APPROVED/REVIEW anterior menor que daily_count_limit. Igualdade monetária é permitida. O consumo diário contabiliza principal autorizado, **sem tarifa**, só em APPROVED. Cancelamentos, expirações, pagamentos e reembolsos não desfazem esse consumo bruto. REVIEW conta para velocidade, mas não para valor diário. DECLINED não conta em nenhum desses dois acumuladores.

Autorizar reserva P+F sem criar dívida. Capturar d substitui reserva por dívida `d + delta_fee`, onde `f(x)=floor(F*x/P)` e `delta_fee=f(c+d)-f(c)`. Ao capturar todo o principal, toda a tarifa fica capturada. Cancelar/expirar libera somente o restante. Um reembolso nunca reabre autorização nem restaura consumo diário.

## Risco

`score=min(100, base_score + international_component + remote_component + amount_component + velocity_component)`.

Componentes: país != BR → +15; card_present=false → +20; P>=100000 → +15; contagem APPROVED/REVIEW nos últimos 60 minutos >=3 → +20. Janela operacional: **registros já existentes com `now-60 <= t <= now`**, incluindo decisões anteriores no mesmo minuto. A tentativa corrente não entra antes de classificada.

Score>=80 → DECLINED/RISK; 50<=score<80 → REVIEW/RISK; abaixo de 50 → APPROVED, desde que gates anteriores passem. Score não é probabilidade e não envolve ML.

## Precedência e atomicidade

Contrato detalhado em `contracts/OPERATIONS.md`. Ordem resumida: envelope/validação → idempotência → entidades/posse → elegibilidade → cotação/parcelamento → limites → risco → preflight de capacidade → commit.

Uma rejeição pode adicionar resposta ao registro de idempotência e ao log operacional. **Não** muda dívida, reserva, diário financeiro, pontos ou limites usados. Portanto “sem alteração financeira” não significa “nenhum byte do engine mudou”. Erro de envelope, referência inexistente, conflito de idempotência e falha técnica de capacidade não consomem chave. Resultados de domínio APPROVED/REVIEW/DECLINED de requisição válida são memorizados.

## Fora do modelo

Sem juros compostos, rotativo, imposto real, estorno de fraude, crédito em conta reutilizável, reconversão cambial no reembolso, portabilidade de dívida, integração contábil externa ou edição retroativa de eventos. Multa por atraso é a regra sintética fixa descrita em faturas.
