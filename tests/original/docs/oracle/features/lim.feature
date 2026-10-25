# language: pt
@cap_lim
Funcionalidade: Limites e consumo
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-LIM-001
  Regra: BR-LIM-001 — Disponibilidade considera dívida e reservas
    Especificação: available=max(0,credit_limit-debt-held); parcelas futuras fazem parte de debt e limite reduzido não pode gerar saldo disponível negativo.

    @SC-BR-LIM-001-01
    Cenário: dívida e hold
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 100000 |
        | debt | 40000 |
        | held | 30000 |
      Quando avalio "limits.available"
      Então observo os seguintes resultados
        | campo | valor |
        | available | 30000 |

    @SC-BR-LIM-001-02
    Cenário: compromisso acima do limite
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 100000 |
        | debt | 110000 |
        | held | 0 |
      Quando avalio "limits.available"
      Então observo os seguintes resultados
        | campo | valor |
        | available | 0 |

  @BR-LIM-002
  Regra: BR-LIM-002 — Crédito cobre principal mais tarifa
    Especificação: A autorização só cabe no crédito quando P+F<=available. Igualdade é permitida; validar só P é erro.

    @SC-BR-LIM-002-01
    Cenário: igualdade completa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | available | 10200 |
      Quando avalio "limits.credit"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-LIM-002-02
    Cenário: tarifa excede
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | available | 10199 |
      Quando avalio "limits.credit"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "CREDIT_LIMIT" |

  @BR-LIM-003
  Regra: BR-LIM-003 — Limite por operação usa principal convertido
    Especificação: P<=per_operation_limit; taxa não entra nessa comparação, mas a moeda precisa já estar convertida. Igualdade passa.

    @SC-BR-LIM-003-01
    Cenário: exato
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 200000 |
        | fee | 5000 |
        | per_operation_limit | 200000 |
      Quando avalio "limits.operation"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-LIM-003-02
    Cenário: um centavo acima
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 200001 |
        | fee | 0 |
        | per_operation_limit | 200000 |
      Quando avalio "limits.operation"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "OPERATION_LIMIT" |

  @BR-LIM-004
  Regra: BR-LIM-004 — Limite diário usa consumo bruto de principal
    Especificação: daily_gross_principal+P<=daily_limit. Tarifa não consome o limite diário de principal.

    @SC-BR-LIM-004-01
    Cenário: exato apesar de tarifa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | used | 290000 |
        | principal | 10000 |
        | fee | 200 |
        | daily_limit | 300000 |
      Quando avalio "limits.daily_amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-LIM-004-02
    Cenário: excesso de principal
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | used | 290001 |
        | principal | 10000 |
        | fee | 0 |
        | daily_limit | 300000 |
      Quando avalio "limits.daily_amount"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "DAILY_AMOUNT" |

  @BR-LIM-005
  Regra: BR-LIM-005 — Limite diário de quantidade verifica histórico anterior
    Especificação: Se o número anterior de APPROVED/REVIEW no dia for menor que10, a nova tentativa pode passar esse gate; com 10 já existentes, é recusada. DECLINED não entra.

    @SC-BR-LIM-005-01
    Cenário: décima tentativa contável
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | approved | 7 |
        | review | 2 |
        | declined | 20 |
      Quando avalio "limits.daily_count"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-LIM-005-02
    Cenário: limite cheio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | approved | 7 |
        | review | 3 |
        | declined | 0 |
      Quando avalio "limits.daily_count"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "DAILY_COUNT" |

  @BR-LIM-006
  Regra: BR-LIM-006 — Consumo é separado por dia lógico
    Especificação: Agregações diárias selecionam somente eventos APPROVED/REVIEW do day corrente. Registros de outro dia não desaparecem, apenas ficam fora do agregado.

    @SC-BR-LIM-006-01
    Cenário: dia corrente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 101 |
        | history | [{"day":100,"decision":"APPROVED","principal":9000},{"day":101,"decision":"APPROVED","principal":5000},{"day":101,"decision":"REVIEW","principal":7000}] |
      Quando avalio "limits.day_projection"
      Então observo os seguintes resultados
        | campo | valor |
        | gross_principal | 5000 |
        | count | 2 |

    @SC-BR-LIM-006-02
    Cenário: dia vazio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 102 |
        | history | [{"day":101,"decision":"APPROVED","principal":5000}] |
      Quando avalio "limits.day_projection"
      Então observo os seguintes resultados
        | campo | valor |
        | gross_principal | 0 |
        | count | 0 |

  @BR-LIM-007
  Regra: BR-LIM-007 — Reversões não restauram consumo bruto diário
    Especificação: Cancelar, expirar ou reembolsar não subtrai principal do agregado diário de autorizações aprovadas. Limite de crédito é liberado por regras próprias, não pelo contador diário.

    @SC-BR-LIM-007-01
    Cenário: cancelamento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | approved_principal | 10000 |
        | action | "CANCEL" |
      Quando avalio "limits.after_reversal"
      Então observo os seguintes resultados
        | campo | valor |
        | daily_gross_principal | 10000 |

    @SC-BR-LIM-007-02
    Cenário: reembolso
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | approved_principal | 10000 |
        | action | "REFUND" |
      Quando avalio "limits.after_reversal"
      Então observo os seguintes resultados
        | campo | valor |
        | daily_gross_principal | 10000 |

  @BR-LIM-008
  Regra: BR-LIM-008 — Review consome quantidade mas não principal diário
    Especificação: Uma decisão REVIEW adiciona1 ao contador diário e zero ao principal bruto; DECLINED adiciona zero a ambos.

    @SC-BR-LIM-008-01
    Cenário: review
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | decision | "REVIEW" |
        | principal | 10000 |
      Quando avalio "limits.record_decision"
      Então observo os seguintes resultados
        | campo | valor |
        | count_delta | 1 |
        | gross_delta | 0 |

    @SC-BR-LIM-008-02
    Cenário: declined
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | decision | "DECLINED" |
        | principal | 10000 |
      Quando avalio "limits.record_decision"
      Então observo os seguintes resultados
        | campo | valor |
        | count_delta | 0 |
        | gross_delta | 0 |

    @SC-BR-LIM-008-03
    Cenário: approved
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | decision | "APPROVED" |
        | principal | 10000 |
      Quando avalio "limits.record_decision"
      Então observo os seguintes resultados
        | campo | valor |
        | count_delta | 1 |
        | gross_delta | 10000 |
