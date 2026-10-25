# language: pt
@cap_bat
Funcionalidade: Batch diário e reconciliação
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-BAT-001
  Regra: BR-BAT-001 — Batch preserva ordem de comandos
    Especificação: Executar na ordem recebida. Autorização que consumiu crédito afeta a seguinte; ordenar por request_id mudaria o comportamento e é proibido.

    @SC-BR-BAT-001-01
    Cenário: primeiro consome
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 10000 |
        | authorizations | [{"id":"Z","amount":6000},{"id":"A","amount":5000}] |
      Quando avalio "batch.order"
      Então observo os seguintes resultados
        | campo | valor |
        | decisions | ["APPROVED","DECLINED"] |
        | held | 6000 |

    @SC-BR-BAT-001-02
    Cenário: ordem inversa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 10000 |
        | authorizations | [{"id":"A","amount":5000},{"id":"Z","amount":6000}] |
      Quando avalio "batch.order"
      Então observo os seguintes resultados
        | campo | valor |
        | decisions | ["APPROVED","DECLINED"] |
        | held | 5000 |

  @BR-BAT-002
  Regra: BR-BAT-002 — Rejeição local não interrompe nem desfaz batch
    Especificação: Cada comando é atômico, o batch não. Uma rejeição financeira no meio preserva o anterior e permite processar o posterior.

    @SC-BR-BAT-002-01
    Cenário: sucesso recusa sucesso
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 10000 |
        | amounts | [3000,8000,4000] |
      Quando avalio "batch.continue"
      Então observo os seguintes resultados
        | campo | valor |
        | decisions | ["APPROVED","DECLINED","APPROVED"] |
        | held | 7000 |

    @SC-BR-BAT-002-02
    Cenário: primeira recusa não impede próxima
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | credit_limit | 10000 |
        | amounts | [11000,4000] |
      Quando avalio "batch.continue"
      Então observo os seguintes resultados
        | campo | valor |
        | decisions | ["DECLINED","APPROVED"] |
        | held | 4000 |

  @BR-BAT-003
  Regra: BR-BAT-003 — Replay em lote não conta como nova operação financeira
    Especificação: Repetir request_id/payload em posições diferentes retorna resposta original e não consome saldo adicional. A ordem das respostas ainda inclui o replay.

    @SC-BR-BAT-003-01
    Cenário: replay intercalado
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | requests | [{"id":"R1","amount":3000},{"id":"R2","amount":2000},{"id":"R1","amount":3000}] |
      Quando avalio "batch.replay"
      Então observo os seguintes resultados
        | campo | valor |
        | response_count | 3 |
        | auth_count | 2 |
        | held | 5000 |

    @SC-BR-BAT-003-02
    Cenário: replay adjacente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | requests | [{"id":"R1","amount":3000},{"id":"R1","amount":3000}] |
      Quando avalio "batch.replay"
      Então observo os seguintes resultados
        | campo | valor |
        | response_count | 2 |
        | auth_count | 1 |
        | held | 3000 |

  @BR-BAT-004
  Regra: BR-BAT-004 — Contas independentes têm projeções isoladas
    Especificação: Operação de A1 não altera dívida/reserva/pontos de A2. Agregações de risco, limite diário e pagamento são por conta, nunca globais.

    @SC-BR-BAT-004-01
    Cenário: autorizar na primeira
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | op_account | "A1" |
        | amount | 10000 |
      Quando avalio "batch.account_isolation"
      Então observo os seguintes resultados
        | campo | valor |
        | A1_held | 10000 |
        | A2_held | 0 |

    @SC-BR-BAT-004-02
    Cenário: autorizar na segunda
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | op_account | "A2" |
        | amount | 10000 |
      Quando avalio "batch.account_isolation"
      Então observo os seguintes resultados
        | campo | valor |
        | A1_held | 0 |
        | A2_held | 10000 |

  @BR-BAT-005
  Regra: BR-BAT-005 — Reconciliação soma recebíveis e reservas reais
    Especificação: RECONCILE deriva debt de todos os componentes de lotes ainda devidos e held de autorizações abertas; confere com projeções. Divergência é erro, não ajustada silenciosamente.

    @SC-BR-BAT-005-01
    Cenário: projeções corretas
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | lot_principal | [1000,2000] |
        | lot_fee | [20,30] |
        | late | [1000] |
        | holds | [5000,2000] |
        | cached_debt | 4050 |
        | cached_held | 7000 |
      Quando avalio "batch.reconcile"
      Então observo os seguintes resultados
        | campo | valor |
        | debt | 4050 |
        | held | 7000 |
        | reconciled | true |

    @SC-BR-BAT-005-02
    Cenário: cache divergente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | lot_principal | [1000] |
        | lot_fee | [20] |
        | late | [] |
        | holds | [5000] |
        | cached_debt | 999 |
        | cached_held | 5000 |
      Quando avalio "batch.reconcile"
      Então observo os seguintes resultados
        | campo | valor |
        | reconciled | false |
        | reason | "INVARIANT_VIOLATION" |

  @BR-BAT-006
  Regra: BR-BAT-006 — Todo evento financeiro é balanceado
    Especificação: Soma algébrica das linhas de cada event_id deve ser zero. Soma global zero não basta para esconder dois eventos individualmente quebrados.

    @SC-BR-BAT-006-01
    Cenário: eventos balanceados
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | events | [[100,-100],[20,-20]] |
      Quando avalio "batch.balance_check"
      Então observo os seguintes resultados
        | campo | valor |
        | balanced | true |

    @SC-BR-BAT-006-02
    Cenário: global zero mas eventos inválidos
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | events | [[100,-90],[20,-30]] |
      Quando avalio "batch.balance_check"
      Então observo os seguintes resultados
        | campo | valor |
        | balanced | false |

  @BR-BAT-007
  Regra: BR-BAT-007 — Relatório canônico ordena entidades e lotes
    Especificação: Resultados de consulta usam ordem ASCII de IDs e ordem normativa de lotes; não expor ordem de hash/endereços. Isso não ordena a execução do batch.

    @SC-BR-BAT-007-01
    Cenário: ordem lexicográfica
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | ids | ["Z2","A10","A2"] |
      Quando avalio "batch.report_order"
      Então observo os seguintes resultados
        | campo | valor |
        | ids | ["A10","A2","Z2"] |

    @SC-BR-BAT-007-02
    Cenário: ordem já canônica
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | ids | ["A10","A2","Z2"] |
      Quando avalio "batch.report_order"
      Então observo os seguintes resultados
        | campo | valor |
        | ids | ["A10","A2","Z2"] |

  @BR-BAT-008
  Regra: BR-BAT-008 — TICK é monótono e atomicamente expira todas as reservas
    Especificação: TICK para trás éINVALID_TIME. Mesmo instante não repete expirações. Falha de capacidade não altera nem relógio nem parte das reservas.

    @SC-BR-BAT-008-01
    Cenário: retrocesso
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | current_now | 1000 |
        | requested_now | 999 |
      Quando avalio "batch.tick"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "INVALID_TIME" |
        | unchanged | true |

    @SC-BR-BAT-008-02
    Cenário: mesmo instante
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | current_now | 1000 |
        | requested_now | 1000 |
        | open_holds | [] |
      Quando avalio "batch.tick"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "OK" |
        | events_added | 0 |

    @SC-BR-BAT-008-03
    Cenário: falha durante preparação
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | current_now | 999 |
        | requested_now | 1000 |
        | expiring_holds | [100,200] |
        | fail_at | "LEDGER_STORAGE" |
      Quando avalio "batch.tick"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CAPACITY" |
        | unchanged | true |
