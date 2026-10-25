# Plano executável e gates

Este plano cobre a reconstrução completa definida em [SPEC.md](SPEC.md). Não transfere o DONE do backlog original para a aplicação nova. O prefixo original: significa `/home/gustavo/workspace/aurum-fixture`; cópia congelada em `tests/original/`, com manifesto independente. O plano antecedeu a implementação e foi atualizado após os comandos/evidências reais do FULL funcional. A entrega/sincronização é registrada separadamente.

## Dependências e resultados

| Tarefa | Resultado verificável | Depende de | Estado/evidência do inventário |
|---|---|---|---|
| T01 — congelar referência | Manifesto de fontes somente leitura, corpus/oráculo preservados e build isolado do original | — | CONCLUÍDO: `tests/original-manifest.json`, `tests/original/`, `evidence/harness-original/summary.json` |
| T02 — especificar tudo | 13 operações, 14 famílias, 123 regras/requisitos, 296 cenários, observáveis e divergências documentados | T01 | CONCLUÍDO: SPEC.md e TRACEABILITY.md; mapa real em TRACEABILITY.md e traceability.json |
| T03 — qualificar harness antes do novo core | Parser oficial, auto-testes negativos, oráculo e baseline reais; corpus diferencial congelado | T01 | CONCLUÍDO para baseline: `evidence/harness-original/summary.json` PASS; não comprova novo programa |
| T04 — contrato funcional | Views constantes, números/resultados por valor, Step/Edit/Continuation, ownership e publicação COMMIT | T02 | CONCLUÍDO: ARCHITECTURE-DESIGN.md, `evidence/purity-architecture.json` PASS, core_step/StateView/Edit reais |
| T05 — aritmética/configuração/admissão | Dinheiro seguro, configuração inteira, cotação/parcelamento, elegibilidade/limites/risco/recompensas puras | T03,T04 | CONCLUÍDO: MNY/PRC/INS/ELG/LIM/RSK/REW PASS; `evidence/harness-candidate/gherkin.json` e numeric.json |
| T06 — projeções e diário | Views, busca const, fontes únicas, saldo futuro/faturado, diário por evento e reconciliação | T04,T05 | CONCLUÍDO: core_state/core_step reais; invariants.json e state-differential.json PASS |
| T07 — autorização/idempotência | Validação→replay→referências→gates→edições; cache e histórico atômicos | T05,T06 | CONCLUÍDO: AUT/IDM PASS; respostas/cache/histórico comparados em estados completos |
| T08 — ciclo e reversões | Captura cumulativa/n lotes/pontos, cancelamento, TICK ASCII, refund reverso e cash | T07 | CONCLUÍDO: CAP/REV/INS/REW e flows PASS; capacidade/rollback em `evidence/full/summary.json` |
| T09 — faturamento/pagamento | Fechamento ordenado e zero, snapshots, prioridade global de componentes, multa única | T06,T08 | CONCLUÍDO: BIL e fluxos 1/4/5/9 PASS; snapshots/ordem preservados nos estados diferenciais |
| T10 — shell/CLI/adaptador de testes | Seed atômico, parser, executor de edits, JSON e erros byte exact; adapter fora da produção | T04,T07,T08,T09 | CONCLUÍDO: C17 CLI, FR-IO/BAT, ABI somente adapter testes; protocolo/diferenciais PASS |
| T11 — equivalência completa | Todo Gherkin, propriedades, invariantes, 12 fluxos, corpus golden/gerado, bytes/status/efeitos | T10 | CONCLUÍDO: 296 cenários + 3 controles; 136+24 streams byte exact; 7557 estados de comando PASS |
| T12 — auditoria funcional e mutantes | AST/inventário de funções/decisões, ausência de heap/I/O/out mutation no core, mutantes mortos | T05..T11 | CONCLUÍDO: 130 funções/714 decisões mapeadas; arquitetura/pureza/revisão PASS; 16 mutantes mortos, 1 equivalente; FULL 14 gates PASS |
| T13 — fechar e entregar | Build normal/sanitizers, links de uso, gaps honestos, manifesto final e sync destino autorizado | T11,T12 | QA FUNCIONAL CONCLUÍDA: `evidence/full/summary.json` PASS; entrega/sync/manifesto final PGC e publicação de artefato pendentes de confirmação root |

T05 e partes do shell podem avançar em paralelo depois de T04; a integração só aceita o mesmo contrato. T06–T09 não podem ser substituídos por wrappers que mantenham política no shell. Testes novos podem ser escritos antes das tarefas dependentes e ficam pendentes até execução real.

## Gates obrigatórios desta reconstrução

| Gate | Critério de aprovação | Evidência esperada |
|---|---|---|
| G0 — preservação | Original, vault e históricos sem modificação; fontes congeladas conferem hashes | Manifesto comparado antes/depois e sync final verificável |
| G1 — build C17 | Build reproduzível normal, warnings como erros, binário sem Python/oráculo | Comando, versão e exit reais |
| G2 — oráculo | 296 cenários, três controles negativos do harness, zero skip/indefinido; tipos/valores exatos | Relatório individual de cenário e classificação UNIT/COMPONENT/E2E |
| G3 — domínio | Propriedades numéricas independentes, invariantes completos, configurações não default e 12 fluxos | Relatórios numérico/invariantes/fluxos |
| G4 — compatibilidade | Mesmos stdout/stderr bytes e exit para corpus fixo/gerado; efeitos canônicos por passo | Divergência mínima reproduzível ou relatório sem diferenças |
| G5 — core puro | Entradas const inalteradas; retorno por valor; core sem alocação/I/O/out mutation; política e ordem no core | Teste de pureza, inspeção AST/call graph e revisão de módulos |
| G6 — atomicidade | Falha lógica em cada recurso, inclusive cache/diário, não publica efeitos; replay não duplica | Snapshots completos antes/depois e status; caso TICK multirreserva |
| G7 — robustez | Build normal, ASan/UBSan suportados, protocolos adversariais e mutantes selecionados | Logs de execução; ferramenta ausente NOT_RUN; sobreviventes explicados |
| G8 — rastreabilidade | 123 BR/FR ligados a comportamento/testes e código real; funções/decisões sem órfãos | Inventário AST + revisão; spans reais; mapas sem IDs de oráculo na produção |
| J — Joern/PGC | Fora do escopo autorizado desta reconstrução | NOT_RUN explícito; não executar slicing/metodologias nem promover qualificação PGC |

FAST local deve executar build, famílias tocadas, pureza/atomicidade afetadas e integridade da rastreabilidade. FULL funcional deve executar G0–G8; não chamar cegamente `original:make test-full`, pois inclui Joern. Comandos concretos devem ser expostos pelo build novo e registrados, sem chamar um relatório editorial de teste financeiro.

## Casos críticos a manter visíveis

- Precedência com falhas simultâneas: envelope/replay/entidades/posse/elegibilidade/cotação/limites/risco/capacidade; parsing de request_id antes/depois do erro.
- Limites numéricos e tempo: 0/1/MONEY_MAX, int64 overflow protegido, FX metade, percentuais, produtos proporcionais grandes, dia/ciclo/janela/TTL exatos.
- Mesma chave com payload canônico, conflito, namespace entre operações; rejeição memorizada após mudança temporal/financeira.
- Cada mutação com coleção cheia, especialmente idempotência depois de vários efeitos; erro preserva snapshot integral e relógio.
- Capturas/reembolsos em partes conservam tarifa/pontos concedidos; cap de pontos não reabre; split de captura não tem falsa invariância de pontos.
- CLOSE novo/replay/NOOP/ciclo anterior à criação; pagamento só faturado e ordenação multa→tarifa→principal; refund deixa multa intacta.
- Linha 8192/8193, CRLF, EOF sem LF, bytes inválidos, comentário completo/inline, flags duplicadas e falhas impeditivas I/O.

## Política de conclusão

Não marcar implementação concluída só porque baseline passou. Não declarar equivalência universal por corpus finito. Qualquer diferença legado/spec deve apontar entrada, expectativa normativa, saída original, saída nova e decisão explícita. Não alterar fontes/golden para acomodar o novo SUT. Falha de malloc ambiental não é equivalência universal entre alocadores; capacidade configurada continua contrato estrito. Compatibilidade ABI binária original não é objetivo da nova API funcional.

## Evidência de fechamento funcional

`evidence/full/summary.json` confirma PASS de 14 gates efetivamente executados. `evidence/harness-candidate/` contém aceitação completa, propriedades, invariantes e regressões; diferenciais confirmam 136 streams fixos, 24 gerados e 7557 snapshots completos. `evidence/purity-architecture.json` e testes de pureza, inclusive sanitizados, aprovam as restrições exercitadas. `evidence/review-manual.json` aprova a revisão; `evidence/review-mutations.json` registra 16 mutantes mortos e um equivalente. O mapa revisado possui 123 obrigações, 130 funções e 714 decisões, sem vínculos órfãos.

G1–G8 estão sustentados pelo FULL, revisão e mapa real. G0 confirma integridade congelada; a verificação final de sincronização/destino permanece na entrega T13. Gate J segue NOT_RUN por exclusão explícita; nenhum resultado autoriza promover qualificação PGC.
