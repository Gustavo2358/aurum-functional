# Oráculo, rastreabilidade e barreira experimental

## Oráculo autoral

Cada BR/FR possui enunciado normativo, condição/entradas relevantes, transformação/efeito e exemplos Gherkin concretos. O catálogo e contratos definem a generalização; um conjunto finito de exemplos não prova universalidade. Antes de implementar uma capacidade, conferir as contas dos exemplos manualmente ou com cálculo independente, e registrar contraprovas significativas.

Após implementação, gerar em `evaluation/`:

- `rule-index.json`: ID, classe BR/FR, capacidade, enunciado, entradas, guardas, efeito, referências aos cenários e qualificadores de escopo;
- `traceability.json`: regra → símbolos/regiões **reais** → cenários/testes; ligação inversa para funções e decisões;
- `criteria.json`: critério por `(arquivo, função, localização, expressão/variável, momento BEFORE/AFTER, saída observada)`, sem texto da resposta no arquivo público de critérios;
- `expected-supports.json`: conjuntos de âncoras essenciais/alternativas/irrelevantes por regra, revisados manualmente, do lado do avaliador;
- `inventory.json`: funções, decisões, exports, arquivos e justificativas de suporte técnico;
- relatório de cobertura e gaps.

IDs de nós Joern só fazem sentido com o CPG daquela execução; não são identidade universal. Âncora durável dentro de uma versão contém caminho relativo, símbolo, span de bytes/linhas, hash do arquivo e trecho/identidade estrutural. Sem inventar linhas antes de o código existir. Recriar mapping após refatoração; não forçar layout para preservar números de linha.

## Regiões e cobertura inversa

Classificação mínima: DOMAIN → BR(s); TECHNICAL → FR/contrato; SUPPORT → regra/contrato servido; TEST_ONLY → fora do sujeito. Cada função e cada decisão de produção precisa de classificação. Dentro de uma função heterogênea, mapear regiões e pontos de decisão; não usar uma única regra como guarda-chuva sem justificativa. Erro e limpeza têm vínculos reais. Não exigir regra por brace/declaração, nem usar texto de regra como comentário de código.

Gates: zero regra sem comportamento/teste implementado; zero cenário indefinido; zero função/decisão órfã; zero referência quebrada; zero expectativa derivada do SUT; zero ID de oracle no corpus limpo. Inventário deve ser produzido por ferramenta de código/AST com revisão, não preenchido só à mão omitindo arquivos.

## Critérios fornecidos não são respostas fornecidas

O MVP do extrator pode receber um efeito observável selecionado. O arquivo permitido contém localização/símbolo e campos a observar, não condição, fórmula, título da regra ou saída esperada. Separar `criteria-public.json` de mapping privado critério→BR. Esse desenho mede extração **condicionada a critérios**, não descoberta automática de todas as regras. O pacote não deve simular que essas duas tarefas são a mesma.

## Comparação futura

Gherkin é representação e teste de aceitação, não um comparador universal de equivalência textual. Avaliar condições/limiares, efeito, fórmula/arredondamento, prioridade, estado/tempo e suporte. Regras semanticamente equivalentes podem ter frases distintas ou dividir/agrupar fragmentos; alinhar unidades explicitamente e não premiar duplicatas. Separar validade da localização (o trecho existe) de suficiência causal (o trecho sustenta a afirmação).

Não dizer que provenance “prova” semanticamente uma conclusão só por listar linhas. Expectativas de suporte aceitam conjuntos alternativos válidos e fronteiras do critério; não usar snapshot exato de nós como única verdade do slice. O oracle de suporte é independente do Joern sob teste. Admitir suporte insuficiente, incerteza e abstenção sem completar condições por palpite.

## Barreira

Extrator roda em diretório/container que monta somente `analysis-subject/` e critérios públicos. Sem acesso ao workspace completo, Git, arquivo ZIP de especificações, features, bindings, resultados, traces privados ou bibliografia com a fixture descrita. Os próprios testes e scripts de análise contêm pistas: também ficam fora. O processo de implementação pode ler tudo; o processo de extração não.

Não usar a mesma conversa LLM que acabou de escrever o oracle como execução “cega” de extração. Iniciar contexto separado, registrar modelo/configuração de geração e limites. Isso reduz contaminação contextual; não elimina vieses de construção.
