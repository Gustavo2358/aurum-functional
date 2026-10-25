# Interface pública e execução Gherkin

## Produto C

A aplicação oferece API C e um executável `aurum`. CLI obrigatória: `aurum --seed seed.txt --commands commands.txt`, aceitando `-` para comandos em stdin. Seed só cria configuração/entidades iniciais; não injeta resultados esperados nem regras. O agente pode adicionar comandos de inspeção/help documentados, não políticas adicionais.

Protocolo: uma operação por linha ASCII, `OP key=value key=value`. Whitespace separador é espaço ou tab; CRLF e LF aceitos. IDs: 1..48 caracteres de `[A-Za-z0-9_-]`; case-sensitive. Sem quoting ou escapes. Inteiros decimais canônicos sem `+`, sem notação científica; zeros à esquerda são aceitos e normalizados. Booleanos `true`/`false`. Chaves duplicadas, desconhecidas, campo obrigatório ausente, enum inválido, sinal indevido ou token malformado → ERROR/INVALID_INPUT. Máximo 8192 bytes por linha incluindo terminador; exceder exige descartar a linha inteira e emitir uma falha, não processar seu prefixo. Linhas vazias e linhas cujo primeiro caractere não branco é `#` são ignoradas. Comentário inline não existe.

Seed usa `CLOCK now=144600`, `CONFIG key=value...`, `ACCOUNT id=A1 ...`, `CARD id=C1 account_id=A1 ...`, `MERCHANT id=M1 ...`, nessa ordem; entidades referenciadas precisam existir. Config omitida usa os defaults do domínio. Entidade duplicada e seed inválido encerram antes de processar comandos com exit2 e diagnóstico stderr. Domínio não cria automaticamente entidades inexistentes. Especificar campos obrigatórios/defaults em `subject/CLI.md` não exportado no corpus; defaults não podem contrariar o perfil base. Não permitir alteração de política após o início do stream.

Comandos implementam OPERATIONS.md. Saída é um objeto JSON por comando efetivamente processado, sem banner, locale, endereço ou timestamp externo. Campos estáveis e inteiros exatos; IDs têm escaping JSON correto mesmo que a gramática atual os restrinja. stdout contém apenas resultados; diagnóstico stderr. Exit0 com stream lido por completo, inclusive decisões DECLINED/ERROR locais; exit2 para erro de invocação/seed/I/O impeditivo. Nunca retornar sucesso de processo como substituto de validar `decision` em cada resultado.

## Vocabulário Gherkin

Os `.feature` deste pacote usam português (`# language: pt`) e três passos parametrizados com DataTables:

1. `Dado o perfil "base" e os seguintes dados` — criar um engine ou contexto puro **válido**, com overrides tipados. Valores são literais JSON; strings têm aspas; tabelas sem override usam `profile=base`.
2. `Quando avalio "<operação>"` — invocar código C de produção; aliases `money.*`, `pricing.*`, `eligibility.*`, `limits.*`, `risk.*`, `auth.*`, `idempotency.*`, `capture.*`, `reversal.*`, `installment.*`, `billing.*`, `rewards.*`, `batch.*`, `io.*` designam observações testáveis, não nomes impostos de funções.
3. `Então observo os seguintes resultados` — comparar campos indicados por igualdade exata tipada. Campos omitidos não são implicitamente zero. Para cenários de rejeição/atomicidade, incluir ou acrescentar uma comparação integral do estado financeiro anterior/depois.

Cada cenário é independente. O catálogo define o significado da operação de observação; o agente registra em `harness/PROBES.md` qual API C ela chama e quais campos prepara. Um probe pode chamar mais de uma API para testar sequência, mas **não pode calcular a resposta esperada nem tomar uma decisão financeira no driver**. Não despachar por ID de regra/cenário; só por operação e entradas.

Não implementar steps como `assert True`, não só contar cenários, não avaliar por regex de texto gerado. Usar `gherkin-official` para parse/compilação de Examples; bindings podem ser escritos em Python e chamar um driver C JSON via subprocess. O agente pode escolher outro runner que respeite esses contratos. No driver só serialização, setup validado e chamadas reais; fórmulas no driver são proibidas salvo construção de dados explícitos de entrada.

Os cenários entregues são vetores normativos iniciais; completar fronteiras, sequências reais e contraprovas durante implementação, sem substituir resultados fixos pelo SUT. As mensagens `op` têm campos que o catálogo define; aliases de observação podem ser agrupados para reduzir boilerplate. Uma função C pura compartilhada deve ser usada tanto pelo comando real quanto pelo probe, não duplicada para testes.

## Observação de estado e respostas

`financial_unchanged`, `same_response`, `reconciled`, `balanced`, `unchanged` são **asserções calculadas pelo harness por comparação**, não flags que o SUT pode devolver e autodeclarar. `events_added`, `holds`, `debt`, `points`, `cash_refund` vêm de snapshots/diário reais. Cenários de protocolo bruto (`raw`) executam o binário de produção, não um parser alternativo.
