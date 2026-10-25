# Definição de pronto

## Gate F — aplicação funcional

Todo catálogo BR/FR implementado, sem skip, stub ou expected-result table. Gherkin executável com parser real; todos os cenários fornecidos e ampliados passam contra C; invariantes de reservas, dívida, faturas, diário e pontos passam em sequências completas. CLI real funciona. FAST e FULL executados, resultados e ferramenta/versão reais registrados.

## Gate T — rastreabilidade e confiança do oráculo

Zero comportamento órfão após inventário de funções/decisões. Regras, testes e spans reconciliados; expectativas não derivadas do SUT; limites e suporte revisados; mutantes selecionados mortos ou equivalência justificada. Sem promover comparação por ID/contagem a prova semântica.

## Gate C — corpus experimental

Pacote de análise compilável isoladamente e livre de oráculo, com manifesto; pacote do avaliador separado. Evidência de mecanismos e interações de COMPLEXITY.md. Critérios públicos não revelam condições/resultados. Construção da fixture e futura extração em contextos separados.

## Gate J — Joern

Ingestão real do corpus limpo, inventário e locations reconciliados, sentinelas testadas, gaps explícitos. Geração de CPG não significa slice completo. Sem execução real, J=NOT_RUN e não declarar fixture plenamente qualificada para o PGC.

## Estados de fechamento

`IMPLEMENTATION_PENDING`, `FUNCTIONAL_READY`, `ORACLE_AND_CORPUS_READY`, `PGC_FIXTURE_QUALIFIED`. Um estado só é promovido com seus gates; registrar FAIL/NOT_RUN em vez de esconder. Entrega pretendida: PGC_FIXTURE_QUALIFIED, sem inventar sucesso se houver impedimento de ambiente.

## Relatório final curto

Informar comando de uso, capacidades/regras/cenários implementados, comandos de validação efetivamente executados, mutações, mapeamento, localização dos dois pacotes exportados e limitações. Evidências completas em artifacts. Não incluir cronologia extensa, cópia integral de logs nem certificados administrativos.
