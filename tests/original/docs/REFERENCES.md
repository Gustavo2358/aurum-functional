# Referências e origem das decisões

## Literatura usada no enquadramento

Huang, H.; Tsai, W. T.; Bhattacharya, S.; Chen, X. P.; Wang, Y.; Sun, J. **Business Rule Extraction from Legacy Code**. Proceedings of the 20th Annual International Computer Software and Applications Conference, 1996, pp.162–167. Seção3, p.163: definição `O=F(I)`; seção3.3, pp.164–165: critérios e restrições de slicing. Fonte: PDF fornecido no projeto PGC. A notação com estado/configuração/tempo usada por Aurum é nossa extensão operacional.

Cosentino, V.; Cabot, J.; Albert, P.; Bauquel, P.; Perronnet, J. **Extracting Business Rules from COBOL: A Model-Based Framework**. Fonte: PDF do projeto, seções IV–VII. Sustenta a separação entre identificação, slicing/descoberta, representação e rastreabilidade. A fixture Aurum não reproduz o exemplo da loja nem implementa o framework desse artigo.

Horwitz, S.; Reps, T.; Binkley, D. **Interprocedural Slicing Using Dependence Graphs**. ACM TOPLAS12(1), 1990, pp.26–60. Fonte: PDF do projeto, introdução e seção2. Destaca dependências e contexto de chamada; não atribuímos ao Joern garantias formais desse algoritmo sem qualificá-las.

## Documentação técnica consultada em 2026-09-30

- Cucumber, Gherkin Reference: https://cucumber.io/docs/gherkin/reference/
- Cucumber, Introduction e step definitions: https://cucumber.io/docs/
- Joern, CPG Slicing: https://docs.joern.io/cpg-slicing/
- Joern, Syntax-Tree Queries: https://docs.joern.io/c-syntaxtree/
- Joern, Exporting Graphs: https://docs.joern.io/export/

`Rule` agrupa cenários de uma regra; exemplos Gherkin precisam de bindings para execução. `joern-slice` não equivale automaticamente a um extrator integral de condições+efeitos para qualquer ponto de C.

## Autoria da fixture

Todas as taxas, limiares, políticas, entidades, operações, prioridades e exemplos de Aurum são decisões sintéticas deste pacote. Não são fatos obtidos dos papers, aconselhamento financeiro, regulação ou regras de uma instituição. O nome Aurum é um nome de trabalho da fixture.
