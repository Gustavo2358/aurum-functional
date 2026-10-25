# language: pt
@cap_cap
Funcionalidade: Captura
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-CAP-001
  Regra: BR-CAP-001 — Captura exige autorização aberta e não expirada
    Especificação: Somente ACTIVE/PARTIAL com now<expires_at pode capturar. CAPTURED/CANCELLED/EXPIRED é DECLINED/AUTH_STATE; não reabrir reserva.

    @SC-BR-CAP-001-01
    Cenário: antes de expirar
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "ACTIVE" |
        | now | 999 |
        | expires_at | 1000 |
      Quando avalio "capture.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-CAP-001-02
    Cenário: instante da expiração
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "ACTIVE" |
        | now | 1000 |
        | expires_at | 1000 |
      Quando avalio "capture.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "AUTH_STATE" |

    @SC-BR-CAP-001-03
    Cenário: cancelada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | state | "CANCELLED" |
        | now | 999 |
        | expires_at | 1000 |
      Quando avalio "capture.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "AUTH_STATE" |

  @BR-CAP-002
  Regra: BR-CAP-002 — Captura respeita principal remanescente
    Especificação: d>0 e c+d<=P. Zero é entrada inválida; exceder saldo é DECLINED/CAPTURE_AMOUNT. Não limitar silenciosamente o montante solicitado.

    @SC-BR-CAP-002-01
    Cenário: captura restante
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | captured | 4000 |
        | amount | 6000 |
      Quando avalio "capture.amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-CAP-002-02
    Cenário: acima
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | captured | 4000 |
        | amount | 6001 |
      Quando avalio "capture.amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "CAPTURE_AMOUNT" |

    @SC-BR-CAP-002-03
    Cenário: zero
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | captured | 0 |
        | amount | 0 |
      Quando avalio "capture.amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INVALID_INPUT" |

  @BR-CAP-003
  Regra: BR-CAP-003 — Tarifa é alocada cumulativamente
    Especificação: Para F da autorização, P original e c capturado, tarifa da nova captura=floor(F*(c+d)/P)-floor(F*c/P). O último fragmento absorve resíduo exato.

    @SC-BR-CAP-003-01
    Cenário: primeiro fragmento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 3 |
        | captured | 0 |
        | amount | 3333 |
      Quando avalio "capture.fee_delta"
      Então observo os seguintes resultados
        | campo | valor |
        | fee_delta | 0 |

    @SC-BR-CAP-003-02
    Cenário: segundo fragmento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 3 |
        | captured | 3333 |
        | amount | 3333 |
      Quando avalio "capture.fee_delta"
      Então observo os seguintes resultados
        | campo | valor |
        | fee_delta | 1 |

    @SC-BR-CAP-003-03
    Cenário: último fragmento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 3 |
        | captured | 6666 |
        | amount | 3334 |
      Quando avalio "capture.fee_delta"
      Então observo os seguintes resultados
        | campo | valor |
        | fee_delta | 2 |

  @BR-CAP-004
  Regra: BR-CAP-004 — Estado depende do total capturado
    Especificação: Após captura com 0<c<P, state=PARTIAL; quando c=P, state=CAPTURED e reserva restante zero.

    @SC-BR-CAP-004-01
    Cenário: parcial
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | captured_after | 4000 |
      Quando avalio "capture.next_state"
      Então observo os seguintes resultados
        | campo | valor |
        | state | "PARTIAL" |

    @SC-BR-CAP-004-02
    Cenário: total
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | captured_after | 10000 |
      Quando avalio "capture.next_state"
      Então observo os seguintes resultados
        | campo | valor |
        | state | "CAPTURED" |

  @BR-CAP-005
  Regra: BR-CAP-005 — Captura troca reserva por dívida
    Especificação: Capturar d e tarifa delta reduz held por d+delta e aumenta debt pelo mesmo total. Sem outros eventos, disponibilidade de crédito não muda.

    @SC-BR-CAP-005-01
    Cenário: parcial com tarifa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | captured | 0 |
        | amount | 4000 |
      Quando avalio "capture.transition"
      Então observo os seguintes resultados
        | campo | valor |
        | held | 6120 |
        | debt | 4080 |
        | available_delta | 0 |

    @SC-BR-CAP-005-02
    Cenário: captura integral
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | captured | 0 |
        | amount | 10000 |
      Quando avalio "capture.transition"
      Então observo os seguintes resultados
        | campo | valor |
        | held | 0 |
        | debt | 10200 |
        | available_delta | 0 |

  @BR-CAP-006
  Regra: BR-CAP-006 — Captura tem contrapartidas contábeis
    Especificação: Principal capturado credita MERCHANT_CLEARING, tarifa credita FEE_REVENUE e os recebíveis correspondentes são debitados; também baixar reserva. Cada evento financeiro fecha em zero.

    @SC-BR-CAP-006-01
    Cenário: principal e tarifa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | amount | 10000 |
      Quando avalio "capture.ledger"
      Então observo os seguintes resultados
        | campo | valor |
        | receivable_principal_delta | 10000 |
        | receivable_fee_delta | 200 |
        | merchant_clearing_delta | -10000 |
        | fee_revenue_delta | -200 |
        | balanced | true |

    @SC-BR-CAP-006-02
    Cenário: sem tarifa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 0 |
        | amount | 10000 |
      Quando avalio "capture.ledger"
      Então observo os seguintes resultados
        | campo | valor |
        | receivable_principal_delta | 10000 |
        | receivable_fee_delta | 0 |
        | balanced | true |

  @BR-CAP-007
  Regra: BR-CAP-007 — Lotes e recompensa fazem parte do mesmo commit
    Especificação: Falha ao preparar lotes, diário ou recompensa cancela toda a captura. Não publicar recebível sem parcela/pontos nem consumir idempotência.

    @SC-BR-CAP-007-01
    Cenário: falta de lotes
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | fail_at | "LOT_STORAGE" |
      Quando avalio "capture.capacity"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CAPACITY" |
        | unchanged | true |

    @SC-BR-CAP-007-02
    Cenário: falta de diário
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | fail_at | "LEDGER_STORAGE" |
      Quando avalio "capture.capacity"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CAPACITY" |
        | unchanged | true |

  @BR-CAP-008
  Regra: BR-CAP-008 — Captura não reexecuta risco da autorização
    Especificação: Uma autorização aberta é um compromisso. Captura válida não deve ser recusada porque a janela de risco agora está alta; os gates de valor/estado/parcelas continuam obrigatórios.

    @SC-BR-CAP-008-01
    Cenário: risco alto depois
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 0 |
        | amount | 10000 |
        | current_base_score | 90 |
      Quando avalio "capture.no_reauthorization"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "OK" |
        | debt | 10000 |

    @SC-BR-CAP-008-02
    Cenário: risco baixo depois
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 0 |
        | amount | 10000 |
        | current_base_score | 10 |
      Quando avalio "capture.no_reauthorization"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "OK" |
        | debt | 10000 |
