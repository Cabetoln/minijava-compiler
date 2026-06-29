# Compilador MiniJava

Implementação de um compilador para MiniJava (subset de Java) com analisador
léxico (que também reconhece comentários, sem pré-processador), analisador
sintático descendente e tabela de símbolos.

# Estrutura do Projeto

- **main.cpp** — Arquivo principal que orquestra o compilador e trata as flags
- **io.cpp** — Leitura do arquivo-fonte
- **lexo.cpp** — Analisador léxico (tokens + comentários)
- **parser.cpp** — Analisador sintático descendente recursivo
- **symbol_table.cpp** — Tabela de símbolos com suporte a escopos global e local

# Compilação

Para compilar o projeto, basta rodar:

g++ main.cpp -o compilador

Para executar o projeto, basta rodar:

./compilador <arquivo.ling> [flags]

# Flags

- **--tokens**      Imprime a lista de tokens gerada pelo léxico.
- **--ast**         Imprime a árvore sintática abstrata (AST).
- **--tabela**      Imprime a tabela de símbolos após a análise sintática.
- **--sugestoes**   Exibe sugestões de correção léxica e sintática.
- **--parar**       Para no primeiro erro léxico (senão, processa toda a entrada).
- **--help**        Mostra a ajuda.

# Autores

Desenvolvido como trabalho prático para a disciplina de Compiladores (DIM0164). Alunos:

-> Carlos Alberto de Lima Neto
-> Claudivan Costa de Lima Júnior
-> Luiz Guilherme Carvalho Viana
-> Osvaldo Heitor de Andrade Tavares de Souza

