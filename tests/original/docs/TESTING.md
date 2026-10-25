# Protocolo de teste

## Camadas

Contrato numérico com fronteiras e domínios; funções puras de cotação/risco; transições de estado; Gherkin sobre API C; sequências pelo binário; reconciliação; mutantes; exportação limpa; ingestão Joern. Testes não substituem especificação e um teste isolado não justifica simplificar a regra universal.

FAST deve compilar e executar contratos críticos + cenários tocados + integridade do oráculo/rastreabilidade. FULL inclui todo Gherkin, regressões, sanitizers disponíveis, CLI, sequências, mutantes selecionados e verificações de corpus. O agente define os comandos exatos, mantém interface `make test-fast/test-full` ou equivalentes documentados.

## Fronteiras

Para cada limiar: abaixo, igual, acima; para combinações: cada predicado isolado, conjunção, negação e conflito de prioridade. Dinheiro: zero quando permitido, um centavo, máximo, overflow protegido, divisões não exatas. Tempo: mesmo minuto, limite exato, minuto seguinte, salto e retrocesso. Histórico: vazio, um elemento, limite, registros fora/dentro da janela, empates de timestamp. Parcelas: resto zero/não zero, mínimo, n=1/12, captura parcial.

## Sequências reais obrigatórias

1. Cotar → autorizar → capturar parcialmente → capturar restante → fechar → pagar.
2. Autorizar → captura parcial → cancelar reserva → reembolsar captura.
3. Autorizar → TICK no limite → captura recusada → verificar dívida/pontos preservados.
4. Capturar → fechar → pagar integral → reembolsar → verificar cash_refund e diário.
5. Capturar parcelado → fechar primeiro ciclo → pagar → fechar seguinte → reembolsar parcial.
6. Repetir cada mutação com mesma chave; variar um campo; executar mesma chave em outro tipo.
7. Misturar APPROVED/REVIEW/DECLINED no mesmo minuto e verificar velocidade/dia.
8. Fronteira de dia/ciclo, sem mover a data para trás.
9. Fatura vencida → nova autorização recusada → pagamento → nova chave autorizável.
10. Cap de pontos → reembolso → nova captura no mesmo ciclo → ciclo seguinte.
11. Falha técnica de capacidade antes do commit de operação multicomponente.
12. Batch com sucesso → rejeição → sucesso, comprovando continuidade e não rollback global.

Os 12 ciclos de `docs/oracle/FLOWS.md` já fornecem cenários Gherkin com comandos e resultados concretos. Complete seus bindings e amplie as variações acima quando necessário; não encerre nenhuma interação obrigatória como item de checklist sem teste. Não trocar o oráculo por snapshots do código atual.

## Metamórficos

Mesma chave/corpo: estado inalterado e resposta igual. Permutar ordem das chaves do protocolo: mesma intenção. Inserir consulta: nenhum efeito financeiro. Somar captura parcelada em blocos até P: tarifa final=F, respeitados mínimos. Somar reembolsos até D: tarifa devolvida=G e pontos revertidos=concedidos. Renomear símbolos locais ou extrair helper: mesmas saídas observáveis. Operações de contas independentes podem trocar ordem preservando projeção por conta, mas não a ordem global do diário. TICK em saltos preserva projeção final de reservas, não necessariamente timestamps/eventos byte a byte.

Não inventar metamorfismos falsos: pontos são arredondados por captura e limitados no ciclo, portanto dividir captura pode mudar pontos; tarifas diárias/riscos também dependem da ordem; pagar antes de reembolsar muda quanto vira cash_refund.

## Mutação e independência

Introduzir mutantes pequenos: `>`↔`>=`; excluir uma guarda; usar P em vez de P+F no crédito; arredondar cada captura independentemente; devolver toda tarifa no primeiro reembolso; resetar cap de pontos no estorno; trocar prioridade de pagamento; executar replay outra vez; expirar um minuto cedo; aceitar linha truncada. Oráculo original deve matar mutantes não equivalentes. Mutantes sobreviventes exigem análise, não alegação automática de falha do programa. Registrar equivalentes separadamente; o gerador não pode editar expectativas junto com o código.

Revisar independentemente exemplos numéricos e pelo menos 20 regras de maior interação. “Independente” pode ser revisão humana ou uma segunda implementação de referência das fórmulas pequenas escrita da especificação, sem chamar/copiar a função C sob teste. Outro LLM não é juiz único nem prova de correção.

## Sanitização e observabilidade

C sem warnings no perfil escolhido; AddressSanitizer/UndefinedBehaviorSanitizer quando suportados, build não instrumentado também. Falta de sanitizer deve ser declarada. Comparar JSON parseado tipado e estado canônico, nunca só stdout contendo PASS. Separar cobertura de statements/branches, execução de cenários e correção de regras.
