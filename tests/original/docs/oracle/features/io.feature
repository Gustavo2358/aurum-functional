# language: pt
@cap_io
Funcionalidade: Protocolo e requisitos técnicos
  Oráculo autoral da fixture Aurum. Não distribuir ao extrator.

  @FR-IO-001
  Regra: FR-IO-001 — Campos obrigatórios e enums são validados
    Especificação: Envelope de AUTHORIZE exige referências eamount; currency/parcelas/canal têm defaults do perfil de pedido. Enum desconhecido ou campo obrigatório ausente éINVALID_INPUT antes de idempotência.

    @SC-FR-IO-001-01
    Cenário: falta amount
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1" |
      Quando avalio "io.parse"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "INVALID_INPUT" |

    @SC-FR-IO-001-02
    Cenário: canal desconhecido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=100 channel=DRONE" |
      Quando avalio "io.parse"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "INVALID_INPUT" |

  @FR-IO-002
  Regra: FR-IO-002 — Chave duplicada é erro, não last-write-wins
    Especificação: Tokens que repetem a mesma chave na linha invalidam o comando inteiro mesmo quando valores são iguais.

    @SC-FR-IO-002-01
    Cenário: valores diferentes
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "QUOTE amount=100 amount=200" |
      Quando avalio "io.parse"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "INVALID_INPUT" |

    @SC-FR-IO-002-02
    Cenário: valores iguais
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "QUOTE amount=100 amount=100" |
      Quando avalio "io.parse"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "INVALID_INPUT" |

  @FR-IO-003
  Regra: FR-IO-003 — Campo desconhecido não é ignorado
    Especificação: Chaves não admitidas pela operação causam INVALID_INPUT, para evitar mutação diferente da intenção enviada.

    @SC-FR-IO-003-01
    Cenário: typo de amount
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "QUOTE amunt=100" |
      Quando avalio "io.parse"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "INVALID_INPUT" |

    @SC-FR-IO-003-02
    Cenário: campo extra
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "QUOTE amount=100 magic=true" |
      Quando avalio "io.parse"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "INVALID_INPUT" |

  @FR-IO-004
  Regra: FR-IO-004 — Identificadores têm formato e tamanho definidos
    Especificação: ID tem1..48 caracteres ASCII[A-Za-z0-9_-]; preservar case. Não aceitar vazio,barra,espaço ou truncamento.

    @SC-FR-IO-004-01
    Cenário: válido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | id | "A-1_abc" |
      Quando avalio "io.identifier"
      Então observo os seguintes resultados
        | campo | valor |
        | valid | true |

    @SC-FR-IO-004-02
    Cenário: limite de 48
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | id | "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" |
      Quando avalio "io.identifier"
      Então observo os seguintes resultados
        | campo | valor |
        | valid | true |

    @SC-FR-IO-004-03
    Cenário: 49 caracteres
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | id | "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" |
      Quando avalio "io.identifier"
      Então observo os seguintes resultados
        | campo | valor |
        | valid | false |

    @SC-FR-IO-004-04
    Cenário: barra
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | id | "A/1" |
      Quando avalio "io.identifier"
      Então observo os seguintes resultados
        | campo | valor |
        | valid | false |

  @FR-IO-005
  Regra: FR-IO-005 — Inteiros são parseados sem float e sem overflow
    Especificação: Entrada decimal inteira, zeros à esquerda aceitos; fração,expoente,sinal+ e fora do domínio são recusados. Validação antes de multiplicação/conversão.

    @SC-FR-IO-005-01
    Cenário: zeros à esquerda
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | text | "000123" |
      Quando avalio "io.money_parse"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | true |
        | value | 123 |

    @SC-FR-IO-005-02
    Cenário: decimal fracionário
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | text | "1.5" |
      Quando avalio "io.money_parse"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |

    @SC-FR-IO-005-03
    Cenário: expoente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | text | "1e3" |
      Quando avalio "io.money_parse"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |

    @SC-FR-IO-005-04
    Cenário: positivo com sinal
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | text | "+1" |
      Quando avalio "io.money_parse"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |

    @SC-FR-IO-005-05
    Cenário: overflow
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | text | "999999999999999999999999999" |
      Quando avalio "io.money_parse"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |

  @FR-IO-006
  Regra: FR-IO-006 — Linha longa é rejeitada integralmente
    Especificação: Até8192 bytes incluindo LF são aceitos pelo gate de comprimento;8193 são rejeitados e o restante descartado até LF. A linha seguinte ainda deve ser processada. Nunca executar prefixo truncado.

    @SC-FR-IO-006-01
    Cenário: limite
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | line_bytes | 8192 |
      Quando avalio "io.line_limit"
      Então observo os seguintes resultados
        | campo | valor |
        | length_allowed | true |

    @SC-FR-IO-006-02
    Cenário: excesso e próxima linha
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | line_bytes | 8193 |
        | next_line | "QUOTE amount=100" |
      Quando avalio "io.line_limit"
      Então observo os seguintes resultados
        | campo | valor |
        | length_allowed | false |
        | next_processed | true |

  @FR-IO-007
  Regra: FR-IO-007 — Linhas vazias e comentários completos não geram resultado
    Especificação: Ignorar linha vazia e linha cujo primeiro caractere não branco é#. Comentário inline é token extra e portanto erro.

    @SC-FR-IO-007-01
    Cenário: linhas ignoráveis
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | lines | ["","   ","# comment","  # comment"] |
      Quando avalio "io.ignored_lines"
      Então observo os seguintes resultados
        | campo | valor |
        | response_count | 0 |

    @SC-FR-IO-007-02
    Cenário: comentário inline inválido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | lines | ["QUOTE amount=100 # comment"] |
      Quando avalio "io.ignored_lines"
      Então observo os seguintes resultados
        | campo | valor |
        | response_count | 1 |
        | decision | "ERROR" |

  @FR-IO-008
  Regra: FR-IO-008 — JSON de saída é determinístico e tipado
    Especificação: Um comando processado produz um objeto JSON; dinheiro é inteiro, não string/float. Mesmo comando/contexto gera os mesmos bytes canônicos, sem banners ou relógio externo.

    @SC-FR-IO-008-01
    Cenário: cotação repetida isoladamente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "QUOTE amount=100 currency=BRL" |
        | runs | 2 |
      Quando avalio "io.output"
      Então observo os seguintes resultados
        | campo | valor |
        | response_count_per_run | 1 |
        | principal_is_integer | true |
        | identical_bytes | true |

    @SC-FR-IO-008-02
    Cenário: valor com centavos
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | raw | "QUOTE amount=101 currency=BRL" |
        | runs | 2 |
      Quando avalio "io.output"
      Então observo os seguintes resultados
        | campo | valor |
        | principal_is_integer | true |
        | identical_bytes | true |

  @FR-IO-009
  Regra: FR-IO-009 — Exit code distingue stream concluído de erro impeditivo
    Especificação: Stream lido por completo retornaexit0 mesmo com rejeições locais. Seed/invocação/I/O impeditivo retornaexit2; stderr recebe diagnóstico e stdout não recebe banner.

    @SC-FR-IO-009-01
    Cenário: rejeição local
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | mode | "VALID_STREAM_WITH_DECLINE" |
      Quando avalio "io.exit"
      Então observo os seguintes resultados
        | campo | valor |
        | exit_code | 0 |

    @SC-FR-IO-009-02
    Cenário: seed inválido
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | mode | "INVALID_SEED" |
      Quando avalio "io.exit"
      Então observo os seguintes resultados
        | campo | valor |
        | exit_code | 2 |

  @FR-IO-010
  Regra: FR-IO-010 — Seed inválido não cria engine parcial
    Especificação: Entidades duplicadas,referências ausentes,configuração inválida ou ordem inadequada abortam a carga antes dos comandos. Não pular silenciosamente linhas inválidas do seed.

    @SC-FR-IO-010-01
    Cenário: conta duplicada
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | defect | "DUPLICATE_ACCOUNT" |
      Quando avalio "io.seed"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | commands_processed | 0 |

    @SC-FR-IO-010-02
    Cenário: dono ausente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | defect | "MISSING_CARD_OWNER" |
      Quando avalio "io.seed"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | commands_processed | 0 |

    @SC-FR-IO-010-03
    Cenário: taxa inválida
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | defect | "INVALID_RATE" |
      Quando avalio "io.seed"
      Então observo os seguintes resultados
        | campo | valor |
        | ok | false |
        | commands_processed | 0 |

  @FR-IO-011
  Regra: FR-IO-011 — Capacidade esgotada é falha explícita
    Especificação: Limites de armazenamento são configuráveis e documentados. Excesso retorna CAPACITY sem truncar histórico,diário,lotes,keys nem perder conta. Testar falha por alocação controlada.

    @SC-FR-IO-011-01
    Cenário: histórico cheio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | resource | "HISTORY" |
        | remaining_capacity | 0 |
      Quando avalio "io.capacity"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CAPACITY" |
        | unchanged | true |

    @SC-FR-IO-011-02
    Cenário: diário cheio
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | resource | "LEDGER" |
        | remaining_capacity | 0 |
      Quando avalio "io.capacity"
      Então observo os seguintes resultados
        | campo | valor |
        | decision | "ERROR" |
        | reason | "CAPACITY" |
        | unchanged | true |

  @FR-IO-012
  Regra: FR-IO-012 — Locale e relógio de parede não mudam semântica
    Especificação: Mesmo seed e stream com TICK explícito produzem o mesmo resultado sob diferentes LC_ALL/TZ suportados. Não usar time/rand para decidir políticas,IDs ou arredondamento.

    @SC-FR-IO-012-01
    Cenário: timezone diferente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | variation | "TZ" |
      Quando avalio "io.environment"
      Então observo os seguintes resultados
        | campo | valor |
        | identical_results | true |

    @SC-FR-IO-012-02
    Cenário: locale diferente
      Dado o perfil "base" e os seguintes dados
        | campo | valor |
        | variation | "LC_ALL" |
      Quando avalio "io.environment"
      Então observo os seguintes resultados
        | campo | valor |
        | identical_results | true |
