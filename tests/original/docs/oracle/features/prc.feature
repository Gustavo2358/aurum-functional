# language: pt
@cap_prc
Funcionalidade: Cotação e tarifas
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-PRC-001
  Regra: BR-PRC-001 — Moeda base não altera principal
    Especificação: BRL usa fator10000; o principal cotado é exatamente o montante solicitado, sem reconversão adicional.

    @SC-BR-PRC-001-01
    Cenário: um centavo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 1 |
        | currency | "BRL" |
      Quando avalio "pricing.quote"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 1 |

    @SC-BR-PRC-001-02
    Cenário: montante base
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10001 |
        | currency | "BRL" |
      Quando avalio "pricing.quote"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 10001 |

  @BR-PRC-002
  Regra: BR-PRC-002 — Cotação estrangeira usa taxa explícita
    Especificação: USD usa50000 e EUR60000 no perfil base. A conversão produz principal em BRL com half-up; taxas são fixas sintéticas.

    @SC-BR-PRC-002-01
    Cenário: USD
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 123 |
        | currency | "USD" |
      Quando avalio "pricing.quote"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 615 |

    @SC-BR-PRC-002-02
    Cenário: EUR
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 123 |
        | currency | "EUR" |
      Quando avalio "pricing.quote"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 738 |

  @BR-PRC-003
  Regra: BR-PRC-003 — Moeda sem taxa não recebe fallback
    Especificação: Uma moeda sem taxa não pode ser tratada como BRL nem inferida de country. A cotação falha com UNSUPPORTED_CURRENCY; AUTHORIZE traduz isso para DECLINED.

    @SC-BR-PRC-003-01
    Cenário: moeda ausente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "JPY" |
      Quando avalio "pricing.quote"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | reason | "UNSUPPORTED_CURRENCY" |

    @SC-BR-PRC-003-02
    Cenário: moeda existente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "BRL" |
      Quando avalio "pricing.quote"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |
        | principal | 10000 |

  @BR-PRC-004
  Regra: BR-PRC-004 — Tarifa cambial de cliente regular
    Especificação: REGULAR em currency!=BRL paga floor(P*200/10000) na componente internacional. O país não ativa essa tarifa sozinho.

    @SC-BR-PRC-004-01
    Cenário: regular USD
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10001 |
        | currency | "USD" |
        | tier | "REGULAR" |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | international_fee | 200 |

    @SC-BR-PRC-004-02
    Cenário: BRL no exterior
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10001 |
        | currency | "BRL" |
        | country | "US" |
        | tier | "REGULAR" |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | international_fee | 0 |

  @BR-PRC-005
  Regra: BR-PRC-005 — Isenção premium é específica
    Especificação: PREMIUM zera somente international_fee. Não zera installment_fee, principal nem multa.

    @SC-BR-PRC-005-01
    Cenário: premium parcelado estrangeiro
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | currency | "USD" |
        | tier | "PREMIUM" |
        | installments | 3 |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | international_fee | 0 |
        | installment_fee | 100 |
        | fee | 100 |

    @SC-BR-PRC-005-02
    Cenário: regular equivalente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | currency | "USD" |
        | tier | "REGULAR" |
        | installments | 3 |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | international_fee | 200 |
        | installment_fee | 100 |
        | fee | 300 |

  @BR-PRC-006
  Regra: BR-PRC-006 — Tarifa por parcelas adicionais
    Especificação: installment_fee=floor(P*50*(n-1)/10000), em cálculo único. Não arredondar separadamente cada parcela adicional.

    @SC-BR-PRC-006-01
    Cenário: à vista
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10001 |
        | installments | 1 |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | installment_fee | 0 |

    @SC-BR-PRC-006-02
    Cenário: três parcelas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10001 |
        | installments | 3 |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | installment_fee | 100 |

    @SC-BR-PRC-006-03
    Cenário: frações combinadas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 599 |
        | installments | 3 |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | installment_fee | 5 |

  @BR-PRC-007
  Regra: BR-PRC-007 — Teto incide depois da soma de componentes
    Especificação: F=min(5000, international_fee+installment_fee). As componentes calculadas continuam observáveis, sem aplicar teto em cada uma.

    @SC-BR-PRC-007-01
    Cenário: teto atingido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 100000 |
        | currency | "USD" |
        | tier | "REGULAR" |
        | installments | 12 |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | international_fee | 2000 |
        | installment_fee | 5500 |
        | fee | 5000 |

    @SC-BR-PRC-007-02
    Cenário: abaixo do teto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | currency | "BRL" |
        | installments | 3 |
      Quando avalio "pricing.components"
      Então observo os seguintes resultados
        | campo | valor |
        | fee | 100 |

  @BR-PRC-008
  Regra: BR-PRC-008 — Cotação não compromete a conta
    Especificação: QUOTE produz preço, mas não reserva limite, cria dívida, soma consumo diário ou concede pontos. Consultas repetidas no mesmo estado/configuração são iguais.

    @SC-BR-PRC-008-01
    Cenário: cotação única
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "USD" |
        | repeat | 1 |
      Quando avalio "pricing.purity"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 50000 |
        | fee | 1000 |
        | financial_unchanged | true |

    @SC-BR-PRC-008-02
    Cenário: cotação repetida
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "USD" |
        | repeat | 3 |
      Quando avalio "pricing.purity"
      Então observo os seguintes resultados
        | campo | valor |
        | same_response | true |
        | financial_unchanged | true |
