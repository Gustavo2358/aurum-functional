# Protocolo e requisitos técnicos

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## FR-IO-001 — Campos obrigatórios e enums são validados

Envelope de AUTHORIZE exige referências eamount; currency/parcelas/canal têm defaults do perfil de pedido. Enum desconhecido ou campo obrigatório ausente éINVALID_INPUT antes de idempotência.

**Observação executada:** `io.parse`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-001` e `SC-FR-IO-001-NN`.

## FR-IO-002 — Chave duplicada é erro, não last-write-wins

Tokens que repetem a mesma chave na linha invalidam o comando inteiro mesmo quando valores são iguais.

**Observação executada:** `io.parse`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-002` e `SC-FR-IO-002-NN`.

## FR-IO-003 — Campo desconhecido não é ignorado

Chaves não admitidas pela operação causam INVALID_INPUT, para evitar mutação diferente da intenção enviada.

**Observação executada:** `io.parse`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-003` e `SC-FR-IO-003-NN`.

## FR-IO-004 — Identificadores têm formato e tamanho definidos

ID tem1..48 caracteres ASCII[A-Za-z0-9_-]; preservar case. Não aceitar vazio,barra,espaço ou truncamento.

**Observação executada:** `io.identifier`.

**Exemplos:** 4 cenários em [Gherkin](../features/io.feature), tags `FR-IO-004` e `SC-FR-IO-004-NN`.

## FR-IO-005 — Inteiros são parseados sem float e sem overflow

Entrada decimal inteira, zeros à esquerda aceitos; fração,expoente,sinal+ e fora do domínio são recusados. Validação antes de multiplicação/conversão.

**Observação executada:** `io.money_parse`.

**Exemplos:** 5 cenários em [Gherkin](../features/io.feature), tags `FR-IO-005` e `SC-FR-IO-005-NN`.

## FR-IO-006 — Linha longa é rejeitada integralmente

Até8192 bytes incluindo LF são aceitos pelo gate de comprimento;8193 são rejeitados e o restante descartado até LF. A linha seguinte ainda deve ser processada. Nunca executar prefixo truncado.

**Observação executada:** `io.line_limit`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-006` e `SC-FR-IO-006-NN`.

## FR-IO-007 — Linhas vazias e comentários completos não geram resultado

Ignorar linha vazia e linha cujo primeiro caractere não branco é#. Comentário inline é token extra e portanto erro.

**Observação executada:** `io.ignored_lines`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-007` e `SC-FR-IO-007-NN`.

## FR-IO-008 — JSON de saída é determinístico e tipado

Um comando processado produz um objeto JSON; dinheiro é inteiro, não string/float. Mesmo comando/contexto gera os mesmos bytes canônicos, sem banners ou relógio externo.

**Observação executada:** `io.output`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-008` e `SC-FR-IO-008-NN`.

## FR-IO-009 — Exit code distingue stream concluído de erro impeditivo

Stream lido por completo retornaexit0 mesmo com rejeições locais. Seed/invocação/I/O impeditivo retornaexit2; stderr recebe diagnóstico e stdout não recebe banner.

**Observação executada:** `io.exit`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-009` e `SC-FR-IO-009-NN`.

## FR-IO-010 — Seed inválido não cria engine parcial

Entidades duplicadas,referências ausentes,configuração inválida ou ordem inadequada abortam a carga antes dos comandos. Não pular silenciosamente linhas inválidas do seed.

**Observação executada:** `io.seed`.

**Exemplos:** 3 cenários em [Gherkin](../features/io.feature), tags `FR-IO-010` e `SC-FR-IO-010-NN`.

## FR-IO-011 — Capacidade esgotada é falha explícita

Limites de armazenamento são configuráveis e documentados. Excesso retorna CAPACITY sem truncar histórico,diário,lotes,keys nem perder conta. Testar falha por alocação controlada.

**Observação executada:** `io.capacity`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-011` e `SC-FR-IO-011-NN`.

## FR-IO-012 — Locale e relógio de parede não mudam semântica

Mesmo seed e stream com TICK explícito produzem o mesmo resultado sob diferentes LC_ALL/TZ suportados. Não usar time/rand para decidir políticas,IDs ou arredondamento.

**Observação executada:** `io.environment`.

**Exemplos:** 2 cenários em [Gherkin](../features/io.feature), tags `FR-IO-012` e `SC-FR-IO-012-NN`.
