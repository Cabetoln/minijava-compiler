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
std::string preprocessar(const std::string& source) {
    std::string result;
    result.reserve(source.size());
 
    size_t i = 0;
    size_t n = source.size();
 
    while (i < n) {
        // Comentário de bloco: /* ... */
        if (i + 1 < n && source[i] == '/' && source[i + 1] == '*') {
            i += 2;
            while (i + 1 < n && !(source[i] == '*' && source[i + 1] == '/')) {
                if (source[i] == '\n') result += '\n';
                i++;
            }
            i += 2; // consome o */
            continue;
        }

        // Comentário de linha única: //
        if (i + 1 < n && source[i] == '/' && source[i + 1] == '/') {
            i += 2;
            while (i < n && source[i] != '\n') {
                i++;
            }
            // Mantém o '\n' para não juntar tokens de linhas diferentes
            if (i < n) {
                result += '\n';
                i++;
            }
            continue;
        }
 
        
 
        // Quebra de linha: preserva para o léxico rastrear número de linha
        if (source[i] == '\n') {
                result += '\n';
            i++;
            continue;
        }
        if (source[i] == '\r') {
            i++;
            continue;
        }
        // Espaço horizontal: colapsa múltiplos em um só
        if (source[i] == ' ' || source[i] == '\t') {
            char last = result.empty() ? '\0' : result.back();
            if (last != ' ' && last != '\n')
                result += ' ';
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
 
