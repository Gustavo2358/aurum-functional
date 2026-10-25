# Ciclos completos já especificados

Esses cenários já têm comandos e resultados concretos em [flows.feature](features/flows.feature). Complementam os casos de unidade/componente; não são novas regras no denominador.

| Cenário | Objetivo | Comandos | Regras principais |
|---|---|---:|---|
| SC-FLOW-001 | Capturar em partes, faturar e quitar | 6 | BR-AUT-002, BR-CAP-003, BR-BIL-007, BR-REW-008 |
| SC-FLOW-002 | Captura parcial, cancelamento do resto e reembolso | 4 | BR-REV-002, BR-REV-009, BR-LIM-007 |
| SC-FLOW-003 | Captura parcial preservada na expiração exata | 4 | BR-REV-004, BR-CAP-001 |
| SC-FLOW-004 | Reembolso após pagamento integral vira dinheiro | 6 | BR-REV-008, BR-REW-006, BR-BIL-010 |
| SC-FLOW-005 | Parcelas com restos, pagamento e reembolso de lotes futuros | 8 | BR-INS-004, BR-INS-005, BR-REV-007, BR-BIL-008 |
| SC-FLOW-006 | Replay, conflito e reutilização de ID em outro namespace | 4 | BR-IDM-001, BR-IDM-002, BR-IDM-004 |
| SC-FLOW-007 | Mistura de decisões no mesmo minuto | 4 | BR-AUT-005, BR-LIM-008, BR-RSK-008 |
| SC-FLOW-008 | Virada diária sem liberar reserva ainda válida | 4 | BR-LIM-006, BR-AUT-006, BR-BAT-008 |
| SC-FLOW-009 | Inadimplência, pagamento e nova intenção | 7 | BR-ELG-009, BR-BIL-010, BR-IDM-006 |
| SC-FLOW-010 | Cap de pontos, reembolso e reinício de ciclo com 80 comandos | 80 | BR-REW-004, BR-REW-005, BR-REW-006, BR-REW-007 |
| SC-FLOW-011 | Falta de espaço para idempotência não deixa captura parcial | 4 | BR-AUT-008, BR-CAP-007, FR-IO-011 |
| SC-FLOW-012 | Falha intermediária não desfaz o lote | 4 | BR-BAT-001, BR-BAT-002 |

Em SC-FLOW-010, os 25 primeiros dias concedem 1000 pontos PREMIUM; devolver metade da primeira captura reverte 20, uma captura ainda no mesmo ciclo concede 0, e a captura do ciclo seguinte concede 2. Dívida final:5000000−100000+10000+10000=4920000.

Em SC-FLOW-005, captura 10001 com tarifa 100 gera totais 3368,3367,3366. Após pagar 3368, devolver 5000 de principal e 49 de tarifa cancela primeiro a última parcela; sobra dívida 1684 na segunda. A primeira emissão continua 3368 e a segunda 3367, embora seus saldos remanescentes mudem.

O alias flow.run deve usar comandos e consultas reais, não reproduzir essas contas em um simulador Python. Esses cálculos explicam o oráculo autoral e não são lógica de produção.
