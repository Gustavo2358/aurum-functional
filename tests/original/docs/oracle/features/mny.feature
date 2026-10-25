# language: pt
@cap_mny
Funcionalidade: Dinheiro e aritmética
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-MNY-001
  Regra: BR-MNY-001 — Domínio monetário inteiro e limitado
    Especificação: Valores monetários não assinados válidos pertencem a [0, 1000000000000]. Negativo ou acima do máximo é NUMERIC_RANGE; zero é permitido na representação, embora comandos de pagamento/captura exijam positivo.

    @SC-BR-MNY-001-01
    Cenário: zero representável
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 0 |
      Quando avalio "money.validate"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |

    @SC-BR-MNY-001-02
    Cenário: máximo representável
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 1000000000000 |
      Quando avalio "money.validate"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |

    @SC-BR-MNY-001-03
    Cenário: acima do máximo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 1000000000001 |
      Quando avalio "money.validate"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | error | "NUMERIC_RANGE" |

    @SC-BR-MNY-001-04
    Cenário: negativo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | -1 |
      Quando avalio "money.validate"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | error | "NUMERIC_RANGE" |

  @BR-MNY-002
  Regra: BR-MNY-002 — Soma monetária verificada
    Especificação: A soma de dois valores monetários válidos só existe no modelo se o resultado não exceder MONEY_MAX. Validar antes de atualizar acumuladores.

    @SC-BR-MNY-002-01
    Cenário: soma exata
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | a | 999999999999 |
        | b | 1 |
      Quando avalio "money.add"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |
        | value | 1000000000000 |

    @SC-BR-MNY-002-02
    Cenário: excesso
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | a | 1000000000000 |
        | b | 1 |
      Quando avalio "money.add"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | error | "NUMERIC_RANGE" |

  @BR-MNY-003
  Regra: BR-MNY-003 — Subtração sem saldo negativo
    Especificação: Subtrair b de a requer a>=b e operandos monetários válidos. Underflow deve ser rejeitado, não convertido em unsigned enorme ou zero silencioso.

    @SC-BR-MNY-003-01
    Cenário: zera
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | a | 500 |
        | b | 500 |
      Quando avalio "money.subtract"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |
        | value | 0 |

    @SC-BR-MNY-003-02
    Cenário: saldo positivo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | a | 501 |
        | b | 500 |
      Quando avalio "money.subtract"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |
        | value | 1 |

    @SC-BR-MNY-003-03
    Cenário: underflow
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | a | 499 |
        | b | 500 |
      Quando avalio "money.subtract"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | error | "NUMERIC_RANGE" |

  @BR-MNY-004
  Regra: BR-MNY-004 — Percentual com truncamento inferior
    Especificação: floor_rate(v,bps)=floor(v*bps/10000), com v não negativo e bps em [0,10000]. Não arredondar meio centavo para cima.

    @SC-BR-MNY-004-01
    Cenário: fração descartada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 10001 |
        | bps | 200 |
      Quando avalio "money.floor_rate"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 200 |

    @SC-BR-MNY-004-02
    Cenário: menos de um centavo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 49 |
        | bps | 200 |
      Quando avalio "money.floor_rate"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 0 |

    @SC-BR-MNY-004-03
    Cenário: cem por cento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 123 |
        | bps | 10000 |
      Quando avalio "money.floor_rate"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 123 |

  @BR-MNY-005
  Regra: BR-MNY-005 — Percentual com arredondamento superior
    Especificação: ceil_rate(v,bps)=ceil(v*bps/10000). Zero permanece zero. Este operador é usado no pagamento mínimo, não nas tarifas.

    @SC-BR-MNY-005-01
    Cenário: fração elevada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 10001 |
        | bps | 1000 |
      Quando avalio "money.ceil_rate"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 1001 |

    @SC-BR-MNY-005-02
    Cenário: zero
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 0 |
        | bps | 1000 |
      Quando avalio "money.ceil_rate"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 0 |

    @SC-BR-MNY-005-03
    Cenário: divisão exata
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 10000 |
        | bps | 1000 |
      Quando avalio "money.ceil_rate"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 1000 |

  @BR-MNY-006
  Regra: BR-MNY-006 — Conversão com metade para cima
    Especificação: fx(v,rate)=floor((v*rate+5000)/10000). A metade exata sobe para o próximo centavo. Operandos e resultado respeitam os limites do domínio.

    @SC-BR-MNY-006-01
    Cenário: metade
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 1 |
        | rate | 15000 |
      Quando avalio "money.fx"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 2 |

    @SC-BR-MNY-006-02
    Cenário: abaixo de metade
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 1 |
        | rate | 14999 |
      Quando avalio "money.fx"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 1 |

    @SC-BR-MNY-006-03
    Cenário: acima de metade
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | value | 1 |
        | rate | 15001 |
      Quando avalio "money.fx"
      Então observo os seguintes resultados
        | campo | valor |
        | value | 2 |

  @BR-MNY-007
  Regra: BR-MNY-007 — Configuração de taxas validada
    Especificação: bps deve estar entre 0 e 10000 inclusive; FX deve estar entre 1 e 100000 inclusive. Taxa inválida não pode ser usada nem corrigida por clamp.

    @SC-BR-MNY-007-01
    Cenário: fronteiras válidas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | bps | 10000 |
        | fx_rate | 100000 |
      Quando avalio "money.validate_rates"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |

    @SC-BR-MNY-007-02
    Cenário: FX zero
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | bps | 200 |
        | fx_rate | 0 |
      Quando avalio "money.validate_rates"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |

    @SC-BR-MNY-007-03
    Cenário: percentual inválido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | bps | 10001 |
        | fx_rate | 50000 |
      Quando avalio "money.validate_rates"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |

    @SC-BR-MNY-007-04
    Cenário: FX excessivo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | bps | 200 |
        | fx_rate | 100001 |
      Quando avalio "money.validate_rates"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |

  @BR-MNY-008
  Regra: BR-MNY-008 — Intermediários aritméticos não transbordam
    Especificação: A multiplicação verificada de inteiros int64 não negativos retorna NUMERIC_RANGE se exceder INT64_MAX. Nunca executar signed overflow para então tentar detectá-lo. O limite monetário final continua sendo outro cheque.

    @SC-BR-MNY-008-01
    Cenário: intermediário usado por FX
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | a | 1000000000000 |
        | b | 100000 |
      Quando avalio "money.checked_mul"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |
        | value | 100000000000000000 |

    @SC-BR-MNY-008-02
    Cenário: overflow int64
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | a | 9223372036854775807 |
        | b | 2 |
      Quando avalio "money.checked_mul"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | error | "NUMERIC_RANGE" |
