#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
 
// Lê o arquivo inteiro e retorna como string
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Erro ao abrir o arquivo: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
 
// Remove comentários e espaços excessivos, retorna o código processado como string
std::string preprocess(const std::string& source) {
    std::string result;
    result.reserve(source.size());
 
    size_t i = 0;
    size_t n = source.size();
 
    while (i < n) {
        // Comentário de linha única: //
        if (i + 1 < n && source[i] == '/' && source[i + 1] == '/') {
            i += 2;
            while (i < n && source[i] != '\n') {
                i++;
            }
            // Mantém o '\n' para não juntar tokens de linhas diferentes
            if (i < n) {
                result += ' ';
                i++; // consome o '\n'
            }
            continue;
        }
 
        // Comentário de múltiplas linhas: /* ... */
        if (i + 1 < n && source[i] == '/' && source[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(source[i] == '*' && source[i + 1] == '/')) {
                i++;
            }
            if (i + 1 >= n) {
                throw std::runtime_error("Comentário de bloco não fechado (falta */)");
            }
            i += 2; // consome */
            result += ' '; // substitui por espaço para não juntar tokens
            continue;
        }
 
        // Qualquer espaço branco: espaço, tab, \n, \r
        if (source[i] == ' ' || source[i] == '\t' ||
            source[i] == '\n' || source[i] == '\r') {
            // Adiciona um único espaço se o resultado não terminar já com espaço
            if (!result.empty() && result.back() != ' ') {
                result += ' ';
            }
            i++;
            continue;
        }
 
        result += source[i];
        i++;
    }
 
    // Remove espaço final, se houver
    if (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
 
    return result;
}
 
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0] << " <arquivo.java>\n";
        return 1;
    }
 
    try {
        std::string source = readFile(argv[1]);
        std::string processed = preprocess(source);
        std::cout << processed << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << "\n";
        return 1;
    }
 
    return 0;
}