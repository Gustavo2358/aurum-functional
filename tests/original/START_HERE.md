# Comece aqui

Aurum é uma fixture financeira sintética em C17, com aplicação, CLI, harness Gherkin oficial, testes independentes, rastreabilidade e exportação implementados. O estado atual é **PGC_FIXTURE_QUALIFIED**, no escopo de [ACCEPTANCE](docs/ACCEPTANCE.md). O backlog B00–B15 está concluído.

## Executar e conferir o estado

1. Leia [README.md](README.md) para instalar as dependências, compilar e executar a aplicação e os testes.
2. Consulte [PROGRESS.md](PROGRESS.md) para os gates e limites atuais e [PACKAGE_CHECK.md](PACKAGE_CHECK.md) para as evidências de qualificação. [PACKAGE_MANIFEST.json](PACKAGE_MANIFEST.json) resume o estado e o inventário do pacote.
3. Para alterações, siga [AGENTS.md](AGENTS.md), [PRODUCT](docs/PRODUCT.md), [DOMAIN](docs/DOMAIN.md), [ARCHITECTURE](docs/ARCHITECTURE.md) e o [backlog concluído](backlog/BACKLOG.md).

`make test-fast` executa a regressão obrigatória rápida; `make test-full` inclui mutações, sanitizers, cobertura, exportação isolada e Joern. `tools/check_spec.py` é uma verificação editorial; seu sucesso isolado não significa execução financeira nem qualidade de slicing.

## Manutenção

Mantenha contratos, catálogo e Gherkin concordantes. Leia os contratos, catálogos e features afetados, preserve a independência do oráculo e regenere os mapas depois de alterar código. A arquitetura e o backlog descrevem o escopo integral entregue, que deve ser preservado nas alterações.

O repositório Git e o remote `origin` já estão configurados. Faça commits coerentes e publique conforme solicitado pelo usuário.

## Limite da qualificação

Os cenários executam a aplicação C real. O gate Joern verifica ingestão, locais de código e dependências selecionadas; os slices nativos têm limites documentados. A extração de regras pelo método do PGC e sua avaliação semântica continuam sendo trabalho separado. O extrator deve receber somente o corpus limpo e os critérios públicos, sem acesso ao oráculo.
