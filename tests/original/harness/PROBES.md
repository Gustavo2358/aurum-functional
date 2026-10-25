# Bindings do oráculo ao C real

O runner usa `gherkin-official` 42.0.1 para analisar os arquivos e compilar pickles,
inclusive expansão de Examples. Cada pickle precisa ter os três steps definidos,
DataTables com literais JSON e pelo menos uma expectativa. Operação desconhecida,
step indefinido, comparação divergente ou preparação inválida falha; não há skip.

```sh
make all
.venv/bin/python harness/run.py --self-test
.venv/bin/python harness/run.py --family mny --family prc
```

`--family` filtra pelo nome do arquivo `.feature`, sem selecionar cenário por ID.
O relatório padrão é `build/gherkin-results.json`. `--report` permite outro destino.
`AURUM_BIN` e `AURUM_LIB` permitem executar builds instrumentados; seus defaults
são `build/aurum` e `build/libaurum.so`.

## Mapeamento verificável

[probe-map.json](probe-map.json) registra **cada alias** com os símbolos C
invocados, os campos de entrada, as observações e a preparação usada.
`bindings.py` cria o ABI ctypes mecanicamente a partir de `aurum.h`: enums,
estruturas e assinaturas. Não interpreta documentação financeira. `probes.py`
recebe somente a operação e os dados de entrada; o runner mantém as expectativas
separadas e compara tipo e valor exatos. Nenhum binding recebe BR/FR/SC ou escolhe
comportamento por nome de cenário.

Os níveis registrados são:

- **UNIT:** helper de produção com escalares, como `money_fx`, `split_part`,
  `cumulative_delta`, `command_same_key` e `invoice_minimum`.
- **COMPONENT:** engine preparado com dados explícitos, operações `execute` ou
  helpers de estado e observações de coleções. Uma preparação de componente
  verifica estrutura e valores locais, mas não afirma alcançabilidade pelo
  perfil comercial. Exemplos normativos de tarifa F=3, cancelamento isolado de
  principal ou concessões históricas escalares permanecem nessa classe.
- **E2E:** processo `aurum --seed ... --commands -`; os fluxos completos,
  protocolo bruto, ordenação de consulta e variações de ambiente usam a CLI.

`flow.run` envia **todas** as linhas do cenário à CLI. Um segundo engine executa
as mesmas linhas usando `parse_command` e `execute`, para tornar o diário
observável ao harness. Respostas da CLI são comparadas às respostas C; consultas
públicas adicionais de conta, autorização e fatura são comparadas às projeções C.
O balanceamento é calculado pelo harness sobre os lançamentos efetivos do segundo
engine, e confrontado com `ledger_balanced`. As expectativas continuam sendo os
literais autorais da feature, não as respostas do segundo engine.

## Preparação e efeitos

O perfil base é explícito. Overrides tipados alteram apenas os campos declarados.
Históricos fornecidos substituem a coleção. Lotes históricos recebem referências
válidas a captura, autorização, conta e fatura; os pais resumem os valores de
entrada declarados, sem consultar resultados esperados. A validação recusa
referências ausentes, identidades duplicadas, componentes negativos e principal
ou tarifa consumidos além do valor original. Lançamentos OPENING balanceados
representam os recebíveis e as reservas de entrada; `reconcile` é exigido antes
da chamada avaliada. Fault injection ocorre após essa preparação.

Preços já contratados são entradas de autorizações sintéticas. Capturas
anteriores de probes de transição são operações C reais. Emissão, pagamento,
estorno, cancelamento e expiração usam o dispatcher real. `limits.record_decision`
chama o mesmo `record_decision` usado por `authorize_apply`; não insere a resposta
esperada no histórico. A comparação de namespace chama `command_same_key`, também
usado pelo dispatcher. Dados escalares de concessões históricas têm pais
estruturais e permanecem COMPONENT; testes de `flow.run` validam concessões
alcançadas por comandos públicos.

`invoice_setup` usa `minimum_due` explícito quando fornecido; para preparações em
que o campo não é observado, usa o helper de produção para produzir metadados de
fatura coerentes. Isso não produz a expectativa do teste de mínimo: aquele teste
compara diretamente `invoice_minimum` com literais fixos da feature.

## Comparações independentes

`execute_checked` envolve cada comando da API:

- ERROR exige igualdade de todo o estado lógico, incluindo configurações,
  relógio, entidades, histórico, idempotência e coleções financeiras.
- DECLINED e REVIEW exigem igualdade das coleções financeiras e totais de conta.
- DECLINED também preserva contagem e consumo diário.

Snapshots não incluem endereços, bytes de padding ou capacidade de alocação.
`financial_unchanged` e `unchanged` são calculados no harness; `same_response` e
`same_quote` comparam os valores realmente retornados. `events_added` usa IDs dos
eventos do diário; deltas contábeis somam lançamentos por conta. Reconciliação
negativa corrompe somente os caches explicitamente declarados depois de uma
preparação válida. O harness compara projeção, cache e saldos do diário e confere
o resultado contra a função de produção.

`--self-test` exige que uma expectativa deliberadamente divergente seja
rejeitada, que uma operação desconhecida falhe e que uma preparação com
cancelamento maior que o principal seja recusada. Esses testes aparecem
separadamente como HARNESS, sem inflar a contagem de cenários Gherkin.

## Barreira do sujeito

Este diretório, as features, mapas, relatórios e dependências Python ficam fora
do pacote analisável. Somente os arquivos de produção C são ligados ao binário
ou à biblioteca; o core não lê as features, os aliases ou qualquer expectativa.

## Validação independente suplementar

`numeric_properties.py` compara helpers C com matemática Python em inteiros de
precisão arbitrária, escrita no avaliador. A semente determinística padrão é
20260930. Cobre limites monetários e de int64, produto proporcional cujo
intermediário excede int64, arredondamentos, deltas cumulativos, conservação de
parcelas, intervalos de configuração e efeitos de parâmetros diferentes dos
defaults. As fórmulas desse teste suplementar não são steps Gherkin nem parte do
sujeito. O relatório informa separadamente casos numéricos e asserções.

`invariants.py` cria apenas configuração e entidades iniciais. Todos os registros
financeiros surgem de comandos passados a `parse_command` e `execute`. Depois de
cada comando, compara referências, somas de capturas/lotes/reembolsos, concessões
brutas e reversões, pagamentos, snapshots imutáveis de emissão, contas do diário
e projeções com equações independentes. Reexecuta mutações com a mesma chave e
com payload diferente, verificando respostas e ausência de efeitos. Inclui
expiração em ordem ASCII e rollback integral de um TICK sem espaço no diário.

```sh
.venv/bin/python harness/numeric_properties.py
.venv/bin/python harness/invariants.py
```

Ambos aceitam `--seed` e `--report`; invariantes aceita `--streams`. Os defaults
geram relatórios pequenos em `build/numeric-properties.json` e
`build/invariants.json`. Esses testes não alteram a contagem de cenários Gherkin.
