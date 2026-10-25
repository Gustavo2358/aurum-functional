# Catálogo de regras e requisitos

Este índice é a entrada do oráculo. Enunciados e exemplos são normativos; contratos definem a composição entre capacidades. Todas as regras são sintéticas.

| Capacidade | Classe | Regras | Exemplos concretos | Catálogo | Gherkin |
|---|---|---:|---:|---|---|
| Dinheiro e aritmética | BR | 8 | 24 | [Abrir](oracle/catalog/mny.md) | [mny.feature](oracle/features/mny.feature) |
| Cotação e tarifas | BR | 8 | 17 | [Abrir](oracle/catalog/prc.md) | [prc.feature](oracle/features/prc.feature) |
| Elegibilidade | BR | 9 | 20 | [Abrir](oracle/catalog/elg.md) | [elg.feature](oracle/features/elg.feature) |
| Limites e consumo | BR | 8 | 17 | [Abrir](oracle/catalog/lim.md) | [lim.feature](oracle/features/lim.feature) |
| Risco determinístico | BR | 8 | 18 | [Abrir](oracle/catalog/rsk.md) | [rsk.feature](oracle/features/rsk.feature) |
| Autorização | BR | 8 | 16 | [Abrir](oracle/catalog/aut.md) | [aut.feature](oracle/features/aut.feature) |
| Idempotência | BR | 8 | 17 | [Abrir](oracle/catalog/idm.md) | [idm.feature](oracle/features/idm.feature) |
| Captura | BR | 8 | 19 | [Abrir](oracle/catalog/cap.md) | [cap.feature](oracle/features/cap.feature) |
| Cancelamento expiração e reembolso | BR | 10 | 23 | [Abrir](oracle/catalog/rev.md) | [rev.feature](oracle/features/rev.feature) |
| Parcelamento | BR | 8 | 20 | [Abrir](oracle/catalog/ins.md) | [ins.feature](oracle/features/ins.feature) |
| Faturas pagamentos e multa | BR | 12 | 28 | [Abrir](oracle/catalog/bil.md) | [bil.feature](oracle/features/bil.feature) |
| Recompensas | BR | 8 | 18 | [Abrir](oracle/catalog/rew.md) | [rew.feature](oracle/features/rew.feature) |
| Batch diário e reconciliação | BR | 8 | 17 | [Abrir](oracle/catalog/bat.md) | [bat.feature](oracle/features/bat.feature) |
| Protocolo e requisitos técnicos | FR | 12 | 30 | [Abrir](oracle/catalog/io.md) | [io.feature](oracle/features/io.feature) |

**Total autoral:** 123 regras/requisitos, 284 cenários concretos. Esse total mede a especificação entregue, não implementação nem cobertura alcançada.

Regras de cálculo puro, regras condicionadas a autorização e requisitos de protocolo têm escopos diferentes. Selecionar critérios de extração e denominadores explicitamente; uma regra defensiva inalcançável no fluxo normal não pode ser reportada como regra de uma autorização alcançável.

Além dos casos de contrato, já existem [12 ciclos completos](oracle/FLOWS.md) em [flows.feature](oracle/features/flows.feature), elevando o total a 296 cenários. Antes do fechamento, completar seus bindings e acrescentar contraprovas de [TESTING](TESTING.md). Os exemplos autorais já presentes não autorizam deixar seus bindings pendentes.
