# language: pt
@cap_elg
Funcionalidade: Elegibilidade
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-ELG-001
  Regra: BR-ELG-001 — Conta inativa impede autorização
    Especificação: No gate de elegibilidade, conta inativa impede autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

    @SC-BR-ELG-001-01
    Cenário: impedimento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | account.active | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "ACCOUNT_INACTIVE" |

    @SC-BR-ELG-001-02
    Cenário: perfil elegível
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | profile | "base" |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |
        | reason | "NONE" |

  @BR-ELG-002
  Regra: BR-ELG-002 — Cartão bloqueado impede autorização
    Especificação: No gate de elegibilidade, cartão bloqueado impede autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

    @SC-BR-ELG-002-01
    Cenário: impedimento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | card.status | "BLOCKED" |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "CARD_BLOCKED" |

    @SC-BR-ELG-002-02
    Cenário: perfil elegível
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | profile | "base" |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |
        | reason | "NONE" |

  @BR-ELG-003
  Regra: BR-ELG-003 — Validade do cartão inclui o último dia
    Especificação: Cartão é válido quando day<=expiry_day. Expiração de cartão não se confunde com TTL da autorização.

    @SC-BR-ELG-003-01
    Cenário: dia final válido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 500 |
        | card.expiry_day | 500 |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |

    @SC-BR-ELG-003-02
    Cenário: dia seguinte inválido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 501 |
        | card.expiry_day | 500 |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "CARD_EXPIRED" |

  @BR-ELG-004
  Regra: BR-ELG-004 — Lojista bloqueado impede autorização
    Especificação: No gate de elegibilidade, lojista bloqueado impede autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

    @SC-BR-ELG-004-01
    Cenário: impedimento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | merchant.status | "BLOCKED" |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "MERCHANT_BLOCKED" |

    @SC-BR-ELG-004-02
    Cenário: perfil elegível
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | profile | "base" |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |
        | reason | "NONE" |

  @BR-ELG-005
  Regra: BR-ELG-005 — Categoria restrita impede nova autorização
    Especificação: No gate de elegibilidade, categoria restrita impede nova autorização. Gates anteriores têm precedência. A decisão é DECLINED com a razão correspondente, sem efeitos financeiros.

    @SC-BR-ELG-005-01
    Cenário: impedimento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | merchant.category | "RESTRICTED" |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "CATEGORY_RESTRICTED" |

    @SC-BR-ELG-005-02
    Cenário: perfil elegível
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | profile | "base" |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |
        | reason | "NONE" |

  @BR-ELG-006
  Regra: BR-ELG-006 — Permissão internacional depende do país
    Especificação: Se country!=BR e card.allow_international=false, rejeitar. Moeda USD em country=BR não aciona essa restrição de país.

    @SC-BR-ELG-006-01
    Cenário: país externo não permitido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | country | "US" |
        | card.allow_international | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "INTERNATIONAL_DISABLED" |

    @SC-BR-ELG-006-02
    Cenário: moeda estrangeira no país base
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | country | "BR" |
        | currency | "USD" |
        | card.allow_international | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |

  @BR-ELG-007
  Regra: BR-ELG-007 — Permissão de aproximação é específica do canal
    Especificação: channel=CONTACTLESS exige card.allow_contactless=true. Proibição não bloqueia POS normal.

    @SC-BR-ELG-007-01
    Cenário: aproximação bloqueada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | channel | "CONTACTLESS" |
        | card.allow_contactless | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "CONTACTLESS_DISABLED" |

    @SC-BR-ELG-007-02
    Cenário: POS permitido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | channel | "POS" |
        | card.allow_contactless | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |

  @BR-ELG-008
  Regra: BR-ELG-008 — PIN exigido acima do limite presencial
    Especificação: Quando card_present=true e amount nominal>10000, pin_ok deve ser true. O limiar é deliberadamente nominal na moeda de entrada, antes da cotação; não usar P convertido. Quando card_present=false, esta guarda de PIN presencial não se aplica.

    @SC-BR-ELG-008-01
    Cenário: fronteira sem PIN
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | pin_ok | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |

    @SC-BR-ELG-008-02
    Cenário: acima da fronteira sem PIN
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10001 |
        | pin_ok | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "PIN_REQUIRED" |

    @SC-BR-ELG-008-03
    Cenário: remoto não exige PIN presencial
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10001 |
        | card_present | false |
        | pin_ok | false |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |

  @BR-ELG-009
  Regra: BR-ELG-009 — Saldo vencido impede nova autorização
    Especificação: Qualquer fatura da conta com day>due_day e outstanding>0 causa PAST_DUE. No dia do vencimento ainda não há atraso; pagamento mínimo não elimina saldo vencido.

    @SC-BR-ELG-009-01
    Cenário: vencimento inclusivo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 130 |
        | invoices | [{"due_day":130,"outstanding":100}] |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |

    @SC-BR-ELG-009-02
    Cenário: vencida com saldo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 131 |
        | invoices | [{"due_day":130,"outstanding":100}] |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | false |
        | reason | "PAST_DUE" |

    @SC-BR-ELG-009-03
    Cenário: vencida quitada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 131 |
        | invoices | [{"due_day":130,"outstanding":0}] |
      Quando avalio "eligibility.check"
      Então observo os seguintes resultados
        | campo | valor |
        | eligible | true |
