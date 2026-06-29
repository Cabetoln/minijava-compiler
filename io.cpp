#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

// Le o arquivo inteiro e devolve como string.
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Erro ao abrir o arquivo: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
