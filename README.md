# Compilador MiniJava

Implementação de um compilador para MiniJava (subset de Java) com analisador
léxico (que também reconhece comentários, sem pré-processador), analisador
sintático descendente e tabela de símbolos.

# Estrutura do Projeto

- **main.cpp** — Arquivo principal que orquestra o compilador e trata as flags
- **io.cpp** — Leitura do arquivo-fonte
- **lexo.cpp** — Analisador léxico (tokens + comentários)
- **ast.cpp** — Definição dos nós da árvore sintática abstrata e seu impressor
- **parser.cpp** — Analisador sintático descendente recursivo (constrói a AST)
- **semantic.cpp** — Análise semântica (tipos, herança, resolução de nomes)
- **symbol_table.cpp** — Tabela de símbolos com suporte a escopos global e local

# Compilação

Para compilar o projeto, basta rodar:

g++ -std=c++14 main.cpp -o compilador

Para executar o projeto, basta rodar:

./compilador <arquivo.ling> [flags]

Exemplos:

./compilador testes/Correto1.ling
./compilador testes/Correto1.ling --ast --tabela
./compilador testes/Erro1_Sintatico.ling --tokens
./compilador testes/Erro2_Semantico.ling --sugestoes --parar

# Flags

- **--tokens**      Imprime a lista de tokens gerada pelo léxico.
- **--ast**         Imprime a árvore sintática abstrata (AST).
- **--tabela**      Imprime a tabela de símbolos após a análise sintática.
- **--sugestoes**   Exibe sugestões de correção léxica e sintática.
- **--parar**       Para no primeiro erro léxico (senão, processa toda a entrada).
- **--help**        Mostra a ajuda.

# Testes

A pasta `testes/` contém códigos de exemplo conformes à nova gramática:

- **Correto1.ling** — fatorial iterativo (if/else e while com chaves, precedência).
- **Correto2.ling** — herança, despacho dinâmico, vetores e `length`.
- **Erro1_Sintatico.ling** — if/else sem chaves (erro sintático).
- **Erro2_Semantico.ling** — incompatibilidades de tipo e classe vazia (erros semânticos).

A subpasta `testes/unidade1_legado/` guarda os códigos da Unidade 1
(`Program1`–`Program5`), que usam if/else sem chaves e por isso **não** são
válidos na gramática nova — ficam apenas como referência histórica.

# Autores

Desenvolvido como trabalho prático para a disciplina de Compiladores (DIM0164). Alunos:

-> Carlos Alberto de Lima Neto
-> Claudivan Costa de Lima Júnior
-> Luiz Guilherme Carvalho Viana
-> Osvaldo Heitor de Andrade Tavares de Souza

