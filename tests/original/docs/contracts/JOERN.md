# Qualificação do sujeito no Joern

## O que validar neste repositório

Com a instalação real disponível, importar somente o pacote limpo C17, registrar comando e versão observada, inventariar métodos não externos, arquivos, relações de chamada selecionadas, CFGs e source locations. Conferir manualmente ao menos cinco sentinelas com cadeia entre arquivos, condição de controle e escrita/retorno observável. Gaps de frontend, externalização indevida ou spans ausentes devem ser explícitos.

Não fixar aqui flags imaginadas do frontend. Consultar `--help` e documentação da versão instalada. Adaptar o comando de ingestão ao build que foi realmente escolhido. Receita deve funcionar sem acesso ao oráculo. Configuração de análise não pode escolher respostas pelo nome de regra.

## Limite da premissa de slicing

A documentação oficial de `joern-slice` descreve modo data-flow como backward slicing iniciado em argumentos de chamadas; há limite de profundidade configurável e saída JSON. Isso **não** é promessa de slice completo de qualquer variável/efeito, nem de preservação automática de todas as dependências de controle/estado/memória em C.

Logo o smoke não garante que todo retorno/escrita será diretamente elegível ao comando pronto. Os critérios podem demandar consultas CPGQL e expansão de controle na etapa do extrator. Não colocar chamadas artificiais na fixture para fingir que essa limitação desapareceu. Slices parciais ou truncados devem registrar fronteira; ausência de caminho não significa inexistência de regra.

## Entregas

`tools/joern_smoke.*` e `artifacts/joern-smoke/REPORT.md` gerados na implementação, com entradas exatas, achados, stdout/stderr e status. Separar `CPG_INGESTION`, `LOCATION_MAPPING` e `SELECTED_DEPENDENCE_CHECKS`. Sem Joern → NOT_RUN e qualificação pendente; a aplicação ainda pode estar funcionalmente pronta. Só anunciar fixture qualificada para o PGC quando o gate Joern for realmente executado.

## Fontes primárias

Joern, “CPG Slicing” e “Syntax-Tree Queries”, consultados em 2026-09-30; links em `../REFERENCES.md`. A segunda referência explica por que nesting de AST não substitui análise de fluxo em controle não estruturado. As consultas completas do extrator não são escopo desta fixture.
