# Compilador MiniJava

Implementação de um compilador para MiniJava (subset de Java) com pré-processador, analisador léxico, analisador sintático descendente e tabela de símbolos.

## Estrutura do Projeto

- **main.cpp** — Arquivo principal que orquestra todo o compilador
- **pre-processador.cpp** — Remove comentários e minifica o código
- **lexo.cpp** — Analisador léxico que gera tokens
- **parser.cpp** — Analisador sintático descendente recursivo
- **symbol_table.cpp** — Tabela de símbolos com suporte a escopos global e local

## Compilação

### Linux/Git Bash com g++

```bash
cd minijava-compiler
g++ -std=c++17 -Wall -Wextra -o compilador main.cpp
```

Ou use o script:
```bash
./build.sh
```

### Windows com MinGW

```bash
g++ -std=c++17 -Wall -Wextra -o compilador.exe main.cpp
```

Ou use o script batch:
```bash
build.bat
```

### Windows com Visual Studio (MSVC)

```bash
cl.exe /std:c++latest /W3 /EHsc main.cpp /Fe:compilador.exe
```

## Execução

```bash
./compilador arquivo.java
```

Exemplo com o arquivo de teste:
```bash
./compilador Program1.ling
```

## Saída

Se o código for sintaticamente correto:
```
✓ Código sintaticamente correto!

┌──────────────────────────────────────────────────────────┐
│ Tabela de Símbolos                                       │
├──────────────────────────────────────────────────────────┤
│ Nome    │ Tipo    │ Categoria │ Escopo           │ Linha │
├──────────────────────────────────────────────────────────┤
│ Programa│ CLASS   │ CLASS     │ GLOBAL           │ 4     │
│ main    │ void    │ METHOD    │ Programa         │ 5     │
│ a       │ String[]│ PARAMETER │ Programa.main    │ 5     │
│ ...     │ ...     │ ...       │ ...              │ ...   │
└──────────────────────────────────────────────────────────┘
```

Se houver erros:
```
Erros léxicos encontrados:
L5:C10 caractere desconhecido '@'

Erros sintáticos encontrados:
L14:C5 esperado ; após atribuição
```

## Features Implementadas

### Pré-processador
- ✅ Remove comentários de linha (`//`)
- ✅ Remove comentários de bloco (`/* */`)
- ✅ Minifica espaços em branco preservando `\n` para rastreamento correto de linhas
- ✅ Retorna string processada

### Analisador Léxico
- ✅ Reconhece 21 palavras-chave (class, public, static, int, boolean, etc)
- ✅ Reconhece identificadores e números
- ✅ Reconhece operadores: `&&`, `>`, `<`, `+`, `-`, `*`, `!`, `=`
- ✅ Reconhece separadores: `{ } ( ) [ ] ; , .`
- ✅ Rastreamento de linha e coluna
- ✅ Coleta de erros léxicos

### Analisador Sintático
- ✅ Parser descendente recursivo
- ✅ Gramática sem recursão à esquerda (transformada para precedência correta)
- ✅ Suporta estrutura completa de MiniJava:
  - Classes com herança (extends)
  - Método main obrigatório
  - Múltiplas classes adicionais
  - Declaração de variáveis
  - Declaração de métodos
  - Comandos: atribuição, if/else, while, System.out.println
  - Expressões com precedência correta (AND, comparação, adição, multiplicação, unário)
  - Acesso a arrays e métodos
- ✅ Recuperação de erros com mensagens precisas (linha e coluna)

### Tabela de Símbolos
- ✅ Rastreamento de classes (escopo GLOBAL)
- ✅ Rastreamento de métodos com escopo de classe
- ✅ Rastreamento de variáveis locais e parâmetros com escopo de método
- ✅ Detecção de redeclarações no mesmo escopo
- ✅ Exibição formatada em tabela

## Gramática Aceita

```
Prog → MainC DefCl*
MainC → class Id { public static void main ( String [ ] Id ) { Cmd } }
DefCl → class Id [ extends Id ] { DefVar DefMet }
DefVar → (Type Id ;)*
DefMet → (public Type Id ( [Args] ) { DefVar Cmd return Exp ; })*
Type → int[] | boolean | int | Id
Args → Type Id (,Type Id)*
Cmd → { Cmd }
    | if ( Exp ) Cmd [else Cmd]
    | while ( Exp ) Cmd
    | System . out . println ( Exp ) ;
    | Id = Exp ;
    | Id [ Exp ] = Exp ;

Exp → AndExp
AndExp → CompExp (&&CompExp)*
CompExp → AddExp ((> | <) AddExp)?
AddExp → MulExp ((+ | -) MulExp)*
MulExp → UnaryExp (* UnaryExp)*
UnaryExp → ! UnaryExp | PrimaryExp
PrimaryExp → ID | Number | true | false | this | ( Exp ) | new Id ( ) | new int [ Exp ]
PostfixExp → [ Exp ] | . length | . ID ( [ListExp] )
ListExp → Exp (, Exp)*
```

## Correções Realizadas

1. **Operador `<` adicionado**: Bug onde operador menor-que não estava sendo tokenizado
2. **Preservação de newlines**: Pré-processador agora preserva `\n` para rastreamento correto de linhas no léxico
3. **Função preprocessar()**: Renomeada de `preprocess()` para manter consistência com lexo.cpp

## Requisitos

- C++17 ou superior
- G++ (MinGW) ou compilador compatível
- Arquivo de entrada com código MiniJava válido

## Testes

Incluído arquivo de teste `Program1.ling` com exemplos de:
- Classe principal com método main
- Classe auxiliar com herança implícita
- Variáveis locais
- Expressões com operadores
- Controle de fluxo (if/else)

## Limitações Conhecidas

- Não há análise semântica (verificação de tipos, escopo de variáveis)
- Não há geração de código intermediário
- Não há otimizações de código
- Redeclarações em escopos diferentes são permitidas (apenas mesmo escopo é detectado)

## Autores

Desenvolvido como trabalho prático para a disciplina de Compiladores (DIM0164).
