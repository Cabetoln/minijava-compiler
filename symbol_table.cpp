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

    std::vector<Symbol> getSymbolsByScope(const std::string& scope) {
        std::vector<Symbol> result;
        for (const auto& sym : symbols) {
            if (sym.scope == scope) {
                result.push_back(sym);
            }
        }
        return result;
    }

    void printTable() {
        if (symbols.empty()) {
            std::cout << "Tabela de Símbolos vazia.\n";
            return;
        }

        std::cout << "\n┌─────────────────────────────────────────────────────────────────────────────┐\n";
        std::cout << "│ Tabela de Símbolos                                                          │\n";
        std::cout << "├─────────────────────────────────────────────────────────────────────────────┤\n";
        std::cout << "│ "
                  << std::left << std::setw(15) << "Nome"
                  << "│ " << std::setw(12) << "Tipo"
                  << "│ " << std::setw(12) << "Categoria"
                  << "│ " << std::setw(20) << "Escopo"
                  << "│ " << std::setw(4) << "Linha" << "│\n";
        std::cout << "├─────────────────────────────────────────────────────────────────────────────┤\n";

        for (const auto& sym : symbols) {
            std::cout << "│ "
                      << std::left << std::setw(15) << sym.name
                      << "│ " << std::setw(12) << sym.type
                      << "│ " << std::setw(12) << sym.category
                      << "│ " << std::setw(20) << sym.scope
                      << "│ " << std::setw(4) << sym.line << "│\n";
        }

        std::cout << "└─────────────────────────────────────────────────────────────────────────────┘\n";
    }

    std::vector<Symbol> getAll() {
        return symbols;
    }
};
