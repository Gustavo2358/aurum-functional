# language: pt
@cap_bil
Funcionalidade: Faturas pagamentos e multa
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @BR-BIL-001
  Regra: BR-BIL-001 — Ciclo só fecha depois de terminar
    Especificação: CLOSE de cycle exige day>=(cycle+1)*30; equality passa. Não antecipar fechamento de ciclo em andamento.

    @SC-BR-BIL-001-01
    Cenário: último dia em andamento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 3 |
        | day | 119 |
      Quando avalio "billing.close_time"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "CYCLE_NOT_ENDED" |

    @SC-BR-BIL-001-02
    Cenário: instante de fechamento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 3 |
        | day | 120 |
      Quando avalio "billing.close_time"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

  @BR-BIL-002
  Regra: BR-BIL-002 — Fechamento respeita ordem e é único
    Especificação: Primeiro fechamento usa ciclo de criação; depois só next_cycle_to_close. Ciclo anterior já fechado com nova chave éNOOP; pular ciclo éCYCLE_ORDER.

    @SC-BR-BIL-002-01
    Cenário: próximo ciclo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | requested | 3 |
        | next_cycle | 3 |
      Quando avalio "billing.close_order"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "OK" |

    @SC-BR-BIL-002-02
    Cenário: ciclo já fechado
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | requested | 2 |
        | next_cycle | 3 |
        | already_closed | true |
      Quando avalio "billing.close_order"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "NOOP" |

    @SC-BR-BIL-002-03
    Cenário: pulo
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | requested | 4 |
        | next_cycle | 3 |
      Quando avalio "billing.close_order"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CYCLE_ORDER" |

  @BR-BIL-003
  Regra: BR-BIL-003 — Fatura inclui apenas lotes do ciclo líquido de reembolso anterior
    Especificação: Emissão soma principal/tarifa não cancelados dos lotes agendados no ciclo; futuras parcelas não entram, embora já componham debt.

    @SC-BR-BIL-003-01
    Cenário: excluir futuro
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 3 |
        | lots | [{"cycle":3,"principal":3000,"fee":20,"cancelled_principal":0},{"cycle":4,"principal":3000,"fee":20,"cancelled_principal":0}] |
      Quando avalio "billing.issue"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 3000 |
        | fee | 20 |
        | total | 3020 |

    @SC-BR-BIL-003-02
    Cenário: reembolso anterior
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 3 |
        | lots | [{"cycle":3,"principal":3000,"fee":20,"cancelled_principal":1000}] |
      Quando avalio "billing.issue"
      Então observo os seguintes resultados
        | campo | valor |
        | principal | 2000 |
        | fee | 20 |
        | total | 2020 |

  @BR-BIL-004
  Regra: BR-BIL-004 — Ciclo vazio produz fatura zero válida
    Especificação: Não omitir fechamento sem consumo: criar emissão com total/minimum_due zero, avançando o ciclo. Isso não cria lançamento financeiro.

    @SC-BR-BIL-004-01
    Cenário: fechamento vazio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 3 |
        | lots | [] |
      Quando avalio "billing.empty"
      Então observo os seguintes resultados
        | campo | valor |
        | total | 0 |
        | minimum_due | 0 |
        | next_cycle | 4 |
        | events_added | 0 |

    @SC-BR-BIL-004-02
    Cenário: outro ciclo vazio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 4 |
        | lots | [] |
      Quando avalio "billing.empty"
      Então observo os seguintes resultados
        | campo | valor |
        | total | 0 |
        | minimum_due | 0 |
        | next_cycle | 5 |
        | events_added | 0 |

  @BR-BIL-005
  Regra: BR-BIL-005 — Vencimento deriva do ciclo, não da hora do fechamento
    Especificação: due_day=(cycle+1)*30+10. Fechamento tardio não prorroga vencimento.

    @SC-BR-BIL-005-01
    Cenário: no início da janela
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 3 |
        | close_day | 120 |
      Quando avalio "billing.due_day"
      Então observo os seguintes resultados
        | campo | valor |
        | due_day | 130 |

    @SC-BR-BIL-005-02
    Cenário: fechamento tardio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | cycle | 3 |
        | close_day | 135 |
      Quando avalio "billing.due_day"
      Então observo os seguintes resultados
        | campo | valor |
        | due_day | 130 |

  @BR-BIL-006
  Regra: BR-BIL-006 — Pagamento mínimo usa teto, piso e total
    Especificação: minimum_due=min(total,max(1000,ceil(total*1000/10000))); totalzero resulta emzero. Não exceder o saldo emitido nem usar arredondamento inferior.

    @SC-BR-BIL-006-01
    Cenário: saldo pequeno
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | total | 500 |
      Quando avalio "billing.minimum_due"
      Então observo os seguintes resultados
        | campo | valor |
        | minimum_due | 500 |

    @SC-BR-BIL-006-02
    Cenário: piso
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | total | 5000 |
      Quando avalio "billing.minimum_due"
      Então observo os seguintes resultados
        | campo | valor |
        | minimum_due | 1000 |

    @SC-BR-BIL-006-03
    Cenário: arredondamento superior
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | total | 10001 |
      Quando avalio "billing.minimum_due"
      Então observo os seguintes resultados
        | campo | valor |
        | minimum_due | 1001 |

    @SC-BR-BIL-006-04
    Cenário: zero
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | total | 0 |
      Quando avalio "billing.minimum_due"
      Então observo os seguintes resultados
        | campo | valor |
        | minimum_due | 0 |

  @BR-BIL-007
  Regra: BR-BIL-007 — Pagamento prioriza multa, tarifa e principal
    Especificação: Aplicar montante por classes: multa→tarifa→principal; dentro de classe, vencimento mais antigo e chaves estáveis. Não ratear proporcionalmente entre classes.

    @SC-BR-BIL-007-01
    Cenário: paga multa e parte de tarifa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | late | 1000 |
        | fee | 200 |
        | principal | 10000 |
        | payment | 1100 |
      Quando avalio "billing.allocate_payment"
      Então observo os seguintes resultados
        | campo | valor |
        | late_remaining | 0 |
        | fee_remaining | 100 |
        | principal_remaining | 10000 |

    @SC-BR-BIL-007-02
    Cenário: chega ao principal
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | late | 1000 |
        | fee | 200 |
        | principal | 10000 |
        | payment | 1500 |
      Quando avalio "billing.allocate_payment"
      Então observo os seguintes resultados
        | campo | valor |
        | late_remaining | 0 |
        | fee_remaining | 0 |
        | principal_remaining | 9700 |

  @BR-BIL-008
  Regra: BR-BIL-008 — Pagamento não antecipa parcelas ainda não faturadas
    Especificação: Só saldo já faturado é elegível. Parcela futura permanece na dívida e no crédito comprometido após quitação da fatura atual.

    @SC-BR-BIL-008-01
    Cenário: quitar atual preserva futuro
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | invoiced | 10000 |
        | future | 20000 |
        | payment | 10000 |
      Quando avalio "billing.pay_invoiced"
      Então observo os seguintes resultados
        | campo | valor |
        | invoiced_remaining | 0 |
        | future_remaining | 20000 |
        | debt | 20000 |

    @SC-BR-BIL-008-02
    Cenário: parcial atual
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | invoiced | 10000 |
        | future | 20000 |
        | payment | 5000 |
      Quando avalio "billing.pay_invoiced"
      Então observo os seguintes resultados
        | campo | valor |
        | invoiced_remaining | 5000 |
        | future_remaining | 20000 |
        | debt | 25000 |

  @BR-BIL-009
  Regra: BR-BIL-009 — Pagamento excedente é recusado integralmente
    Especificação: payment>invoiced_outstanding → DECLINED/OVERPAYMENT, sem consumo parcial ou saldo credor. Zero éINVALID_INPUT.

    @SC-BR-BIL-009-01
    Cenário: exato
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | invoiced | 10000 |
        | payment | 10000 |
      Quando avalio "billing.payment_admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | true |

    @SC-BR-BIL-009-02
    Cenário: excedente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | invoiced | 10000 |
        | payment | 10001 |
      Quando avalio "billing.payment_admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "OVERPAYMENT" |

    @SC-BR-BIL-009-03
    Cenário: zero
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | invoiced | 10000 |
        | payment | 0 |
      Quando avalio "billing.payment_admission"
      Então observo os seguintes resultados
        | campo | valor |
        | allowed | false |
        | reason | "INVALID_INPUT" |

  @BR-BIL-010
  Regra: BR-BIL-010 — Pagamento parcial reduz saldo mas não reescreve emissão
    Especificação: Snapshot de emissão e minimum_due original ficam estáveis; outstanding diminui. Saldo não quitado após due_day permanece inadimplente mesmo com mínimo pago.

    @SC-BR-BIL-010-01
    Cenário: mínimo pago ainda há atraso
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | issued_total | 10000 |
        | minimum_due | 1000 |
        | payment | 1000 |
        | due_day | 130 |
        | day | 131 |
      Quando avalio "billing.partial_payment"
      Então observo os seguintes resultados
        | campo | valor |
        | issued_total | 10000 |
        | minimum_due | 1000 |
        | outstanding | 9000 |
        | past_due | true |

    @SC-BR-BIL-010-02
    Cenário: quitado
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | issued_total | 10000 |
        | minimum_due | 1000 |
        | payment | 10000 |
        | due_day | 130 |
        | day | 131 |
      Quando avalio "billing.partial_payment"
      Então observo os seguintes resultados
        | campo | valor |
        | issued_total | 10000 |
        | outstanding | 0 |
        | past_due | false |

  @BR-BIL-011
  Regra: BR-BIL-011 — Multa por atraso é fixa e única por fatura
    Especificação: ASSESS com day>due_day,outstanding>0 e ainda não aplicada posta1000. Repetição com outra chave retorna NOOP; PREMIUM não isenta. Snapshot emitido permanece igual.

    @SC-BR-BIL-011-01
    Cenário: primeira multa
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 131 |
        | due_day | 130 |
        | outstanding | 5000 |
        | assessed | false |
        | tier | "PREMIUM" |
      Quando avalio "billing.assess_late_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "OK" |
        | late_delta | 1000 |
        | outstanding | 6000 |

    @SC-BR-BIL-011-02
    Cenário: já aplicada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 132 |
        | due_day | 130 |
        | outstanding | 6000 |
        | assessed | true |
      Quando avalio "billing.assess_late_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "NOOP" |
        | late_delta | 0 |
        | outstanding | 6000 |

  @BR-BIL-012
  Regra: BR-BIL-012 — Sem atraso ou saldo não há multa
    Especificação: No due_day a fatura não está atrasada. Saldozero impede multa mesmo depois do vencimento; não marcar a flag assessed num NOOP sem multa.

    @SC-BR-BIL-012-01
    Cenário: dia do vencimento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 130 |
        | due_day | 130 |
        | outstanding | 5000 |
        | assessed | false |
      Quando avalio "billing.assess_late_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "NOOP" |
        | late_delta | 0 |
        | assessed | false |

    @SC-BR-BIL-012-02
    Cenário: quitada após vencimento
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | day | 131 |
        | due_day | 130 |
        | outstanding | 0 |
        | assessed | false |
      Quando avalio "billing.assess_late_fee"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "NOOP" |
        | late_delta | 0 |
        | assessed | false |
