# Arquitetura por capacidades

## Fronteiras

| Área | Responsabilidade | Não pode fazer |
|---|---|---|
| `subject/include/` | Tipos e API C de produção | Expor IDs de regras/cenários |
| `subject/src/` | Domínio, operações, diário, projeções e CLI | Ler oráculo ou delegar políticas ao harness |
| `harness/` | Gherkin, bindings, drivers C, comparações | Reimplementar fórmula esperada usando o SUT |
| `docs/oracle/` | Regras normativas e exemplos esperados | Ser entrada do extrator |
| `evaluation/` | Matrizes, critérios, adjudicação e expectativas | Entrar no pacote analisável |
| `tools/` | Checks, build auxiliar, exportação e smoke | Produzir falso PASS ou respostas hardcoded |
| `artifacts/` | Evidências geradas de execução | Virar fonte normativa |

Organize o core por dinheiro/cotação, elegibilidade, limites, risco, autorizações, idempotência, capturas, reversões, parcelas, faturamento, recompensas e diário/batch. Agrupe ou subdivida arquivos conforme coesão. Não adote uma função artificial `rule_001` por regra. Uma regra pode cruzar capacidades e uma função pode servir várias regras.

Fluxo conceitual: leitor de comandos → dispatcher → validação/consulta de idempotência → preparação de transição → verificações → commit único → serialização do resultado. Consultas usam a mesma projeção do core. Capacidade de coleções e aritmética são verificadas antes de aplicar efeitos correlacionados.

## Estruturas e acoplamento

Estado explícito passado por ponteiro. Separar cálculo puro de mutação, mas não eliminar cadeias de dados reais entre funções. Structs, arrays, ponteiros tipados, enums, early return, switch, loops e auxiliares compartilhados fazem parte do corpus. Defina ownership em headers e evite estado global mutável. Alocação pode ser dinâmica com limites documentados; limites não podem truncar silenciosamente uma operação.

C17 e biblioteca padrão no produto. Um parser pequeno de protocolo textual basta; não adicionar framework. Python 3 no harness é permitido; `gherkin-official` faz parsing, drivers executam C real. O agente escolhe Make/CMake ou equivalente, com comandos de build/teste estáveis e `compile_commands.json` ou receita equivalente reproduzível.

## Forma do corpus

Múltiplas translation units de produção, chamadas interprocedurais reais e interfaces de observação no retorno ou em estado público consultável. Não acrescentar sinks fictícios ao core só para Joern. Critérios de slicing identificam retornos, argumentos reais ou escritas reais, e o mapeamento de regras para critérios fica fora do sujeito.

Não exigimos que `joern-slice` sozinho recupere todo o controle/estado. Sua documentação descreve slicing de dados iniciado em argumentos de chamadas; uma extração completa de contexto pode exigir consultas adicionais. Este repositório apenas qualifica ingestão, mapeamento de localização e sentinelas selecionadas. Ver `contracts/JOERN.md`.

## Exportação

Gerar `dist/analysis-subject/` por allowlist de arquivos necessários a C17, sem docs, testes, tags de regra, Git, drivers, cenários, matrizes, respostas, relatórios ou queries de seleção privilegiadas. Gerar separadamente `dist/evaluation-oracle/`. O manifesto de arquivos/hashes é válido para aquela exportação; não exige fixação burocrática de SHA do repositório.

O corpus mantém identificadores normais do domínio e enumerações reais de resultados; isso não é vazamento. Comentários que transcrevem regras/limiares como narrativa, IDs de regra ou títulos de cenários são vazamento. Não ofuscar o baseline para compensar: variantes de renomeação são experimento separado.
