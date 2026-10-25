# Aurum funcional em C17

Qualificação final: **14 gates PASS**. Veja [resultados e limites](docs/QUALIFICATION.md). Entrega local: `/home/gustavo/Documents/PGC/aurum-functional`, com manifesto SHA-256 e arquivo ZIP.

Reconstrução integral do simulador sintético Aurum com functional core e imperative shell. O executável usa somente C17 e a biblioteca padrão; não chama o Aurum original, Python, serviços externos ou arquivos de teste para decidir resultados.

```sh
make
./build/aurum --seed examples/seed.txt --commands examples/commands.txt
./build/aurum --seed examples/seed.txt --commands -
make test-fast
make test-full
```

O protocolo e as 13 operações são os do Aurum original: cotação, autorização, captura, cancelamento, reembolso, fechamento, pagamento, multa, avanço de tempo, consultas e reconciliação. Todo domínio financeiro é inventado. [SPEC.md](docs/SPEC.md) define o contrato completo, precedências, arredondamento, histórico, erros e observáveis. [TASKS.md](docs/TASKS.md) registra dependências e conclusão; [TRACEABILITY.md](docs/TRACEABILITY.md) liga regras a implementação e testes.

O core recebe `StateView`, comando e continuação explícitos. Cada chamada retorna `Step` por valor: uma alteração tipada e nova continuação, ou resultado terminal. A shell materializa as alterações em uma cópia privada e publica somente `CORE_COMMIT`. Configuração, tempo, cálculos, ordem financeira, idempotência, limites e decisões de commit pertencem ao core. A shell cuida dos arquivos, parsing, JSON, alocação e ownership.

Há ponteiros: views e buscas apontam somente para leitura, com vida útil controlada pela shell. O core não modifica argumentos, não aloca memória, não acessa relógio ou aleatoriedade, nem usa globais mutáveis. Variáveis locais são usadas para construir os valores devolvidos. C não prova pureza pelo tipo; restrições, contratos de alias e verificações estão em [ARCHITECTURE-DESIGN.md](docs/ARCHITECTURE-DESIGN.md) e [architecture-contract.md](tests/architecture-contract.md).

Build requer compilador C17 e Make. Testes requerem Python 3; o parser oficial Gherkin foi preservado com sua licença em `tests/vendor`. Auditoria AST requer um ambiente existente com libclang; selecione-o com `AURUM_AST_PYTHON=/caminho/python`. Nesta execução foi usado o ambiente já instalado do Aurum original, somente para a ferramenta. Nenhuma instalação automática é realizada. Sanitizers e gcov usam o compilador disponível.

[HARNESS.md](tests/HARNESS.md) documenta o golden master congelado, cenários independentes, diferenciais e controles negativos. `build/libaurum.so` inclui um adaptador exclusivamente de testes para executar o gabarito anterior contra o novo core. O binário `build/aurum` não inclui esse adaptador. `make build/aurum` funciona somente com `subject/` e o Makefile, sem o oráculo.

A comparação é limitada às entradas/estados e interfaces documentados; testes finitos não demonstram equivalência universal. Ordem de falha real de alocação depende do ambiente e não é contrato de equivalência de alocadores. Capacidade lógica configurada, rollback e histórico continuam obrigatórios. Seleções ordenadas sem memória auxiliar podem ter custo quadrático; limites de coleções foram preservados, sem promessa de desempenho em escala máxima.

Não foram executados Joern, slicing nem metodologias de extração. Esta entrega não herda a qualificação experimental PGC do original. O próximo experimento depende de decisão do usuário.

Remoto configurado: `https://github.com/Gustavo2358/aurum-functional.git`. Nenhum push/publicação faz parte desta entrega. Fontes originais preservadas como referência de teste em `tests/original`; proveniência e hashes em `tests/original-manifest.json`. Não se concede aqui licença nova para o original ou para a reconstrução; dependências de terceiros conservam seus avisos.
