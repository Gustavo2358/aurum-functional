# language: pt
@cap_aut
Funcionalidade: Autorização
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-AUT-001
  Regra: BR-AUT-001 — Primeira causa determina a rejeição
    Especificação: Seguir a precedência de OPERATIONS.md. Conta inativa vence insuficiência de crédito; limite de crédito vence score alto. Não informar a última guarda avaliada.

    @SC-BR-AUT-001-01
    Cenário: elegibilidade vence crédito
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | account.active | false |
        | credit_limit | 0 |
      Quando avalio "auth.decide"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "DECLINED" |
        | reason | "ACCOUNT_INACTIVE" |

    @SC-BR-AUT-001-02
    Cenário: crédito vence risco
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 0 |
        | base_score | 90 |
      Quando avalio "auth.decide"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "DECLINED" |
        | reason | "CREDIT_LIMIT" |

  @BR-AUT-002
  Regra: BR-AUT-002 — Aprovação cria reserva total
    Especificação: Aprovação cria uma autorização e reserva P+F, com state=ACTIVE e capturado zero. Reserva deve aparecer no crédito disponível.

    @SC-BR-AUT-002-01
    Cenário: doméstica
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "BRL" |
      Quando avalio "auth.approve"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "APPROVED" |
        | held | 10000 |
        | available | 990000 |
        | state | "ACTIVE" |

    @SC-BR-AUT-002-02
    Cenário: estrangeira
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "USD" |
      Quando avalio "auth.approve"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "APPROVED" |
        | held | 51000 |
        | available | 949000 |
        | state | "ACTIVE" |

  @BR-AUT-003
  Regra: BR-AUT-003 — Autorização sozinha não cria recebível nem pontos
    Especificação: Em APPROVED, debt e pontos permanecem inalterados; a dívida e recompensa surgem na captura.

    @SC-BR-AUT-003-01
    Cenário: sem captura
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
      Quando avalio "auth.approve"
      Então observo os seguintes resultados
        | campo | valor |
        | debt | 0 |
        | points | 0 |
        | held | 10000 |

    @SC-BR-AUT-003-02
    Cenário: valor diferente sem captura
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 20000 |
      Quando avalio "auth.approve"
      Então observo os seguintes resultados
        | campo | valor |
        | debt | 0 |
        | points | 0 |
        | held | 20000 |

  @BR-AUT-004
  Regra: BR-AUT-004 — Rejeição não produz efeitos financeiros
    Especificação: DECLINED não altera reserva, dívida, diário financeiro, pontos nem contadores de valor/quantidade. Pode persistir resposta de idempotência e log operacional.

    @SC-BR-AUT-004-01
    Cenário: insuficiência
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 9999 |
        | amount | 10000 |
      Quando avalio "auth.decide"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "DECLINED" |
        | financial_unchanged | true |
        | count_delta | 0 |

    @SC-BR-AUT-004-02
    Cenário: risco alto
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 90 |
        | amount | 10000 |
      Quando avalio "auth.decide"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "DECLINED" |
        | financial_unchanged | true |
        | count_delta | 0 |

  @BR-AUT-005
  Regra: BR-AUT-005 — Review é terminal sem reserva
    Especificação: REVIEW não cria autorização nem dívida/hold/pontos; soma1 à quantidade diária e à velocidade. Não é aprovação condicional financeira.

    @SC-BR-AUT-005-01
    Cenário: score review
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 50 |
        | amount | 10000 |
      Quando avalio "auth.decide"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "REVIEW" |
        | held | 0 |
        | debt | 0 |
        | points | 0 |
        | auth_created | false |
        | count_delta | 1 |

    @SC-BR-AUT-005-02
    Cenário: último score review
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | base_score | 79 |
        | amount | 10000 |
      Quando avalio "auth.decide"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "REVIEW" |
        | held | 0 |
        | auth_created | false |

  @BR-AUT-006
  Regra: BR-AUT-006 — Prazo de reserva deriva do instante explícito
    Especificação: expires_at=approved_at+1440 minutos. O relógio de parede não participa e a mesma política vale na virada do dia.

    @SC-BR-AUT-006-01
    Cenário: prazo padrão
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | now | 144600 |
      Quando avalio "auth.approve"
      Então observo os seguintes resultados
        | campo | valor |
        | approved_at | 144600 |
        | expires_at | 146040 |

    @SC-BR-AUT-006-02
    Cenário: virada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | now | 145439 |
      Quando avalio "auth.approve"
      Então observo os seguintes resultados
        | campo | valor |
        | expires_at | 146879 |

  @BR-AUT-007
  Regra: BR-AUT-007 — Aprovação usa a mesma cotação observável
    Especificação: No mesmo estado/configuração, P e F aprovados devem corresponder ao QUOTE da mesma entrada; armazenar os valores para a captura cumulativa, sem recotação.

    @SC-BR-AUT-007-01
    Cenário: regular estrangeiro
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "USD" |
        | installments | 1 |
      Quando avalio "auth.quote_consistency"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 50000 |
        | fee | 1000 |
        | same_quote | true |

    @SC-BR-AUT-007-02
    Cenário: premium parcelado doméstico
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | amount | 10000 |
        | currency | "BRL" |
        | tier | "PREMIUM" |
        | installments | 3 |
      Quando avalio "auth.quote_consistency"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 10000 |
        | fee | 100 |
        | same_quote | true |

  @BR-AUT-008
  Regra: BR-AUT-008 — Preflight torna a aprovação indivisível
    Especificação: Se não houver capacidade para autorização, diário ou idempotência, retornar ERROR/CAPACITY sem reserva, contador, chave ou resultado parcialmente publicado. Reserva de memória ocorre antes do commit.

    @SC-BR-AUT-008-01
    Cenário: sem espaço de autorização
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | fail_at | "AUTH_STORAGE" |
      Quando avalio "auth.capacity"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CAPACITY" |
        | unchanged | true |

    @SC-BR-AUT-008-02
    Cenário: sem espaço de idempotência
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | fail_at | "IDEMPOTENCY_STORAGE" |
      Quando avalio "auth.capacity"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CAPACITY" |
        | unchanged | true |
