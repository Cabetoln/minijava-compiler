#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <memory>

#include "io.cpp"
#include "lexo.cpp"
#include "symbol_table.cpp"
#include "parser.cpp"

// ─────────────────────────────────────────────
//  FLAGS DE LINHA DE COMANDO
// ─────────────────────────────────────────────

struct Opcoes {
    std::string arquivo;
    bool tokens          = false; // --tokens   : imprime a lista de tokens
    bool ast             = false; // --ast      : imprime a árvore sintática abstrata
    bool tabela          = false; // --tabela   : imprime a tabela de símbolos
    bool sugestoes       = false; // --sugestoes: imprime sugestões de correção
    bool pararPrimeiroErro = false; // --parar  : para no primeiro erro léxico
    bool ajuda           = false;
};

void imprimirAjuda(const char* prog) {
    std::cout <<
        "Uso: " << prog << " <arquivo.ling> [flags]\n\n"
        "Flags:\n"
        "  --tokens      Imprime a lista de tokens gerada pelo léxico.\n"
        "  --ast         Imprime a árvore sintática abstrata (AST).\n"
        "  --tabela      Imprime a tabela de símbolos após a análise sintática.\n"
        "  --sugestoes   Exibe sugestões de correção léxica e sintática.\n"
        "  --parar       Para no primeiro erro léxico (senão, processa tudo).\n"
        "  --help        Mostra esta ajuda.\n";
}

Opcoes parseArgs(int argc, char* argv[]) {
    Opcoes op;
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        if      (a == "--tokens")    op.tokens = true;
        else if (a == "--ast")       op.ast = true;
        else if (a == "--tabela")    op.tabela = true;
        else if (a == "--sugestoes") op.sugestoes = true;
        else if (a == "--parar")     op.pararPrimeiroErro = true;
        else if (a == "--help" || a == "-h") op.ajuda = true;
        else if (!a.empty() && a[0] == '-') {
            std::cerr << "Flag desconhecida: " << a << "\n";
        } else {
            op.arquivo = a;
        }
    }
    return op;
}

// ─────────────────────────────────────────────
//  IMPRESSÃO DA LISTA DE TOKENS
// ─────────────────────────────────────────────

void imprimirTokens(const std::vector<Token>& tokens) {
    std::cout << "\nLista de Tokens:\n";
    std::cout << std::left
              << std::setw(8)  << "Linha"
              << std::setw(8)  << "Coluna"
              << std::setw(12) << "Categoria"
              << std::setw(16) << "Subtipo"
              << "Valor\n";
    std::cout << std::string(52, '-') << "\n";
    for (const auto& t : tokens) {
        std::cout << std::left
                  << std::setw(8)  << t.linha
                  << std::setw(8)  << t.coluna
                  << std::setw(12) << categoria(t.tipo)
                  << std::setw(16) << t.subtipo
                  << t.valor << "\n";
    }
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    Opcoes op = parseArgs(argc, argv);

    if (op.ajuda) { imprimirAjuda(argv[0]); return 0; }
    if (op.arquivo.empty()) {
        std::cerr << "Uso: " << argv[0] << " <arquivo.ling> [flags]\n";
        std::cerr << "Use --help para ver as flags disponíveis.\n";
        return 1;
    }

    try {
        std::string source = readFile(op.arquivo);

        // ── Análise léxica ──────────────────────────────
        Lexer lexer(source);
        lexer.pararNoPrimeiroErro = op.pararPrimeiroErro;
        std::vector<Token> tokens = lexer.tokenizar();

        if (op.tokens)
            imprimirTokens(tokens);

        // Sugestões léxicas (typos de palavras-chave) só com a flag
        if (op.sugestoes && !lexer.avisos.empty()) {
            std::cerr << "Sugestões léxicas:\n";
            for (const auto& aviso : lexer.avisos)
                std::cerr << aviso << "\n";
            std::cerr << "\n";
        }

        if (!lexer.erros.empty()) {
            std::cerr << "Erros léxicos encontrados:\n";
            for (const auto& erro : lexer.erros)
                std::cerr << erro << "\n";
            return 1;
        }

        // ── Análise sintática ───────────────────────────
        Parser parser(tokens);
        bool success = parser.parse();

        if (!success || !parser.erros.empty()) {
            std::cerr << "Erros sintáticos encontrados:\n";
            for (const auto& erro : parser.erros)
                std::cerr << erro << "\n";
            return 1;
        }

        std::cout << "\n✓ Código sintaticamente correto!\n";

        // ── Saídas condicionadas a flags ────────────────
        if (op.ast) {
            // TODO (Fase 2): impressão da AST quando o parser construí-la.
            std::cout << "\n[--ast] A construção da AST será adicionada na Fase 2.\n";
        }

        if (op.tabela)
            parser.getSymbolTable().printTable();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }
}
