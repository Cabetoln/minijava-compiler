#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <iomanip>
#include <memory>

#include "pre-processador.cpp"
#include "lexo.cpp"
#include "symbol_table.cpp"
#include "parser.cpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo.java>\n";
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);
        std::string processed = preprocessar(source);

        Lexer lexer(processed);
        std::vector<Token> tokens = lexer.tokenizar();

        if (!lexer.avisos.empty()) {
            std::cerr << "Avisos léxicos:\n";
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

        Parser parser(tokens);
        bool success = parser.parse();

        if (!success || !parser.erros.empty()) {
            std::cerr << "Erros sintáticos encontrados:\n";
            for (const auto& erro : parser.erros) {
                std::cerr << erro << "\n";
            }
            return 1;
        }

        std::cout << "\n✓ Código sintaticamente correto!\n";
        parser.getSymbolTable().printTable();

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }
}

