# Produto e recorte científico

## Objetivo

Construir **Aurum**, simulador determinístico de operações de cartão de crédito: cotação, elegibilidade, limites, risco, autorização, captura, cancelamento, expiração, reembolso, parcelamento, fatura, pagamento, recompensas e processamento em lote. O simulador é a aplicação-alvo do extrator, não o próprio extrator.

No PGC, “regra de negócio” é a terminologia da literatura de Business Rule Extraction, adotada operacionalmente como função, restrição ou transformação de entradas em saídas, `O = F(I)`, conforme Huang et al. Para estado, usamos a extensão de trabalho `(O, S') = F(I, S, C, T)`: entradas, estado anterior, configuração explícita e tempo explícito. Essa extensão é uma convenção deste projeto, não uma fórmula atribuída ao paper. SQLite continua sendo um sistema real pertinente a essa definição de regra funcional. Ver `REFERENCES.md`.

## Entrega inaugural completa

Toda a regra de negócio do catálogo, requisitos técnicos documentados, interface C, CLI batch, cenários Gherkin executados contra o C, testes adversariais/metamórficos, registros contábeis verificáveis, rastreabilidade, corpus limpo e smoke real de Joern. Não há “implementar seis regras agora e o resto depois”.

A complexidade deve vir de interações semânticas: estado, prioridades, fórmulas, múltiplas funções e arquivos, agregações, parcelas e efeitos correlacionados. A faixa orientativa de alguns milhares de linhas de produção não é meta para encher código. Não há recompensa por boilerplate ou por ramificações mortas.

## Fora do escopo

Não criar banco real, autenticação de usuário, rede, HTTP, interface web, integração de pagamento, multithreading, criptografia, acesso a dados reais, framework de regras, interpretador de DSL financeira, banco de dados persistente ou analisador estático próprio. O programa não precisa de SQLite como dependência. Não chamar LLM em runtime. O extrator Joern → evidência → regras é trabalho separado.

## O que “todo código tem regra” significa

Comportamentos de domínio têm IDs BR; comportamentos técnicos observáveis têm IDs FR; auxiliares internos e infraestrutura têm vínculo aos contratos que implementam. Não é correto criar cem falsas regras para getters/loops/alocação. É obrigatório inventariar funções e pontos de decisão, justificar suporte técnico e apontar regra(s) servida(s). A meta é **zero comportamento órfão**, não uma relação artificial um-para-um entre linha e regra.

O denominador de regras funcionais extraíveis e o denominador de requisitos de infraestrutura são separados no relatório. Ambas as classes ficam documentadas. Dentro do recorte científico, uma regra técnica funcional pode ser objeto de extração, desde que o protocolo de avaliação a selecione explicitamente.

## Limites do experimento

O autor da fixture conhece o oráculo, mas o processo que executa a extração não o recebe. Isso previne vazamento de arquivos, não prova ausência de viés de construção ou memorização pelo mesmo modelo. O sistema sintético não é amostra aleatória de sistemas reais. Casos inéditos, mutações e o estudo posterior em SQLite são avaliações complementares, não provas automáticas de generalização.
