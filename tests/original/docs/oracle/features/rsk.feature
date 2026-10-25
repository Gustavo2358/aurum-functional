# language: pt
@cap_rsk
Funcionalidade: Risco determinístico
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-RSK-001
  Regra: BR-RSK-001 — Score começa no score-base válido
    Especificação: base_score em [0,100] participa do somatório; não substituir por probabilidade ou taxa externa. O resultado mínimo, sem componentes adicionais, é o score-base.

    @SC-BR-RSK-001-01
    Cenário: base padrão
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | principal | 10000 |
        | country | "BR" |
        | card_present | true |
        | recent_count | 0 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 10 |

    @SC-BR-RSK-001-02
    Cenário: base alta
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 79 |
        | principal | 10000 |
        | country | "BR" |
        | card_present | true |
        | recent_count | 0 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 79 |

  @BR-RSK-002
  Regra: BR-RSK-002 — País internacional acrescenta quinze pontos
    Especificação: country!=BR acrescenta15 ao score; moeda estrangeira sozinha não acrescenta esse componente.

    @SC-BR-RSK-002-01
    Cenário: país externo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | country | "US" |
        | currency | "BRL" |
        | principal | 10000 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 25 |

    @SC-BR-RSK-002-02
    Cenário: moeda externa no país base
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | country | "BR" |
        | currency | "USD" |
        | principal | 10000 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 10 |

  @BR-RSK-003
  Regra: BR-RSK-003 — Cartão ausente acrescenta vinte pontos
    Especificação: card_present=false soma20, independentemente do canal nominal. Não inferir presença do enum do canal.

    @SC-BR-RSK-003-01
    Cenário: ausente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | card_present | false |
        | principal | 10000 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 30 |

    @SC-BR-RSK-003-02
    Cenário: presente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | card_present | true |
        | principal | 10000 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 10 |

  @BR-RSK-004
  Regra: BR-RSK-004 — Montante alto inclui a fronteira
    Especificação: P>=100000 acrescenta15; P=99999 não. Usar principal convertido, não amount nominal.

    @SC-BR-RSK-004-01
    Cenário: abaixo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | principal | 99999 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 10 |

    @SC-BR-RSK-004-02
    Cenário: igual
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | principal | 100000 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 25 |

  @BR-RSK-005
  Regra: BR-RSK-005 — Velocidade conta uma janela inclusiva de eventos anteriores
    Especificação: Contar APPROVED/REVIEW já persistidos com now-60<=minute<=now; excluir DECLINED. Três ou mais acrescentam20. A tentativa corrente não entra antes do resultado.

    @SC-BR-RSK-005-01
    Cenário: fronteiras inclusivas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | now | 1000 |
        | history | [{"minute":939,"decision":"APPROVED"},{"minute":940,"decision":"APPROVED"},{"minute":999,"decision":"REVIEW"},{"minute":1000,"decision":"APPROVED"},{"minute":1000,"decision":"DECLINED"}] |
      Quando avalio "risk.velocity"
      Então observo os seguintes resultados
        | campo | valor |
        | recent_count | 3 |
        | component | 20 |

    @SC-BR-RSK-005-02
    Cenário: dois eventos
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | now | 1000 |
        | history | [{"minute":940,"decision":"APPROVED"},{"minute":999,"decision":"REVIEW"}] |
      Quando avalio "risk.velocity"
      Então observo os seguintes resultados
        | campo | valor |
        | recent_count | 2 |
        | component | 0 |

  @BR-RSK-006
  Regra: BR-RSK-006 — Faixas de decisão não se sobrepõem
    Especificação: score<50→APPROVED; 50<=score<80→REVIEW; score>=80→DECLINED. Essas saídas classificam risco, não pulam os gates de autorização.

    @SC-BR-RSK-006-01
    Cenário: último aprovado
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | score | 49 |
      Quando avalio "risk.classify"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "APPROVED" |

    @SC-BR-RSK-006-02
    Cenário: primeiro review
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | score | 50 |
      Quando avalio "risk.classify"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "REVIEW" |

    @SC-BR-RSK-006-03
    Cenário: último review
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | score | 79 |
      Quando avalio "risk.classify"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "REVIEW" |

    @SC-BR-RSK-006-04
    Cenário: primeiro declined
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | score | 80 |
      Quando avalio "risk.classify"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "DECLINED" |

  @BR-RSK-007
  Regra: BR-RSK-007 — Somatório é saturado em cem
    Especificação: Somar todos os componentes e aplicar min(100,total); não saturar por wraparound nem permitir score>100.

    @SC-BR-RSK-007-01
    Cenário: saturação
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 90 |
        | principal | 100000 |
        | country | "US" |
        | card_present | false |
        | recent_count | 3 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 100 |

    @SC-BR-RSK-007-02
    Cenário: sem saturação
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | principal | 100000 |
        | country | "US" |
        | card_present | false |
        | recent_count | 3 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 80 |

  @BR-RSK-008
  Regra: BR-RSK-008 — Componentes se acumulam mesmo quando nenhum basta
    Especificação: Não usar else-if entre componentes independentes. Internacional+remoto+alto valor com base10 produz 60, enquanto internacional+remoto abaixo do limite produz 45.

    @SC-BR-RSK-008-01
    Cenário: composição gera review
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | principal | 100000 |
        | country | "US" |
        | card_present | false |
        | recent_count | 0 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 60 |

    @SC-BR-RSK-008-02
    Cenário: abaixo do valor alto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 10 |
        | principal | 99999 |
        | country | "US" |
        | card_present | false |
        | recent_count | 0 |
      Quando avalio "risk.score"
      Então observo os seguintes resultados
        | campo | valor |
        | score | 45 |
