#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Lê o arquivo inteiro e retorna como string.
// O reconhecimento de comentários passou a ser responsabilidade do
// analisador léxico, então não há mais etapa de pré-processamento.
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Erro ao abrir o arquivo: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
