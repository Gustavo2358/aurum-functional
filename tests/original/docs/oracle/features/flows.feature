# language: pt
@integration @flow
Funcionalidade: Ciclos financeiros completos
  Cenários compostos; não são novas regras nem entradas do extrator.

  @SC-FLOW-001 @covers_BR-AUT-002 @covers_BR-CAP-003 @covers_BR-BIL-007 @covers_BR-REW-008
  Cenário: Capturar em partes, faturar e quitar
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","CAPTURE request_id=C1 auth_id=R1 principal=4000","CAPTURE request_id=C2 auth_id=R1 principal=6000","TICK now=172800","CLOSE request_id=F1 account_id=A1 cycle=3","PAY request_id=P1 account_id=A1 amount=10000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | held | 0 |
      | debt | 0 |
      | points | 0 |
      | cash_refund_total | 0 |
      | invoices.I1.issued_total | 10000 |
      | invoices.I1.outstanding | 0 |
      | balanced | true |

  @SC-FLOW-002 @covers_BR-REV-002 @covers_BR-REV-009 @covers_BR-LIM-007
  Cenário: Captura parcial, cancelamento do resto e reembolso
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","CAPTURE request_id=C1 auth_id=R1 principal=4000","CANCEL request_id=X1 auth_id=R1","REFUND request_id=E1 capture_id=C1 principal=4000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | held | 0 |
      | debt | 0 |
      | cash_refund_total | 0 |
      | daily_gross_principal | 10000 |
      | auths.R1.state | "CANCELLED" |
      | balanced | true |

  @SC-FLOW-003 @covers_BR-REV-004 @covers_BR-CAP-001
  Cenário: Captura parcial preservada na expiração exata
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","CAPTURE request_id=C1 auth_id=R1 principal=4000","TICK now=146040","CAPTURE request_id=C2 auth_id=R1 principal=6000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | held | 0 |
      | debt | 4000 |
      | auths.R1.state | "EXPIRED" |
      | responses.3.decision | "DECLINED" |
      | responses.3.reason | "AUTH_STATE" |
      | balanced | true |

  @SC-FLOW-004 @covers_BR-REV-008 @covers_BR-REW-006 @covers_BR-BIL-010
  Cenário: Reembolso após pagamento integral vira dinheiro
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=20000","CAPTURE request_id=C1 auth_id=R1 principal=20000","TICK now=172800","CLOSE request_id=F1 account_id=A1 cycle=3","PAY request_id=P1 account_id=A1 amount=20000","REFUND request_id=E1 capture_id=C1 principal=20000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | held | 0 |
      | debt | 0 |
      | points | 0 |
      | cash_refund_total | 20000 |
      | invoices.I1.issued_total | 20000 |
      | invoices.I1.outstanding | 0 |
      | balanced | true |

  @SC-FLOW-005 @covers_BR-INS-004 @covers_BR-INS-005 @covers_BR-REV-007 @covers_BR-BIL-008
  Cenário: Parcelas com restos, pagamento e reembolso de lotes futuros
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10001 installments=3","CAPTURE request_id=C1 auth_id=R1 principal=10001","TICK now=172800","CLOSE request_id=F1 account_id=A1 cycle=3","PAY request_id=P1 account_id=A1 amount=3368","TICK now=216000","CLOSE request_id=F2 account_id=A1 cycle=4","REFUND request_id=E1 capture_id=C1 principal=5000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | held | 0 |
      | debt | 1684 |
      | points | 1 |
      | cash_refund_total | 0 |
      | invoices.I1.issued_total | 3368 |
      | invoices.I1.outstanding | 0 |
      | invoices.I2.issued_total | 3367 |
      | invoices.I2.outstanding | 1684 |
      | balanced | true |

  @SC-FLOW-006 @covers_BR-IDM-001 @covers_BR-IDM-002 @covers_BR-IDM-004
  Cenário: Replay, conflito e reutilização de ID em outro namespace
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=3000","AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=3000","AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=3001","CAPTURE request_id=R1 auth_id=R1 principal=3000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | decision_sequence | ["APPROVED","APPROVED","ERROR","OK"] |
      | held | 0 |
      | debt | 3000 |
      | auth_count | 1 |
      | capture_count | 1 |
      | daily_count | 1 |
      | balanced | true |

  @SC-FLOW-007 @covers_BR-AUT-005 @covers_BR-LIM-008 @covers_BR-RSK-008
  Cenário: Mistura de decisões no mesmo minuto
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","AUTHORIZE request_id=R2 account_id=A1 card_id=C1 merchant_id=M1 amount=100000 country=US card_present=false","AUTHORIZE request_id=R3 account_id=A1 card_id=C1 merchant_id=M1 amount=200001","AUTHORIZE request_id=R4 account_id=A1 card_id=C1 merchant_id=M1 amount=10000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | decision_sequence | ["APPROVED","REVIEW","DECLINED","APPROVED"] |
      | held | 20000 |
      | debt | 0 |
      | daily_count | 3 |
      | daily_gross_principal | 20000 |
      | balanced | true |

  @SC-FLOW-008 @covers_BR-LIM-006 @covers_BR-AUT-006 @covers_BR-BAT-008
  Cenário: Virada diária sem liberar reserva ainda válida
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | now | 145439 |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","TICK now=145440","AUTHORIZE request_id=R2 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","GET_ACCOUNT account_id=A1"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | held | 20000 |
      | daily_count | 1 |
      | daily_gross_principal | 10000 |
      | auths.R1.state | "ACTIVE" |
      | balanced | true |

  @SC-FLOW-009 @covers_BR-ELG-009 @covers_BR-BIL-010 @covers_BR-IDM-006
  Cenário: Inadimplência, pagamento e nova intenção
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","CAPTURE request_id=C1 auth_id=R1 principal=10000","TICK now=188640","CLOSE request_id=F1 account_id=A1 cycle=3","AUTHORIZE request_id=R2 account_id=A1 card_id=C1 merchant_id=M1 amount=1000","PAY request_id=P1 account_id=A1 amount=10000","AUTHORIZE request_id=R3 account_id=A1 card_id=C1 merchant_id=M1 amount=1000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | responses.4.decision | "DECLINED" |
      | responses.4.reason | "PAST_DUE" |
      | responses.6.decision | "APPROVED" |
      | held | 1000 |
      | debt | 0 |
      | points | 1 |
      | balanced | true |

  @SC-FLOW-010 @covers_BR-REW-004 @covers_BR-REW-005 @covers_BR-REW-006 @covers_BR-REW-007
  Cenário: Cap de pontos, reembolso e reinício de ciclo com 80 comandos
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | now | 130200 |
      | tier | "PREMIUM" |
      | credit_limit | 10000000 |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C1 auth_id=R1 principal=200000","TICK now=131640","AUTHORIZE request_id=R2 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C2 auth_id=R2 principal=200000","TICK now=133080","AUTHORIZE request_id=R3 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C3 auth_id=R3 principal=200000","TICK now=134520","AUTHORIZE request_id=R4 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C4 auth_id=R4 principal=200000","TICK now=135960","AUTHORIZE request_id=R5 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C5 auth_id=R5 principal=200000","TICK now=137400","AUTHORIZE request_id=R6 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C6 auth_id=R6 principal=200000","TICK now=138840","AUTHORIZE request_id=R7 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C7 auth_id=R7 principal=200000","TICK now=140280","AUTHORIZE request_id=R8 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C8 auth_id=R8 principal=200000","TICK now=141720","AUTHORIZE request_id=R9 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C9 auth_id=R9 principal=200000","TICK now=143160","AUTHORIZE request_id=R10 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C10 auth_id=R10 principal=200000","TICK now=144600","AUTHORIZE request_id=R11 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C11 auth_id=R11 principal=200000","TICK now=146040","AUTHORIZE request_id=R12 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C12 auth_id=R12 principal=200000","TICK now=147480","AUTHORIZE request_id=R13 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C13 auth_id=R13 principal=200000","TICK now=148920","AUTHORIZE request_id=R14 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C14 auth_id=R14 principal=200000","TICK now=150360","AUTHORIZE request_id=R15 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C15 auth_id=R15 principal=200000","TICK now=151800","AUTHORIZE request_id=R16 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C16 auth_id=R16 principal=200000","TICK now=153240","AUTHORIZE request_id=R17 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C17 auth_id=R17 principal=200000","TICK now=154680","AUTHORIZE request_id=R18 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C18 auth_id=R18 principal=200000","TICK now=156120","AUTHORIZE request_id=R19 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C19 auth_id=R19 principal=200000","TICK now=157560","AUTHORIZE request_id=R20 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C20 auth_id=R20 principal=200000","TICK now=159000","AUTHORIZE request_id=R21 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C21 auth_id=R21 principal=200000","TICK now=160440","AUTHORIZE request_id=R22 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C22 auth_id=R22 principal=200000","TICK now=161880","AUTHORIZE request_id=R23 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C23 auth_id=R23 principal=200000","TICK now=163320","AUTHORIZE request_id=R24 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C24 auth_id=R24 principal=200000","TICK now=164760","AUTHORIZE request_id=R25 account_id=A1 card_id=C1 merchant_id=M1 amount=200000","CAPTURE request_id=C25 auth_id=R25 principal=200000","REFUND request_id=E1 capture_id=C1 principal=100000","AUTHORIZE request_id=R26 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","CAPTURE request_id=C26 auth_id=R26 principal=10000","TICK now=173400","AUTHORIZE request_id=R27 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","CAPTURE request_id=C27 auth_id=R27 principal=10000"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | held | 0 |
      | debt | 4920000 |
      | points | 982 |
      | cash_refund_total | 0 |
      | capture_count | 27 |
      | balanced | true |

  @SC-FLOW-011 @covers_BR-AUT-008 @covers_BR-CAP-007 @covers_FR-IO-011
  Cenário: Falta de espaço para idempotência não deixa captura parcial
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | capacity.idempotency | 1 |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=10000","CAPTURE request_id=C1 auth_id=R1 principal=10000","QUOTE amount=10000","GET_ACCOUNT account_id=A1"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | responses.1.decision | "ERROR" |
      | responses.1.reason | "CAPACITY" |
      | held | 10000 |
      | debt | 0 |
      | points | 0 |
      | capture_count | 0 |
      | balanced | true |

  @SC-FLOW-012 @covers_BR-BAT-001 @covers_BR-BAT-002
  Cenário: Falha intermediária não desfaz o lote
    Dado o perfil "base" e os seguintes dados
      | campo | valor |
      | credit_limit | 10000 |
      | commands | ["AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=3000","AUTHORIZE request_id=R2 account_id=A1 card_id=C1 merchant_id=M1 amount=8000","AUTHORIZE request_id=R3 account_id=A1 card_id=C1 merchant_id=M1 amount=4000","GET_ACCOUNT account_id=A1"] |
    Quando avalio "flow.run"
    Então observo os seguintes resultados
      | campo | valor |
      | responses.0.decision | "APPROVED" |
      | responses.1.decision | "DECLINED" |
      | responses.2.decision | "APPROVED" |
      | held | 7000 |
      | debt | 0 |
      | available_credit | 3000 |
      | balanced | true |
