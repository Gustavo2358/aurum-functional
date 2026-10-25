# language: pt
@cap_rew
Funcionalidade: Recompensas
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-REW-001
  Regra: BR-REW-001 — Cliente regular ganha pontos por principal capturado
    Especificação: REGULAR,NORMAL recebe floor(D/10000) pontos antes do cap. Tarifa não gera pontos; autorização sem captura não gera pontos.

    @SC-BR-REW-001-01
    Cenário: um ponto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | fee | 200 |
        | tier | "REGULAR" |
        | category | "NORMAL" |
      Quando avalio "rewards.raw"
      Então observo os seguintes resultados
        | campo | valor |
        | raw_points | 1 |

    @SC-BR-REW-001-02
    Cenário: abaixo de um ponto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 9999 |
        | fee | 5000 |
        | tier | "REGULAR" |
        | category | "NORMAL" |
      Quando avalio "rewards.raw"
      Então observo os seguintes resultados
        | campo | valor |
        | raw_points | 0 |

  @BR-REW-002
  Regra: BR-REW-002 — Premium multiplica depois do truncamento
    Especificação: PREMIUM,NORMAL recebe2*floor(D/10000), não floor(2*D/10000). Essa diferença em frações é intencional.

    @SC-BR-REW-002-01
    Cenário: fração não rende um ponto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 9999 |
        | tier | "PREMIUM" |
        | category | "NORMAL" |
      Quando avalio "rewards.raw"
      Então observo os seguintes resultados
        | campo | valor |
        | raw_points | 0 |

    @SC-BR-REW-002-02
    Cenário: uma unidade rende dois
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 10000 |
        | tier | "PREMIUM" |
        | category | "NORMAL" |
      Quando avalio "rewards.raw"
      Então observo os seguintes resultados
        | campo | valor |
        | raw_points | 2 |

  @BR-REW-003
  Regra: BR-REW-003 — Categoria restrita não pontua
    Especificação: Uma captura com categoria de política RESTRICTED produz zero pontos. A autorização normal já a rejeita; esta regra defensiva tem escopo de cálculo/estados históricos válidos e não afirma alcance no fluxo normal sem bypass.

    @SC-BR-REW-003-01
    Cenário: restrita premium
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 100000 |
        | tier | "PREMIUM" |
        | category | "RESTRICTED" |
      Quando avalio "rewards.raw"
      Então observo os seguintes resultados
        | campo | valor |
        | raw_points | 0 |

    @SC-BR-REW-003-02
    Cenário: normal premium
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 100000 |
        | tier | "PREMIUM" |
        | category | "NORMAL" |
      Quando avalio "rewards.raw"
      Então observo os seguintes resultados
        | campo | valor |
        | raw_points | 20 |

  @BR-REW-004
  Regra: BR-REW-004 — Cap limita pontos efetivamente concedidos
    Especificação: granted=min(raw_points,max(0,1000-gross_granted_in_cycle)). Não conceder valor negativo nem aplicar cap ao principal monetário.

    @SC-BR-REW-004-01
    Cenário: últimos pontos disponíveis
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw_points | 10 |
        | gross_granted | 995 |
      Quando avalio "rewards.cap"
      Então observo os seguintes resultados
        | campo | valor |
        | granted | 5 |

    @SC-BR-REW-004-02
    Cenário: cap cheio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw_points | 10 |
        | gross_granted | 1000 |
      Quando avalio "rewards.cap"
      Então observo os seguintes resultados
        | campo | valor |
        | granted | 0 |

    @SC-BR-REW-004-03
    Cenário: sem cap próximo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw_points | 10 |
        | gross_granted | 0 |
      Quando avalio "rewards.cap"
      Então observo os seguintes resultados
        | campo | valor |
        | granted | 10 |

  @BR-REW-005
  Regra: BR-REW-005 — Cap usa ciclo da captura
    Especificação: Gross concedido em ciclo anterior não reduz cap do ciclo atual. A autorização pode ter sido feita antes; período é da captura.

    @SC-BR-REW-005-01
    Cenário: novo ciclo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_day | 120 |
        | grants | [{"cycle":3,"points":1000}] |
        | raw_points | 10 |
      Quando avalio "rewards.cycle_cap"
      Então observo os seguintes resultados
        | campo | valor |
        | granted | 10 |

    @SC-BR-REW-005-02
    Cenário: mesmo ciclo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | capture_day | 119 |
        | grants | [{"cycle":3,"points":1000}] |
        | raw_points | 10 |
      Quando avalio "rewards.cycle_cap"
      Então observo os seguintes resultados
        | campo | valor |
        | granted | 0 |

  @BR-REW-006
  Regra: BR-REW-006 — Reversão proporcional usa concessão real
    Especificação: Pontos a reverter=floor(granted*(r+d)/D)-floor(granted*r/D). Usar granted pós-cap, não raw; reembolso total reverte todos os concedidos.

    @SC-BR-REW-006-01
    Cenário: concessão reduzida por cap
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 100000 |
        | granted | 3 |
        | refunded | 0 |
        | amount | 50000 |
      Quando avalio "rewards.refund"
      Então observo os seguintes resultados
        | campo | valor |
        | reversed | 1 |

    @SC-BR-REW-006-02
    Cenário: metade final
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 100000 |
        | granted | 3 |
        | refunded | 50000 |
        | amount | 50000 |
      Quando avalio "rewards.refund"
      Então observo os seguintes resultados
        | campo | valor |
        | reversed | 2 |

    @SC-BR-REW-006-03
    Cenário: integral
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 100000 |
        | granted | 3 |
        | refunded | 0 |
        | amount | 100000 |
      Quando avalio "rewards.refund"
      Então observo os seguintes resultados
        | campo | valor |
        | reversed | 3 |

  @BR-REW-007
  Regra: BR-REW-007 — Reembolso não libera cap bruto
    Especificação: Reverter pontos reduz saldo líquido, mas gross_granted_in_cycle permanece. Nova captura no mesmo ciclo não readquire o espaço devolvido.

    @SC-BR-REW-007-01
    Cenário: cap cheio antes do estorno
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | gross_granted | 1000 |
        | points_reversed | 100 |
        | new_raw_points | 10 |
      Quando avalio "rewards.after_refund"
      Então observo os seguintes resultados
        | campo | valor |
        | gross_granted | 1000 |
        | new_granted | 0 |

    @SC-BR-REW-007-02
    Cenário: espaço não depende do estorno
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | gross_granted | 990 |
        | points_reversed | 100 |
        | new_raw_points | 20 |
      Quando avalio "rewards.after_refund"
      Então observo os seguintes resultados
        | campo | valor |
        | new_granted | 10 |

  @BR-REW-008
  Regra: BR-REW-008 — Arredondamento é por captura, não pela autorização
    Especificação: Duas capturas menores podem somar menos pontos que uma captura única, pois floor é aplicado em cada D. Não impor metamorfismo falso de invariância por divisão.

    @SC-BR-REW-008-01
    Cenário: uma captura
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | captures | [10000] |
        | tier | "REGULAR" |
      Quando avalio "rewards.capture_partition"
      Então observo os seguintes resultados
        | campo | valor |
        | points | 1 |

    @SC-BR-REW-008-02
    Cenário: duas capturas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | captures | [5000,5000] |
        | tier | "REGULAR" |
      Quando avalio "rewards.capture_partition"
      Então observo os seguintes resultados
        | campo | valor |
        | points | 0 |
