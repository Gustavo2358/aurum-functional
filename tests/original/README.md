# Aurum

Fixture financeira sintética em C17: cotação, elegibilidade, limites, risco, autorização, captura, cancelamento, expiração, reembolso, parcelas, faturas, pagamento, pontos e batch. Todas as políticas são inventadas. O programa usa apenas a biblioteca padrão C e estado/tempo explícitos.

## Executar

```sh
make all
./build/aurum --seed examples/seed.txt --commands examples/commands.txt
```

Para ler comandos da entrada padrão, use `--commands -`. Cada comando produz um objeto JSON. Rejeições locais preservam o stream; erro de seed/invocação/I/O impeditivo retorna exit 2. O [protocolo](subject/CLI.md) documenta campos, defaults, configuração, capacidades e resultados.

A API está em [aurum.h](subject/include/aurum.h). `execute` é a fronteira transacional; `engine_init`/`engine_free` controlam ownership. O engine possui suas coleções. Ponteiros para registros não sobrevivem a uma mutação bem-sucedida. As rotinas internas `*_apply` operam no estado privado preparado pelo dispatcher.

## Validar

Requisitos do produto: compilador C17 e Make. Para avaliação: Python 3, pacotes abaixo, GCC/gcov para cobertura e Joern para o gate de análise.

```sh
python3 -m venv .venv
.venv/bin/pip install -r tools/requirements.txt
make test-fast
make test-full
```

FAST compila com warnings tratados como erro, verifica a especificação, audita exemplos numéricos, executa todo Gherkin contra o C e os testes de protocolo e reconcilia a rastreabilidade.

FULL inclui testes adversariais, fórmulas independentes, invariantes, regressões da API, mutantes, ASan/UBSan, cobertura, build isolado do corpus e smoke Joern real. Resultados e versões são registrados em `artifacts/full.json`; um comando não executado nunca vira PASS. Os comandos individuais também podem ser executados pelos scripts em `tools/` e `harness/`.

## Oráculo e análise

- [Catálogo](docs/RULEBOOK.md): 111 BR e 12 FR; 296 cenários fornecidos, incluindo 12 fluxos públicos.
- [Bindings](harness/PROBES.md): 94 observações; UNIT, COMPONENT e E2E separados. Os steps não recebem expectativas para preparar o estado.
- [Rastreabilidade](evaluation/traceability.json): regras → regiões/funções → cenários, com índice inverso de funções e decisões no [inventário AST](evaluation/inventory.json).
- [Critérios públicos](evaluation/criteria-public.json): locais de retorno/escrita e seletores observáveis. Fórmulas, regras e respostas ficam nos arquivos privados do avaliador.
- [Revisão do oráculo](evaluation/oracle-review.md): cálculos independentes e contraprovas de interação.

```sh
.venv/bin/python tools/export.py
.venv/bin/python tools/joern_smoke.py
```

A exportação por allowlist produz `dist/analysis-subject/` (somente C, headers e Makefile) e `dist/evaluation-oracle/`. `dist/manifest.json` registra os hashes desses arquivos. O sujeito compila sem Python, documentação ou oráculo. Os critérios públicos são um terceiro arquivo separado.

[PROGRESS.md](PROGRESS.md) registra o estado real dos gates. CPG, locations e dependências selecionadas são verificados separadamente. Slicing limitado não prova recuperação completa das regras. A extração futura deve usar outro contexto e acesso somente ao corpus limpo e aos critérios públicos.

O experimento PGC de slicing de `fee` tem manutenção em `/home/gustavo/Documents/PGC/experimentos/joern-slicing`, separado do vault e deste repositório. Seu [README](/home/gustavo/Documents/PGC/experimentos/joern-slicing/README.md) reúne scripts, validação, avaliação, resultados e comandos atuais. O corpus `dist/analysis-subject/` e o CPG compartilhado `artifacts/joern-smoke/cpg.bin` continuam no Aurum. O script baseline e seu validador preexistentes permanecem aqui como referência histórica; a variante ampliada e os resultados específicos ficam no espaço do PGC.

## Limites da implementação

As transações copiam as coleções para garantir rollback completo; o custo por mutação cresce com o estado. As capacidades são explícitas e falham sem truncamento. Não há rede, relógio de parede, persistência, produtos reais, float para dinheiro nem interpretação de regras documentais em runtime.

Os testes de componentes incluem estados históricos preparados e fórmulas isoladas; os fluxos E2E alcançam seus estados usando comandos públicos. Cobertura e mutantes sustentam os casos exercitados, sem prova formal de equivalência nem garantia de generalização do extrator.
