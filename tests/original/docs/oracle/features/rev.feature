# language: pt
@cap_rev
Funcionalidade: Cancelamento expiração e reembolso
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-REV-001
  Regra: BR-REV-001 — Cancelamento de reserva intacta libera todo hold
    Especificação: CANCEL em ACTIVE sem captura libera P+F e muda para CANCELLED, sem criar recebível ou reembolso em dinheiro.

    @SC-BR-REV-001-01
    Cenário: com tarifa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | captured | 0 |
      Quando avalio "reversal.cancel"
      Então observo os seguintes resultados
        | campo | valor |
        | released | 10200 |
        | held | 0 |
        | debt | 0 |
        | state | "CANCELLED" |

    @SC-BR-REV-001-02
    Cenário: sem tarifa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 0 |
        | captured | 0 |
      Quando avalio "reversal.cancel"
      Então observo os seguintes resultados
        | campo | valor |
        | released | 10000 |
        | held | 0 |
        | state | "CANCELLED" |

  @BR-REV-002
  Regra: BR-REV-002 — Cancelamento parcial preserva a captura
    Especificação: CANCEL em PARTIAL libera apenas principal+tarifa remanescentes, sem reembolsar o que já virou dívida.

    @SC-BR-REV-002-01
    Cenário: quarenta por cento capturado
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | captured | 4000 |
      Quando avalio "reversal.cancel"
      Então observo os seguintes resultados
        | campo | valor |
        | released | 6120 |
        | held | 0 |
        | debt | 4080 |
        | state | "CANCELLED" |

    @SC-BR-REV-002-02
    Cenário: noventa por cento capturado
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | captured | 9000 |
      Quando avalio "reversal.cancel"
      Então observo os seguintes resultados
        | campo | valor |
        | released | 1020 |
        | debt | 9180 |
        | state | "CANCELLED" |

  @BR-REV-003
  Regra: BR-REV-003 — Cancelamento de terminal é NOOP
    Especificação: CANCEL de CAPTURED/CANCELLED/EXPIRED com chave nova retorna NOOP e não muda o diário. Referência ausente não éNOOP.

    @SC-BR-REV-003-01
    Cenário: capturada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "CAPTURED" |
      Quando avalio "reversal.cancel_terminal"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "NOOP" |
        | financial_unchanged | true |

    @SC-BR-REV-003-02
    Cenário: já cancelada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "CANCELLED" |
      Quando avalio "reversal.cancel_terminal"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "NOOP" |
        | financial_unchanged | true |

    @SC-BR-REV-003-03
    Cenário: ausente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "MISSING" |
      Quando avalio "reversal.cancel_terminal"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "NOT_FOUND" |

  @BR-REV-004
  Regra: BR-REV-004 — Expiração inclui o instante limite
    Especificação: TICK expira reservas abertas com expires_at<=now. Expirar reserva parcialmente capturada preserva dívida/pontos da captura e não altera CAPTURED.

    @SC-BR-REV-004-01
    Cenário: um minuto antes
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "ACTIVE" |
        | expires_at | 1000 |
        | now | 999 |
        | remaining_hold | 10000 |
      Quando avalio "reversal.expire"
      Então observo os seguintes resultados
        | campo | valor |
        | state | "ACTIVE" |
        | released | 0 |

    @SC-BR-REV-004-02
    Cenário: instante exato
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "PARTIAL" |
        | expires_at | 1000 |
        | now | 1000 |
        | remaining_hold | 6000 |
        | debt | 4000 |
      Quando avalio "reversal.expire"
      Então observo os seguintes resultados
        | campo | valor |
        | state | "EXPIRED" |
        | released | 6000 |
        | debt | 4000 |

    @SC-BR-REV-004-03
    Cenário: capturada não expira
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "CAPTURED" |
        | expires_at | 1000 |
        | now | 1001 |
        | remaining_hold | 0 |
        | debt | 10000 |
      Quando avalio "reversal.expire"
      Então observo os seguintes resultados
        | campo | valor |
        | state | "CAPTURED" |
        | released | 0 |
        | debt | 10000 |

  @BR-REV-005
  Regra: BR-REV-005 — Reembolso respeita o principal da captura
    Especificação: Solicitação positiva e soma de reembolsos<=D da captura referenciada. Ultrapassar éDECLINED/REFUND_AMOUNT; zero éINVALID_INPUT. Não usar principal total da autorização como teto.

    @SC-BR-REV-005-01
    Cenário: restante exato
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_principal | 4000 |
        | refunded | 1000 |
        | amount | 3000 |
      Quando avalio "reversal.refund_amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-REV-005-02
    Cenário: excede captura
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_principal | 4000 |
        | refunded | 1000 |
        | amount | 3001 |
      Quando avalio "reversal.refund_amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "REFUND_AMOUNT" |

    @SC-BR-REV-005-03
    Cenário: zero
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_principal | 4000 |
        | refunded | 0 |
        | amount | 0 |
      Quando avalio "reversal.refund_amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INVALID_INPUT" |

  @BR-REV-006
  Regra: BR-REV-006 — Tarifa devolvida é cumulativa por captura
    Especificação: Para tarifa G da captura D, devolver floor(G*(r+d)/D)-floor(G*r/D). Reembolso total devolve exatamente G, sem usar FX corrente.

    @SC-BR-REV-006-01
    Cenário: primeira fração
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_principal | 10000 |
        | capture_fee | 3 |
        | refunded | 0 |
        | amount | 3333 |
      Quando avalio "reversal.refund_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | fee_refund | 0 |

    @SC-BR-REV-006-02
    Cenário: última fração
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_principal | 10000 |
        | capture_fee | 3 |
        | refunded | 6666 |
        | amount | 3334 |
      Quando avalio "reversal.refund_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | fee_refund | 2 |

  @BR-REV-007
  Regra: BR-REV-007 — Reembolso abate lotes mais distantes primeiro
    Especificação: Dentro da captura, cancelar principal e tarifa ainda devidos separadamente, por ciclo mais distante e índice de parcela decrescente. Outros recebíveis da conta não recebem essa baixa.

    @SC-BR-REV-007-01
    Cenário: últimas parcelas primeiro
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | lot_principal | [1000,1000,1000] |
        | refund_principal | 1500 |
      Quando avalio "reversal.refund_allocation"
      Então observo os seguintes resultados
        | campo | valor |
        | remaining_principal | [1000,500,0] |
        | cash_refund | 0 |

    @SC-BR-REV-007-02
    Cenário: somente uma parte da última
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | lot_principal | [1000,1000,1000] |
        | refund_principal | 500 |
      Quando avalio "reversal.refund_allocation"
      Então observo os seguintes resultados
        | campo | valor |
        | remaining_principal | [1000,1000,500] |
        | cash_refund | 0 |

  @BR-REV-008
  Regra: BR-REV-008 — Parte já paga é devolvida em dinheiro
    Especificação: Após abater o saldo ainda devido da mesma captura/componente, o resto do reembolso soma a cash_refund_total. Não tornar debt negativa nem criar crédito reutilizável.

    @SC-BR-REV-008-01
    Cenário: captura toda paga
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_principal | 10000 |
        | capture_fee | 200 |
        | unpaid_principal | 0 |
        | unpaid_fee | 0 |
        | refund_principal | 10000 |
      Quando avalio "reversal.refund_paid"
      Então observo os seguintes resultados
        | campo | valor |
        | debt_after | 0 |
        | cash_refund | 10200 |

    @SC-BR-REV-008-02
    Cenário: captura parcialmente paga
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_principal | 10000 |
        | capture_fee | 200 |
        | unpaid_principal | 3000 |
        | unpaid_fee | 0 |
        | refund_principal | 5000 |
      Quando avalio "reversal.refund_paid"
      Então observo os seguintes resultados
        | campo | valor |
        | debt_after | 0 |
        | cash_refund | 2100 |

  @BR-REV-009
  Regra: BR-REV-009 — Reembolso não reabre autorização nem desfaz consumo
    Especificação: Reembolsar captura preserva estado terminal da autorização, reserva zero e principal diário bruto. Referência válida pode ser reembolsada depois de cancelamento/expiração da reserva.

    @SC-BR-REV-009-01
    Cenário: autorização cancelada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | auth_state | "CANCELLED" |
        | captured | 4000 |
        | daily_gross_principal | 10000 |
        | refund_principal | 4000 |
      Quando avalio "reversal.refund_closed_auth"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "OK" |
        | auth_state | "CANCELLED" |
        | held | 0 |
        | daily_gross_principal | 10000 |

    @SC-BR-REV-009-02
    Cenário: autorização expirada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | auth_state | "EXPIRED" |
        | captured | 4000 |
        | daily_gross_principal | 10000 |
        | refund_principal | 4000 |
      Quando avalio "reversal.refund_closed_auth"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "OK" |
        | auth_state | "EXPIRED" |
        | held | 0 |
        | daily_gross_principal | 10000 |

  @BR-REV-010
  Regra: BR-REV-010 — Reembolso não devolve multa
    Especificação: Multa de atraso é ligada à fatura, não à captura; reembolso integral do principal/tarifa não elimina a multa já lançada.

    @SC-BR-REV-010-01
    Cenário: com multa pendente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | unpaid_principal | 10000 |
        | unpaid_fee | 200 |
        | unpaid_late_fee | 1000 |
        | refund_principal | 10000 |
      Quando avalio "reversal.refund_with_late_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | unpaid_principal | 0 |
        | unpaid_fee | 0 |
        | unpaid_late_fee | 1000 |
        | debt | 1000 |

    @SC-BR-REV-010-02
    Cenário: sem multa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | unpaid_principal | 10000 |
        | unpaid_fee | 200 |
        | unpaid_late_fee | 0 |
        | refund_principal | 10000 |
      Quando avalio "reversal.refund_with_late_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | debt | 0 |
