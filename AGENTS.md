# Aurum funcional — instruções do projeto

Leia README.md, docs/SPEC.md, docs/ARCHITECTURE-DESIGN.md, docs/TASKS.md e docs/TRACEABILITY.md antes de alterar o produto. A especificação e os testes independentes antecedem mudanças de comportamento. Preserve precedência, bytes de protocolo, estado, histórico, idempotência e capacidade lógica; nunca altere o golden para fazer o candidato passar.

`subject/src/core_*.c` recebe valores/views de leitura e retorna valores, edições e continuações. Não introduza I/O, heap, globais mutáveis, relógio/aleatoriedade ocultos ou mutação através de argumentos. Regras, ordenação financeira e decisão de publicação permanecem no core. A shell possui memória, parsing, serialização e materialização genérica de transações. Leia o protocolo de lifetime/continuação; C não prova pureza sozinho.

`tests/original/` e goldens são referências congeladas somente de teste. Produção não depende desses arquivos. O adaptador de ABI legado fica em tests/ e nunca no executável CLI. Atualize rastreabilidade para símbolos/spans reais e rode gates proporcionais; `make test-full` é a qualificação funcional final. Ferramentas ausentes são NOT_RUN, nunca PASS. Não instalar automaticamente novas dependências.

Não executar Joern, slicing, packs experimentais nem metodologias de extração sem nova solicitação. Não modificar o Aurum original, vault ou históricos. O origin deste projeto é o novo aurum-functional; não fazer push sem autorização explícita. A entrega inicial foi coordenada em tarefas Astra High a pedido do usuário; não inferir autorização para chamadas externas LLM.
