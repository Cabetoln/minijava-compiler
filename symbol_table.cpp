#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

struct Symbol {
    std::string name;
    std::string type;
    std::string category;
    std::string scope;
    int line;
};

class SymbolTable {
private:
    std::vector<Symbol> symbols;

public:
    void addSymbol(const std::string& name, const std::string& type,
                   const std::string& category, const std::string& scope, int line) {
        if (isDeclared(name, scope)) {
            return;
        }
        symbols.push_back({name, type, category, scope, line});
    }

    bool isDeclared(const std::string& name, const std::string& scope) {
        for (const auto& sym : symbols) {
            if (sym.name == name && sym.scope == scope) {
                return true;
            }
        }
        return false;
    }

    Symbol* lookup(const std::string& name) {
        for (auto& sym : symbols) {
            if (sym.name == name) {
                return &sym;
            }
        }
        return nullptr;
    }

    void printTable() {
        if (symbols.empty()) {
            std::cout << "Tabela de simbolos vazia.\n";
            return;
        }

        std::cout << "\nTabela de Simbolos\n";
        std::cout << std::left
                  << std::setw(16) << "Nome"
                  << std::setw(13) << "Tipo"
                  << std::setw(13) << "Categoria"
                  << std::setw(22) << "Escopo"
                  << "Linha\n";
        std::cout << std::string(68, '-') << "\n";

        for (const auto& sym : symbols) {
            std::cout << std::left
                      << std::setw(16) << sym.name
                      << std::setw(13) << sym.type
                      << std::setw(13) << sym.category
                      << std::setw(22) << sym.scope
                      << sym.line << "\n";
        }
    }
};
