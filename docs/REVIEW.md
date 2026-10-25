# Revisão independente da reconstrução funcional

Revisão do código integrado, concluída em 2026-10-25. Não foi encontrada
divergência comportamental ou arquitetural aberta nos arquivos auditados.
Os hashes de produção examinados constam de
[review-manual.json](../evidence/review-manual.json). A conclusão cobre essa
versão, os contratos e os testes descritos abaixo; não é prova de equivalência
para todas as entradas nem substitui os demais gates de entrega.

Foram lidos SPEC, TASKS, TRACEABILITY, ARCHITECTURE-DESIGN, as instruções
originais, os contratos OPERATIONS, STATE_AND_LEDGER, INPUT_SCHEMA e
IO_AND_HARNESS, os módulos de produção completos, o adapter e os comparadores.
As fontes e expectativas congeladas não foram alteradas. A revisão não usou
Joern, slicing, metodologia de extração, rede ou instalação de dependências.

## Fronteiras e transições examinadas

| Obrigação | Código real inspecionado | Resultado da revisão e evidência complementar |
|---|---|---|
| Valores e snapshots imutáveis | `core.h`, `core_command.c`, `core_money.c`, `core_policy.c`, `core_state.c`, `core_transition.c` | Ponteiros de entrada constantes; structs retornados por valor; continuação sem ponteiros; nenhum heap, I/O ou estado global mutável no core. O adapter mutável só participa da biblioteca de testes. AST e runtime de pureza complementam esta inspeção. |
| Envelope, canonicalização e precedência | `core_command.c`; `core_step` BEGIN e AUTHORIZE_START | Validação precede cache; replay precede entidades e elegibilidade; cotação precede limites e risco. Defaults, campos não semânticos e namespace por operação seguem o contrato. Mutantes de replay, crédito, risco e janela foram mortos. |
| Aritmética e configuração | `core_money.c`, `core_policy.c` | Inteiros e verificações antes de operações perigosas; proporcional divide sem formar produto de até 10²⁴; FX empata para cima; tarifa parcelada calcula o produto antes de truncar. Configuração validada e fórmulas usam seus parâmetros. Oráculo numérico independente: 16.887 casos no baseline. |
| Autorização/captura | AUTHORIZE_START…AUTHORIZE_RESULT; CAPTURE_START…CAPTURE_RESULT | Histórico, reserva, captura, lotes, concessão de pontos, diário e cache ficam na mesma transação privada. Captura não reaplica elegibilidade ou risco. Tarifa cumulativa, estado e partição de lotes preservados. Mutantes de estado e arredondamento foram mortos. |
| Cancelamento/expiração | CANCEL_START…CANCEL_RESULT; TICK_START…TICK_CLOCK; `next_expiring` | Apenas saldo de reserva restante é liberado; relógio vem por último; seleção ASCII é do core. Falha na segunda autorização do TICK preserva o relógio e ambas as reservas. Mutantes de instante e ordem de expiração foram mortos. |
| Reembolso | REFUND_START…REFUND_RESULT; `refund_order`, `next_refund_lot` | Principal/tarifa devidos são abatidos separadamente, começando no ciclo e índice mais distantes; excesso vira cash; pontos usam granted real; recompensa bruta permanece. Multa e emissão não são reescritas. Mutantes de tarifa e ordem foram mortos. |
| Fechamento/pagamento/multa | CLOSE_START…CLOSE_RESULT; PAY_START…PAY_RECORD; ASSESS_START…ASSESS_INVOICE | Faturas têm ID global; CLOSE NOOP mantém a forma legada; pagamento aplica multa, tarifa, principal nessa ordem e só a lotes faturados. Multa zero ainda marca aplicação. Mutante de prioridade foi morto. |
| Diário, cache, projeções e commit | `pair`, `post`, LEDGER, FINISH, CACHE, REFRESH; `core_state.c`; `shell_storage.c:execute` | Cada buffer tem no máximo quatro pares do evento, sem limitar coleções; LEDGER esvazia antes da fase seguinte. Cache antecede refresh e COMMIT. Falhas lógicas, numéricas e de materialização descartam scratch. Mutantes de linhas zero e publicação no ABORT foram mortos. |
| Protocolo e seed | `shell_protocol.c`, `shell_output.c`, `main.c` | Shell contém parsing, envelopes, armazenamento e serialização. Seleção de ordenação em `write_auth_ids` serve somente ao relatório; política financeira e ordenação de alocações ficam no core. Seed é preparado em armazenamento privado. Mutante que aceita linha truncada foi morto. |

O domínio assume `StateView` e configuração válidos, preparados pelo seed ou
pelo setup histórico validado. A revisão dos limites considerou as capacidades
máximas de um milhão: agregações intermediárias de registros monetários válidos
cabem em int64; limites monetários das projeções são novamente verificados antes
de publicar. Ordenações por seleção podem custar tempo quadrático, conforme a
decisão arquitetural documentada; esta revisão não certifica desempenho em um
milhão de registros.

## Ajustes e contraprovas da revisão

1. `edited()` inicialmente devolvia `append.index=0` para todas as inserções.
   A inspeção identificou a inconsistência com o aplicador independente, que
   exige a posição real. O responsável corrigiu para `edit.index=length` antes
   dos gates finais. O shell continuou aplicando append de maneira mecânica.
2. A inspeção destacou formas internas diferentes de `Result ERROR/CAPACITY`:
   ASSESS conserva `invoice_id` quando falta diário, PAY conserva `principal`
   quando falta registro de pagamento, mas falta de cache reseta esses campos.
   A implementação final mantém essas formas e os testes abaixo as confirmam.
   Não houve mudança no golden para aceitar uma forma nova.
3. A primeira rodada de mutações mostrou que bytes CLI e estados do corpus
   gerado não detectavam FX com metade para baixo. O oráculo numérico já
   congelado detectou `money_fx(1,5000)` retornando 0 em vez de 1. Esse gate foi
   incorporado ao driver de mutações. A ordem do diário e a omissão de linhas
   zero exigiram o comparador de estado completo, pois os bytes CLI permaneceram
   iguais. Esses limites de observabilidade estão explícitos no relatório.

## Testes específicos executados

`python3 tests/review_regressions.py`: **PASS, 7/7**. Os sete estados são
alcançados por seed e comandos públicos. Em original e candidato, o comando
final produz ERROR/CAPACITY e preserva todas as coleções, configuração e relógio:
ASSESS no segundo lançamento e no cache; PAY no registro e no cache; CANCEL no
segundo lançamento; TICK na segunda autorização; REFUND no segundo lançamento.
As respostas completas e os hashes de snapshots são iguais. O script verifica
fontes congeladas e recompila o original isoladamente em `build/oracle`, sem
depender de um binário local previamente existente.
[Evidência](../evidence/review-regressions.json).

`python3 tests/candidate_mutations.py`: **PASS, 16 mortos, zero sobreviventes,
um controle equivalente aprovado**. Cada mutante é uma edição em cópia
temporária da implementação nova. O build limpo precisa passar 136 invocações
base, 24 sequências geradas, 7.557 comparações de estado/resultado e o oráculo
numérico antes de iniciar. Falhas de compilação ou runner não contam como morte.
O renomeamento local equivalente passa os mesmos gates. Hashes confirmam que
produção e corpora não foram modificados pela campanha.
[Evidência](../evidence/review-mutations.json).

| Mutação | Detector decisivo |
|---|---|
| Igualdade de crédito recusada; crédito sem tarifa | Corpus base, fronteiras de crédito |
| FX metade para baixo | Oráculo numérico congelado |
| Guarda de estado da captura removida | Corpus base, expiração |
| Fronteira de REVIEW exclusiva; janela de velocidade exclusiva | Corpus base, risco |
| Linha truncada aceita | Corpus base, protocolo |
| Captura arredondada independentemente; primeira devolução de tarifa integral | Corpus base, fragmentos |
| Principal pago antes da tarifa | Corpus base, prioridade |
| Expiração antecipada | Corpus base, instante de expiração |
| Expiração em ordem decrescente | Estado completo do corpus gerado |
| Reembolso começa pelos lotes antigos | Fluxo normativo 005 |
| Replay executa novamente | Corpus base, replay |
| Diário inclui linhas zero | Estado completo do corpus gerado |
| ABORT publica estado privado | Corpus base, capacidade |

Os relatórios da aceitação e da pureza são evidências complementares examinadas;
o fechamento agregado G0–G8 pertence a `tests/qualify.py` e ao relatório FULL da
entrega. O inventário AST é estrutural: a rastreabilidade semântica por região
continua sendo obrigação separada, não é inferida de um rótulo genérico de função.
Falhas ambientais de alocação seguem a distinção D-05 da especificação.
