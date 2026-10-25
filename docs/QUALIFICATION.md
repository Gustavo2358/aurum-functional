# Qualificação da reconstrução

Resultado: aplicação integrada funcionalmente qualificada no escopo de SPEC.md. Nenhuma diferença aberta nas comparações executadas. Esta conclusão não é prova de equivalência universal nem qualificação Joern/PGC.

## Comando final executado

`make test-full` terminou com exit 0 e os 14 gates PASS; [relatório completo](../evidence/full/summary.json). Foi executado pelo fluxo de escalonamento aprovado porque LeakSanitizer não funciona sob a restrição ptrace do sandbox. Um programa vazio reproduziu essa limitação e passou fora dela. Nenhuma instalação foi necessária.

| Evidência | Resultado |
|---|---|
| Gabarito normativo | 296 cenários PASS: 136 UNIT, 127 COMPONENT, 33 E2E; mais 3 controles do harness |
| Propriedades numéricas | 16.887 casos; 32.502 asserções com referência inteira independente |
| Invariantes relacionais | 9 streams, 540 comandos, 212.281 asserções |
| Diferencial CLI | 136 streams fixos +24 gerados, stdout/stderr/exit/efeitos de arquivos exatos |
| Diferencial de estado | 7.557 comandos; todas as coleções, configuração, relógio e Result canônicos iguais |
| Falhas tardias adicionais | 7/7; rollback e campos de Result iguais ao original |
| Mutantes do novo programa | 16 mortos, zero sobreviventes; 1 controle equivalente aprovado |
| Controles antecedentes | 11 mutantes originais mortos; 1 equivalente; 5 alterações executáveis de observáveis detectadas |
| Pureza estrutural | 130 funções e 714 decisões AST inventariadas; 15 controles do gate aprovados |
| Pureza dinâmica | 13 coleções readonly, ciclo de 11 operações, replay/interleaving/conflito; inputs preservados |
| Sanitizers | ASan/UBSan PASS; CLI diferencial também com LeakSanitizer ativado |
| Produção isolada | Build com somente Makefile+subject PASS, sem backend/harness/oráculo |
| Preservação | 107 arquivos da origem conferidos por SHA-256; git original limpo |

Os gates financeiros não usam o resultado do candidato para construir expectativas. Gherkin oficial, referências matemáticas e invariantes independentes complementam o golden master. O golden foi compilado e executado **antes** de existir produção nova; especificação/harness foram registrados no commit `5acf7ab`. A implementação integral foi registrada em `7cb19a2`; o histórico posterior contém evidências e documentação de fechamento.

O original congelado é a revisão `a9f3b7e8ec7f09e2ccf9f514be635950226ad513`. SHA-256 do executável usado para congelar as comparações: `59ab090645f12b4424ba00ab405e13c6ff45379a11c0a8a4bd07cb99897698f0`. Fontes, gabarito, hashes e executáveis congelados acompanham a entrega; somente testes os usam.

## Cobertura e limites

Cobertura combinada do código de produção: 1.368/1.419 linhas instrumentadas (96,41%) e 1.419/1.808 arestas de branch (78,48%). Esses denominadores do gcov diferem dos 714 pontos de decisão do inventário AST. Não se afirma 100% de caminhos, nem se classifica automaticamente cada ramo não exercitado como impossível. [Detalhes por arquivo](../evidence/coverage/summary.json).

A auditoria AST e os testes readonly sustentam as restrições da implementação presente, sem constituir prova formal de pureza para C arbitrário. Const views têm ownership/lifetime documentados; o core retorna edições e continuações por valor, e a shell materializa a transição. Há ponteiros de leitura e mutabilidade local de construção.

Estados/continuações arbitrariamente corrompidos fora do contrato não são uma nova interface pública. Estados históricos e corrupções controladas do gabarito foram testados conforme seus probes. O adaptador ABI antigo é exclusivamente de teste; a API funcional de produção é nova.

Capacidades configuradas, rollback e erros de domínio são equivalência obrigatória e foram comparados. A ordem de falha ambiental do alocador não é comparada entre implementações com estratégias de memória distintas. Falhas reais de alocação continuam tratadas atomicamente pela shell. Sanitização do host Python desliga seu detector de vazamentos; os testes CLI instrumentados ativam o detector no processo C, separadamente. O runner de pureza não aloca no core e desliga LSan devido à limitação do sandbox, mantendo ASan/UBSan.

As varreduras ordenadas podem ter custo quadrático. As capacidades de 1..1.000.000 foram mantidas; não se declara teste exaustivo de todas as combinações nem benchmark na escala máxima.

Ferramentas: GCC/cc Ubuntu 15.2.0, C17, Python 3.14.4, gherkin-official 42.0.1 preservado com licença, libclang 18.1.1 já instalado. Binário C normal não depende dessas ferramentas Python.

## Próxima decisão

Joern, slicing, extração de regras, packs experimentais e geração extraída de Gherkin: **NOT_RUN por instrução explícita do usuário**. O pacote de entrega é somente um arquivo do projeto e das evidências. A próxima rodada dessas metodologias permanece para decisão do usuário.

O remoto novo está configurado como `https://github.com/Gustavo2358/aurum-functional.git`, verificado vazio antes da configuração. Não houve push, publicação ou alteração do remoto do original. Os commits locais estão prontos para revisão/publicação posterior.
