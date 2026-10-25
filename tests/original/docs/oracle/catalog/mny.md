# Dinheiro e aritmética

Contrato geral: [DOMAIN](../../DOMAIN.md), [operações](../../contracts/OPERATIONS.md), [estado](../../contracts/STATE_AND_LEDGER.md).
Aliases de observação e setup: [IO_AND_HARNESS](../../contracts/IO_AND_HARNESS.md) e [PROBES](../PROBES.md).

## BR-MNY-001 — Domínio monetário inteiro e limitado

Valores monetários não assinados válidos pertencem a [0, 1000000000000]. Negativo ou acima do máximo é NUMERIC_RANGE; zero é permitido na representação, embora comandos de pagamento/captura exijam positivo.

**Observação executada:** `money.validate`.

**Exemplos:** 4 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-001` e `SC-BR-MNY-001-NN`.

## BR-MNY-002 — Soma monetária verificada

A soma de dois valores monetários válidos só existe no modelo se o resultado não exceder MONEY_MAX. Validar antes de atualizar acumuladores.

**Observação executada:** `money.add`.

**Exemplos:** 2 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-002` e `SC-BR-MNY-002-NN`.

## BR-MNY-003 — Subtração sem saldo negativo

Subtrair b de a requer a>=b e operandos monetários válidos. Underflow deve ser rejeitado, não convertido em unsigned enorme ou zero silencioso.

**Observação executada:** `money.subtract`.

**Exemplos:** 3 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-003` e `SC-BR-MNY-003-NN`.

## BR-MNY-004 — Percentual com truncamento inferior

floor_rate(v,bps)=floor(v*bps/10000), com v não negativo e bps em [0,10000]. Não arredondar meio centavo para cima.

**Observação executada:** `money.floor_rate`.

**Exemplos:** 3 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-004` e `SC-BR-MNY-004-NN`.

## BR-MNY-005 — Percentual com arredondamento superior

ceil_rate(v,bps)=ceil(v*bps/10000). Zero permanece zero. Este operador é usado no pagamento mínimo, não nas tarifas.

**Observação executada:** `money.ceil_rate`.

**Exemplos:** 3 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-005` e `SC-BR-MNY-005-NN`.

## BR-MNY-006 — Conversão com metade para cima

fx(v,rate)=floor((v*rate+5000)/10000). A metade exata sobe para o próximo centavo. Operandos e resultado respeitam os limites do domínio.

**Observação executada:** `money.fx`.

**Exemplos:** 3 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-006` e `SC-BR-MNY-006-NN`.

## BR-MNY-007 — Configuração de taxas validada

bps deve estar entre 0 e 10000 inclusive; FX deve estar entre 1 e 100000 inclusive. Taxa inválida não pode ser usada nem corrigida por clamp.

**Observação executada:** `money.validate_rates`.

**Exemplos:** 4 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-007` e `SC-BR-MNY-007-NN`.

## BR-MNY-008 — Intermediários aritméticos não transbordam

A multiplicação verificada de inteiros int64 não negativos retorna NUMERIC_RANGE se exceder INT64_MAX. Nunca executar signed overflow para então tentar detectá-lo. O limite monetário final continua sendo outro cheque.

**Observação executada:** `money.checked_mul`.

**Exemplos:** 2 cenários em [Gherkin](../features/mny.feature), tags `BR-MNY-008` e `SC-BR-MNY-008-NN`.
