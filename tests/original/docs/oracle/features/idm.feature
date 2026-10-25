# language: pt
@cap_idm
Funcionalidade: Idempotência
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-IDM-001
  Regra: BR-IDM-001 — Replay exato devolve a resposta original
    Especificação: Mesma operação, request_id e payload canônico devolvem a resposta armazenada exatamente, sem novo processamento financeiro.

    @SC-BR-IDM-001-01
    Cenário: autorizar duas vezes
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | op | "AUTHORIZE" |
        | repeat | 2 |
        | amount | 10000 |
      Quando avalio "idempotency.replay"
      Então observo os seguintes resultados
        | campo | valor |
        | same_response | true |
        | held | 10000 |
        | auth_count | 1 |

    @SC-BR-IDM-001-02
    Cenário: três replays
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | op | "AUTHORIZE" |
        | repeat | 4 |
        | amount | 10000 |
      Quando avalio "idempotency.replay"
      Então observo os seguintes resultados
        | campo | valor |
        | same_response | true |
        | held | 10000 |
        | auth_count | 1 |

  @BR-IDM-002
  Regra: BR-IDM-002 — Chave repetida com outra intenção é conflito
    Especificação: Alterar qualquer campo semântico do payload para uma chave existente retorna IDEMPOTENCY_CONFLICT e preserva o primeiro resultado/estado.

    @SC-BR-IDM-002-01
    Cenário: montante alterado
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first_amount | 10000 |
        | second_amount | 10001 |
      Quando avalio "idempotency.conflict"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "IDEMPOTENCY_CONFLICT" |
        | held | 10000 |

    @SC-BR-IDM-002-02
    Cenário: moeda alterada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first_currency | "BRL" |
        | second_currency | "USD" |
        | amount | 10000 |
      Quando avalio "idempotency.conflict"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "IDEMPOTENCY_CONFLICT" |
        | held | 10000 |

  @BR-IDM-003
  Regra: BR-IDM-003 — Canonicalização elimina diferenças só de escrita
    Especificação: Ordem de chaves e zeros à esquerda não mudam o payload depois de parse/validação. Campos default omitidos e seus valores explícitos são equivalentes.

    @SC-BR-IDM-003-01
    Cenário: ordem e zeros
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first | {"amount":"010000","currency":"BRL"} |
        | second | {"currency":"BRL","amount":"10000"} |
      Quando avalio "idempotency.canonical"
      Então observo os seguintes resultados
        | campo | valor |
        | same_payload | true |

    @SC-BR-IDM-003-02
    Cenário: default explícito
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first | {"amount":"10000"} |
        | second | {"amount":"10000","installments":"1"} |
      Quando avalio "idempotency.canonical"
      Então observo os seguintes resultados
        | campo | valor |
        | same_payload | true |

  @BR-IDM-004
  Regra: BR-IDM-004 — Namespace de chave inclui o tipo de operação
    Especificação: A chave é(op,request_id), não apenas request_id. Mesmo identificador textual pode ser usado em AUTHORIZE e CAPTURE; referências têm o tipo da entidade.

    @SC-BR-IDM-004-01
    Cenário: tipos diferentes
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first_op | "AUTHORIZE" |
        | second_op | "CAPTURE" |
        | request_id | "R1" |
      Quando avalio "idempotency.namespace"
      Então observo os seguintes resultados
        | campo | valor |
        | same_key | false |

    @SC-BR-IDM-004-02
    Cenário: mesmo tipo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first_op | "AUTHORIZE" |
        | second_op | "AUTHORIZE" |
        | request_id | "R1" |
      Quando avalio "idempotency.namespace"
      Então observo os seguintes resultados
        | campo | valor |
        | same_key | true |

  @BR-IDM-005
  Regra: BR-IDM-005 — Erros técnicos e de envelope não consomem chave
    Especificação: INVALID_INPUT,NOT_FOUND,OWNERSHIP,CAPACITY e IDEMPOTENCY_CONFLICT não criam nova entrada no cache. Uma tentativa corrigida pode reutilizar a chave que nunca foi registrada.

    @SC-BR-IDM-005-01
    Cenário: invalid input
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | reason | "INVALID_INPUT" |
      Quando avalio "idempotency.error_policy"
      Então observo os seguintes resultados
        | campo | valor |
        | cache_response | false |

    @SC-BR-IDM-005-02
    Cenário: capacity
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | reason | "CAPACITY" |
      Quando avalio "idempotency.error_policy"
      Então observo os seguintes resultados
        | campo | valor |
        | cache_response | false |

    @SC-BR-IDM-005-03
    Cenário: not found
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | reason | "NOT_FOUND" |
      Quando avalio "idempotency.error_policy"
      Então observo os seguintes resultados
        | campo | valor |
        | cache_response | false |

  @BR-IDM-006
  Regra: BR-IDM-006 — Rejeições válidas também são memorizadas
    Especificação: Uma requisição estruturalmente válida recusada por regra de domínio fica registrada; repetir com a mesma chave retorna a mesma decisão, não tenta aproveitar mudança posterior de saldo/tempo.

    @SC-BR-IDM-006-01
    Cenário: recusa de crédito persistida
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first_credit_limit | 9999 |
        | later_credit_limit | 1000000 |
        | amount | 10000 |
      Quando avalio "idempotency.declined_replay"
      Então observo os seguintes resultados
        | campo | valor |
        | first_decision | "DECLINED" |
        | second_decision | "DECLINED" |
        | same_response | true |

    @SC-BR-IDM-006-02
    Cenário: review persistido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | first_base_score | 50 |
        | later_base_score | 10 |
      Quando avalio "idempotency.declined_replay"
      Então observo os seguintes resultados
        | campo | valor |
        | first_decision | "REVIEW" |
        | second_decision | "REVIEW" |
        | same_response | true |

  @BR-IDM-007
  Regra: BR-IDM-007 — Replay não duplica efeitos indiretos
    Especificação: Repetição não duplica contagem de risco, consumo diário, pontos, lotes ou lançamentos. Verificar o conjunto de efeitos, não apenas o saldo principal.

    @SC-BR-IDM-007-01
    Cenário: captura repetida
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 20000 |
        | repeat | 2 |
      Quando avalio "idempotency.capture_replay"
      Então observo os seguintes resultados
        | campo | valor |
        | debt | 20000 |
        | points | 2 |
        | capture_count | 1 |
        | daily_count | 1 |

    @SC-BR-IDM-007-02
    Cenário: captura repetida várias vezes
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | principal | 20000 |
        | repeat | 5 |
      Quando avalio "idempotency.capture_replay"
      Então observo os seguintes resultados
        | campo | valor |
        | debt | 20000 |
        | points | 2 |
        | capture_count | 1 |
        | daily_count | 1 |

  @BR-IDM-008
  Regra: BR-IDM-008 — Consulta não precisa de chave de mutação
    Especificação: QUOTE,GET e RECONCILE não consomem idempotência. Repeti-los não altera a capacidade disponível para chaves financeiras.

    @SC-BR-IDM-008-01
    Cenário: quote
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | op | "QUOTE" |
        | repeat | 3 |
      Quando avalio "idempotency.query"
      Então observo os seguintes resultados
        | campo | valor |
        | idempotency_delta | 0 |
        | financial_unchanged | true |

    @SC-BR-IDM-008-02
    Cenário: reconcile
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | op | "RECONCILE" |
        | repeat | 3 |
      Quando avalio "idempotency.query"
      Então observo os seguintes resultados
        | campo | valor |
        | idempotency_delta | 0 |
        | financial_unchanged | true |
