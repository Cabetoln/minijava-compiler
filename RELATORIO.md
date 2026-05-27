# Relatório Técnico: Compilador MiniJava

## 1. Introdução

Este trabalho implementa um compilador para MiniJava, um subset da linguagem Java utilizado na disciplina de Compiladores. O compilador é composto por quatro etapas principais: pré-processamento, análise léxica, análise sintática e construção da tabela de símbolos.

## 2. Arquitetura

```
Arquivo Fonte
    ↓
[Pré-processador] → Remove comentários, minifica espaços
    ↓
[Léxico] → Gera tokens com linha/coluna
    ↓
[Parser] → Valida sintaxe, popula tabela de símbolos
    ↓
[Saída] → Mensagem de sucesso e tabela de símbolos
```

## 3. Estruturas de Dados

### 3.1 Token (lexo.cpp)

```cpp
struct Token {
    TipoToken tipo;      // Tipo enum (35 valores)
    std::string valor;   // Valor literal ("class", "42", "myVar")
    std::string subtipo; // Categoria textual ("KEYWORD", "NUMBER", "ID")
    int linha;           // Linha no arquivo
    int coluna;          // Coluna no arquivo
};
```

**Utilidade**: Cada token é uma unidade léxica com metadados de posição para error reporting preciso.

### 3.2 Symbol (symbol_table.cpp)

```cpp
struct Symbol {
    std::string name;      // Nome do símbolo
    std::string type;      // Tipo (int, boolean, int[], String, ClassName)
    std::string category;  // Categoria (CLASS, METHOD, VARIABLE, PARAMETER)
    std::string scope;     // Escopo (GLOBAL, ClassName, ClassName.methodName)
    int line;              // Linha de declaração
};
```

**Utilidade**: Registra cada símbolo do programa com informações de tipo e escopo para futuras análises semânticas.

### 3.3 SymbolTable (symbol_table.cpp)

```cpp
class SymbolTable {
    std::vector<Symbol> symbols;  // Lista de símbolos
    
    void addSymbol(...);          // Adiciona símbolo
    bool isDeclared(name, scope); // Verifica redeclaração
    Symbol* lookup(name);         // Busca por nome
    void printTable();            // Exibe formatado
};
```

**Utilidade**: Gerencia símbolos do programa com operações CRUD e detecção de conflitos.

## 4. Técnicas Utilizadas

### 4.1 Pré-processador: Máquina de Estados Simples

O pré-processador usa scanning linear com estados implícitos:

```
NORMAL → (encontrou "/") → CHECK_COMMENT
CHECK_COMMENT → ("/" → SKIP_LINE) | ("*" → SKIP_BLOCK) | (outro → NORMAL)
SKIP_LINE → (char != '\n') → SKIP_LINE | ('\n' → NORMAL)
SKIP_BLOCK → (encontrou "*/") → NORMAL
```

**Vantagens**: Simples, eficiente O(n), suporta comentários aninhados.

### 4.2 Léxico: Scanning com Lookahead

Implementa máquina de estados com transições baseadas em categorias de caracteres:

- **Identificadores**: `[a-zA-Z_][a-zA-Z0-9_]*`
- **Números**: `[0-9]+` com validação
- **Operadores simples**: Map `char → TipoToken`
- **Operador composto**: `&&` com lookahead de 1 caractere
- **Rastreamento de posição**: Contador linha/coluna atualizado em cada avanço

**Vantagens**: Determinístico, suporta error recovery, preserva posição precisa.

### 4.3 Parser: Descendente Recursivo com Gramática Transformada

Técnica: **Eliminação de Recursão à Esquerda + Factorização à Esquerda**

**Exemplo de transformação**:
```
Original (recursiva à esquerda):
Exp → Exp && Exp | Exp > Exp | ... | PrimaryExp

Transformada (iterativa):
Exp → AndExp
AndExp → CompExp (&&CompExp)*     // * = zero ou mais
CompExp → AddExp (> | < AddExp)?   // ? = zero ou um
```

**Padrão utilizado**: Recursão indireta via funções chamadas em cadeia:
- `parseExpression()` → `parseAndExp()` → `parseCompExp()` → `parseAddExp()` → ...

**Vantagens**:
- Sem recursão à esquerda (não cause stack overflow)
- Determina precedência por nível de função
- Implementação limpa com cada função = um nível de precedência

### 4.4 Recuperação de Erros

Estratégia: **Parada no primeiro erro significativo**

```cpp
bool expect(TipoToken tipo, const std::string& msg) {
    if (current().tipo != tipo) {
        erros.push_back("L" + std::to_string(...) + " esperado " + msg);
        return false;  // Continua parsing para encontrar outros erros
    }
    avancar();
    return true;
}
```

**Vantagens**: Relatório detalhado com posição precisa.

### 4.5 Tabela de Símbolos: Escopo Simples

Estratégia: **Concatenação de nomes para representar escopo**

```
Classe Programa:
  - Símbolo "Programa" com escopo "GLOBAL"
  
Método main em Programa:
  - Símbolo "main" com escopo "Programa"
  - Variáveis locais com escopo "Programa.main"
  - Parâmetros com escopo "Programa.main"
```

**Verificação de redeclaração**:
```cpp
bool isDeclared(const std::string& name, const std::string& scope) {
    for (const auto& sym : symbols) {
        if (sym.name == name && sym.scope == scope) {
            return true;  // Redeclaração no mesmo escopo
        }
    }
    return false;
}
```

**Vantagens**: Linear em lookup, sem necessidade de estruturas aninhadas.

## 5. Fluxo de Execução

1. **Ler arquivo** → string bruta
2. **Pré-processar** → remover comentários, preservar `\n` → string processada
3. **Léxico** → gerar tokens com posição → `vector<Token>`
4. **Validar léxico** → Se erros, exibir e sair
5. **Parser** → 
   - Valida sintaxe
   - Popula tabela de símbolos
   - Detecta redeclarações
6. **Exibir resultado** → Mensagem + tabela formatada

## 6. Correções Implementadas

### 6.1 Operador `<` Faltante

**Problema**: Operador menor-que não estava mapeado.

**Solução**:
```cpp
// Adicionar ao enum
OP_LT,

// Adicionar ao map
{'<', {TipoToken::OP_LT, "LT"}}
```

### 6.2 Preservação de Linhas

**Problema**: Pré-processador eliminava `\n`, fazendo todas as posições ficarem linha 1.

**Solução**: Ao remover comentário de linha, mantém um espaço em seu lugar:
```cpp
if (i + 1 < n && source[i] == '/' && source[i + 1] == '/') {
    i += 2;
    while (i < n && source[i] != '\n') i++;
    if (i < n) {
        result += ' ';   // Preserva espaço (token separator)
        i++;             // Avança no '\n'
    }
    continue;
}
```

Desta forma, `\n` é preservado pelo lexer, mantendo contagem correta de linhas.

## 7. Complexidade

| Operação | Complexidade | Observação |
|----------|-------------|-----------|
| Pré-processamento | O(n) | Scan linear |
| Léxico | O(n·m) | n = caracteres, m = comprimento médio token |
| Parser | O(n) | n = tokens (derivação linear) |
| Tabela de símbolos | O(k²) | k = símbolos (linear per operation) |
| **Total** | **O(n·m + k²)** | Dominado por léxico e tabela |

## 8. Testes

Arquivo de teste: `Program1.ling` contém:
- Classe principal com método main
- Classe auxiliar com herança
- Variáveis locais
- Expressões com múltiplos operadores
- Estruturas de controle (if/else)

Saída esperada: Tabela com 8+ símbolos registrados corretamente com escopos.

## 9. Limitações e Extensões Futuras

### Limitações Atuais
- Sem análise semântica (type checking, verificação de escopo de referências)
- Sem geração de código
- Sem otimizações

### Extensões Possíveis
1. **Análise Semântica**: Verificar tipos, validar referências a símbolos
2. **Geração de Bytecode/Assembly**: Produzir código intermediário
3. **Otimizações**: Constant folding, dead code elimination
4. **Debugging**: Símbolos para debugger, linhas de código original

## 10. Conclusão

Este compilador implementa com sucesso as três primeiras etapas de compilação (pré-processamento, análise léxica, análise sintática) com técnicas clássicas e estruturas de dados eficientes. A eliminação de recursão à esquerda garante parsing determinístico, e a tabela de símbolos simples estabelece base para futuras análises semânticas.

---

**Data**: 27 de maio de 2026  
**Disciplina**: DIM0164 - Compiladores  
**Professor**: Valdigleis S. Costa
