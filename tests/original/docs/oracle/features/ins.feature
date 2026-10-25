# language: pt
@cap_ins
Funcionalidade: Parcelamento
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-INS-001
  Regra: BR-INS-001 — Parcelamento tem quantidade limitada
    Especificação: n inteiro em [1,12]. Valores fora do intervalo produzem INSTALLMENTS no gate de domínio, sem corrigir para o limite mais próximo.

    @SC-BR-INS-001-01
    Cenário: uma parcela
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | n | 1 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-INS-001-02
    Cenário: doze parcelas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 12000 |
        | n | 12 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-INS-001-03
    Cenário: zero parcelas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | n | 0 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INSTALLMENTS" |

    @SC-BR-INS-001-04
    Cenário: treze parcelas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 13000 |
        | n | 13 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INSTALLMENTS" |

  @BR-INS-002
  Regra: BR-INS-002 — Compra em país externo admite no máximo três parcelas
    Especificação: country!=BR implica n<=3, ainda que currency=BRL. País base permite até 12 se outros requisitos passarem.

    @SC-BR-INS-002-01
    Cenário: três fora do país
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 12000 |
        | country | "US" |
        | n | 3 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-INS-002-02
    Cenário: quatro fora do país
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 12000 |
        | country | "US" |
        | n | 4 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INSTALLMENTS" |

    @SC-BR-INS-002-03
    Cenário: doze no país base
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 12000 |
        | country | "BR" |
        | n | 12 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

  @BR-INS-003
  Regra: BR-INS-003 — Principal mínimo por parcela
    Especificação: Para n>1, floor(P/n)>=500. n=1 permite principal positivo abaixo de 500. Tarifa não pode completar o mínimo.

    @SC-BR-INS-003-01
    Cenário: igual ao mínimo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 1500 |
        | n | 3 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-INS-003-02
    Cenário: abaixo do mínimo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 1499 |
        | n | 3 |
        | fee | 100 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INSTALLMENTS" |

    @SC-BR-INS-003-03
    Cenário: à vista pequeno
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 1 |
        | n | 1 |
      Quando avalio "installment.admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

  @BR-INS-004
  Regra: BR-INS-004 — Resto do principal vai às primeiras parcelas
    Especificação: Dividir D emn partes: q=D//n, r=D%n, parte i=q+1 para i<r e q para as de mais, com índice começando em 0.

    @SC-BR-INS-004-01
    Cenário: dois centavos de resto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | total | 10001 |
        | n | 3 |
      Quando avalio "installment.split"
      Então observo os seguintes resultados
        | campo | valor |
        | parts | [3334,3334,3333] |

    @SC-BR-INS-004-02
    Cenário: sem resto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | total | 9000 |
        | n | 3 |
      Quando avalio "installment.split"
      Então observo os seguintes resultados
        | campo | valor |
        | parts | [3000,3000,3000] |

  @BR-INS-005
  Regra: BR-INS-005 — Tarifa é dividida independentemente do principal
    Especificação: Aplicar q/r à tarifa da captura separadamente; não calcular tarifa percentual por parcela. Zero pode ocorrer em parcela de tarifa.

    @SC-BR-INS-005-01
    Cenário: restos diferentes
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10001 |
        | fee | 5 |
        | n | 3 |
      Quando avalio "installment.split_components"
      Então observo os seguintes resultados
        | campo | valor |
        | principal_parts | [3334,3334,3333] |
        | fee_parts | [2,2,1] |

    @SC-BR-INS-005-02
    Cenário: tarifa menor que n
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 9000 |
        | fee | 1 |
        | n | 3 |
      Quando avalio "installment.split_components"
      Então observo os seguintes resultados
        | campo | valor |
        | fee_parts | [1,0,0] |

  @BR-INS-006
  Regra: BR-INS-006 — Agenda começa no ciclo da captura
    Especificação: Parcela i pertence a cycle(capture_day)+i; due_day=(cycle_i+1)*30+10. Autorização anterior não ancora a agenda.

    @SC-BR-INS-006-01
    Cenário: captura no dia100
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_day | 100 |
        | n | 3 |
      Quando avalio "installment.schedule"
      Então observo os seguintes resultados
        | campo | valor |
        | cycles | [3,4,5] |
        | due_days | [130,160,190] |

    @SC-BR-INS-006-02
    Cenário: fronteira do ciclo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_day | 120 |
        | n | 2 |
      Quando avalio "installment.schedule"
      Então observo os seguintes resultados
        | campo | valor |
        | cycles | [4,5] |
        | due_days | [160,190] |

  @BR-INS-007
  Regra: BR-INS-007 — Parcelas conservam componentes exatos
    Especificação: Somatórios de parcelas de principal e tarifa devem igualar os respectivos totais da captura. Não criar centavo de diferença contábil.

    @SC-BR-INS-007-01
    Cenário: três parcelas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10001 |
        | fee | 5 |
        | n | 3 |
      Quando avalio "installment.conservation"
      Então observo os seguintes resultados
        | campo | valor |
        | sum_principal | 10001 |
        | sum_fee | 5 |

    @SC-BR-INS-007-02
    Cenário: doze parcelas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 12001 |
        | fee | 11 |
        | n | 12 |
      Quando avalio "installment.conservation"
      Então observo os seguintes resultados
        | campo | valor |
        | sum_principal | 12001 |
        | sum_fee | 11 |

  @BR-INS-008
  Regra: BR-INS-008 — Captura parcial parcelada também respeita mínimo
    Especificação: Para autorização com n>1, cada captura d gera n lotes e exige floor(d/n)>=500; não usar P original para aprovar captura pequena.

    @SC-BR-INS-008-01
    Cenário: mínimo exato da captura
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | authorized_principal | 10000 |
        | capture_principal | 1500 |
        | n | 3 |
      Quando avalio "installment.capture_admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-INS-008-02
    Cenário: captura insuficiente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | authorized_principal | 10000 |
        | capture_principal | 1499 |
        | n | 3 |
      Quando avalio "installment.capture_admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INSTALLMENTS" |
