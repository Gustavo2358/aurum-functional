# Complexidade útil e casos difíceis

A entrega é ampla desde o início. A divisão em backlog não autoriza cortar capacidades. Dificuldade não deve depender de esconder bugs, UB, código morto artificial ou gerar milhares de ifs iguais.

## Mecanismos obrigatórios e evidência de ocorrência

| Mecanismo | Onde deve aparecer naturalmente |
|---|---|
| Dependência entre arquivos | preço → autorização → captura → fatura |
| Composição de guardas | elegibilidade, limite, risco, precedência |
| Definição sobrescrita | tarifa-base seguida de isenção/cap |
| Estrutura compartilhada | atualizações de conta via ponteiros tipados |
| Array/loop de agregação | histórico de velocidade, parcelas, recebíveis |
| Loop com `continue`/`break` | filtrar histórico e encerrar alocação de pagamento |
| Early return | rejeições e consultas de idempotência |
| `switch` | dispatcher, canal, categoria ou estados |
| Chamadas reutilizadas | auxiliares de dinheiro usados em tarifas e parcelas |
| Guarda distante do efeito | validação antes de commit em outro módulo |
| Alternativas convergentes | resultado comum de múltiplas rejeições |
| Estado temporal | validade, janela, expiração, fatura e cap de pontos |
| Sequências não comutativas | pagar→reembolsar versus reembolsar→pagar |
| Alocação cumulativa | tarifa/pontos no capture/refund parcial |
| Efeitos correlacionados | reserva+dívida+diário+pontos+idempotência |
| Ruído de suporte legítimo | parsing, serialização, logs e contagem operacional |

Sem ponteiro de função obrigatório, concorrência, metaprogramação ou aritmética de ponteiro opaca. `goto cleanup` pode ser usado se justificado por ownership real, não como artifício para elevar dificuldade. Macros não podem esconder fórmulas/branches; constantes e include guards são normais.

## Evidência mínima de interação

Na entrega, demonstrar pelo menos 24 regras cuja sustentação exige mais de uma função, pelo menos 12 cruzando arquivos, pelo menos 12 cenários com quatro ou mais comandos de domínio e pelo menos 8 regras com agregação/loop. Contagens devem vir do mapeamento real revisado; uma função wrapper não cria artificialmente uma dependência relevante. Pode haver sobreposição entre esses conjuntos.

## Não resolver a extração pela forma do código

Proibido: nomes de função/variável com ID da regra; comentário copiando Gherkin; tabela “entrada do caso → resposta”; uma árvore de decisão copiada mecanicamente do catálogo e sem integração; função isolada nunca chamada pela aplicação. Regras devem integrar um sistema operacional real dentro do domínio sintético.

Baseline legível, porém não didaticamente reduzido. Variantes metamórficas de renomeação e extração/inlining podem ser produzidas como corpus separado, preservando comportamento e mantendo o oracle externo. Não multiplicar LOC apenas para cumprir a faixa orientativa.
