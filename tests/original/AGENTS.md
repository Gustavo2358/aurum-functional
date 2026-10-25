# Aurum — instruções para o agente

Aurum é uma aplicação financeira sintética em **C17**, implementada e qualificada como fixture de extração de regras no PGC. O estado atual é `PGC_FIXTURE_QUALIFIED`, nos limites de `docs/ACCEPTANCE.md`; evidências e pendências estão em `PROGRESS.md` e `PACKAGE_CHECK.md`. Preserve o escopo completo ao corrigir, refatorar ou ampliar a aplicação.

## Comece aqui

Leia `START_HERE.md`, `docs/PRODUCT.md`, `docs/DOMAIN.md`, `docs/ARCHITECTURE.md` e `backlog/BACKLOG.md`. Para cada capacidade, carregue somente seu contrato, seu catálogo de regras e seus arquivos Gherkin. `PROGRESS.md` é um resumo de retomada, não um diário.

## Autoridade e decisões

Contratos de comportamento e Gherkin são normativos e precisam concordar. `docs/RULEBOOK.md` indexa o catálogo. ADRs registram decisões arquiteturais, não podem revogar silenciosamente regras. Uma contradição material deve ser registrada e resolvida explicitamente antes de implementar o comportamento afetado. Nunca altere uma expectativa para fazer o código passar. Exemplos não limitam uma regra universal ao conjunto de linhas da tabela.

## Autonomia

Escolha nomes, funções, estruturas internas, algoritmos, build e organização de testes. A arquitetura estabelece fronteiras, não um diagrama de classes obrigatório. Não peça autorização por decisão reversível. Use ADR novo apenas para decisão material. Pode paralelizar capacidades com contratos compartilhados estabilizados e responsáveis claros. Não fabrique integrações ao unir trabalho paralelo.

Trabalhe no repositório Git existente, faça commits de unidades coerentes e mantenha o estado retomável. O remote `origin` aponta para `git@github.com:Gustavo2358/aurum.git`; use-o para as publicações solicitadas pelo usuário. Não use código, dados ou regras do banco do usuário. Não adicione produtos financeiros, regulamentos ou políticas reais: todo o domínio aqui é inventado.

## Regras inegociáveis

- Todo comportamento implementado deve ter regra, contrato ou requisito documentado; toda função/ramificação relevante deve ter rastreabilidade inversa. Código auxiliar liga-se ao contrato funcional/técnico que serve, sem inventar uma regra de negócio por linha.
- O oráculo existente antecede a implementação. Para novos comportamentos, escreva as expectativas antes do código; elas não podem ser calculadas pela aplicação sob teste. Step definitions só preparam estado, chamam o C real e comparam resultados.
- O core não lê `.feature`, catálogo, expectativas, matrizes nem nomes de casos. Nenhum ID `BR-*`, `FR-*`, `SC-*` ou narrativa Gherkin em `subject/`.
- Toda lógica financeira permanece em C comum: não implemente um interpretador de regras carregadas da documentação, nem uma tabela de respostas por cenário.
- Sem UB, overflow silencioso, float para dinheiro, relógio implícito, aleatoriedade não controlada, rede ou dependência de locale.
- Erros financeiros não podem deixar efeitos parciais. Contabilidade, reservas, parcelas, estornos, pontos e idempotência devem concordar.
- Use o parser oficial de Gherkin no harness, não um parser por regex. Cenário não implementado, passo indefinido, skip ou fixture vazia falha no gate obrigatório.
- O pacote de análise exportado contém somente código de produção e dependências de build estritamente necessárias. O oráculo e as respostas esperadas ficam fora.
- Não confunda sucesso de parse/CPG no Joern com correção de slicing. Relate as duas coisas separadamente.
- Nunca declare PASS de comando não executado. Falta de ferramenta é `NOT_RUN`, e o gate correspondente continua aberto.

## Ciclo de entrega

Regra → fronteiras e contraprova → exemplos revisados → implementação → testes → rastreabilidade → regressão. Execute FAST a cada unidade e FULL ao fechar. Preserve evidência pequena e reproduzível. Não produza certificados, pins de commits, pilhas de versões documentais ou relatórios administrativos por etapa. Registrar versões realmente usadas na execução final é reprodutibilidade, não autorização para burocracia.

O backlog B00–B15 está concluído. Ao entregar alterações, mantenha os gates de `docs/ACCEPTANCE.md`, a rastreabilidade e as limitações verdadeiras em `PROGRESS.md` coerentes com o código e os testes executados. Se uma mudança invalidar evidência anterior, reexecute o gate afetado antes de manter a qualificação.
