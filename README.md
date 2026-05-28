# Compilador MiniJava

Implementação de um compilador para MiniJava (subset de Java) com pré-processador, analisador léxico, analisador sintático descendente e tabela de símbolos.

# Estrutura do Projeto

- **main.cpp** — Arquivo principal que orquestra todo o compilador
- **pre-processador.cpp** — Remove comentários e minifica o código
- **lexo.cpp** — Analisador léxico que gera tokens
- **parser.cpp** — Analisador sintático descendente recursivo
- **symbol_table.cpp** — Tabela de símbolos com suporte a escopos global e local

# Compilação

Para compilar o projeto, basta rodar:

g++ main.cpp -o compilador

Para executar o projeto, basta rodar:

./compilador <arquivo.ling>

# Autores

Desenvolvido como trabalho prático para a disciplina de Compiladores (DIM0164). Alunos:

-> Carlos Alberto de Lima Neto
-> Claudivan Costa de Lima Júnior
-> Luiz Guilherme Carvalho Viana
-> Osvaldo Heitor de Andrade Tavares de Souza

