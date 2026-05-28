#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>

std::string preprocessar(const std::string& source);

// ─────────────────────────────────────────────
//  TOKENS
// ─────────────────────────────────────────────

enum class TipoToken {
    KW_CLASS, KW_PUBLIC, KW_STATIC, KW_VOID, KW_MAIN,
    KW_STRING, KW_EXTENDS, KW_RETURN, KW_IF, KW_ELSE,
    KW_WHILE, KW_TRUE, KW_FALSE, KW_THIS, KW_NEW,
    KW_INT, KW_BOOLEAN, KW_LENGTH, KW_PRINTLN, KW_SYSTEM, KW_OUT,
    ID, NUMBER,
    OP_AND, OP_GT, OP_LT, OP_PLUS, OP_MINUS, OP_TIMES, OP_NOT, OP_ASSIGN,
    SEP_LBRACE, SEP_RBRACE, SEP_LPAREN, SEP_RPAREN,
    SEP_LBRACK, SEP_RBRACK, SEP_SEMI, SEP_COMMA, SEP_DOT,
    TOKEN_EOF, TOKEN_ERRO
};

struct Token {
    TipoToken   tipo;
    std::string valor;
    std::string subtipo;
    int         linha;
    int         coluna;
};

std::string categoria(TipoToken t) {
    switch (t) {
        case TipoToken::ID:         return "ID";
        case TipoToken::NUMBER:     return "NUMBER";
        case TipoToken::TOKEN_EOF:  return "EOF";
        case TipoToken::TOKEN_ERRO: return "ERRO";
        case TipoToken::OP_AND: case TipoToken::OP_GT: case TipoToken::OP_LT:
        case TipoToken::OP_PLUS: case TipoToken::OP_MINUS:
        case TipoToken::OP_TIMES: case TipoToken::OP_NOT:
        case TipoToken::OP_ASSIGN:  return "OP";
        case TipoToken::SEP_LBRACE: case TipoToken::SEP_RBRACE:
        case TipoToken::SEP_LPAREN: case TipoToken::SEP_RPAREN:
        case TipoToken::SEP_LBRACK: case TipoToken::SEP_RBRACK:
        case TipoToken::SEP_SEMI: case TipoToken::SEP_COMMA:
        case TipoToken::SEP_DOT:    return "SEP";
        default:                    return "KEYWORD";
    }
}


// ─────────────────────────────────────────────
//  SUGESTÃO DE IDENTIFICADORES
// ─────────────────────────────────────────────

// Calcula distância de edição mínima entre duas strings
static int levenshtein(const std::string& a, const std::string& b) {
    int m = a.size(), n = b.size();
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1));
    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;
    for (int i = 1; i <= m; i++)
        for (int j = 1; j <= n; j++)
            dp[i][j] = (a[i-1] == b[j-1])
                ? dp[i-1][j-1]
                : 1 + std::min(dp[i-1][j], std::min(dp[i][j-1], dp[i-1][j-1]));
    return dp[m][n];
}

// Retorna a keyword mais proxima se distancia <= 2, senao string vazia
static std::string keywordMaisProxima(const std::string& w) {
    static const std::vector<std::string> KWS = {
        "class","public","static","void","main","String","extends",
        "return","if","else","while","true","false","this","new",
        "int","boolean","length","println","System","out"
    };
    std::string melhor;
    int menorDist = 2; // limiar: so sugere se distancia <= 1
    for (const auto& kw : KWS) {
        int d = levenshtein(w, kw);
        if (d < menorDist) { menorDist = d; melhor = kw; }
    }
    return melhor;
}

// ─────────────────────────────────────────────
//  ANALISADOR LÉXICO
// ─────────────────────────────────────────────

class Lexer {
    std::string src;
    size_t pos;
    int linha, coluna;

    static const std::set<std::string> KEYWORDS;
    static const std::map<char, std::pair<TipoToken,std::string>> OPERADORES;
    static const std::map<char, std::pair<TipoToken,std::string>> SEPARADORES;

    char cur() { return pos < src.size() ? src[pos] : '\0'; }

    char avancar() {
        char c = src[pos++];
        if (c == '\n') { linha++; coluna = 1; } else coluna++;
        return c;
    }

    void skip() {
        while (pos < src.size() && (cur()==' '||cur()=='\t'||cur()=='\r'||cur()=='\n'))
            avancar();
    }

    TipoToken kwTipo(const std::string& w) {
        if (w=="class")   return TipoToken::KW_CLASS;
        if (w=="public")  return TipoToken::KW_PUBLIC;
        if (w=="static")  return TipoToken::KW_STATIC;
        if (w=="void")    return TipoToken::KW_VOID;
        if (w=="main")    return TipoToken::KW_MAIN;
        if (w=="String")  return TipoToken::KW_STRING;
        if (w=="extends") return TipoToken::KW_EXTENDS;
        if (w=="return")  return TipoToken::KW_RETURN;
        if (w=="if")      return TipoToken::KW_IF;
        if (w=="else")    return TipoToken::KW_ELSE;
        if (w=="while")   return TipoToken::KW_WHILE;
        if (w=="true")    return TipoToken::KW_TRUE;
        if (w=="false")   return TipoToken::KW_FALSE;
        if (w=="this")    return TipoToken::KW_THIS;
        if (w=="new")     return TipoToken::KW_NEW;
        if (w=="int")     return TipoToken::KW_INT;
        if (w=="boolean") return TipoToken::KW_BOOLEAN;
        if (w=="length")  return TipoToken::KW_LENGTH;
        if (w=="println") return TipoToken::KW_PRINTLN;
        if (w=="System")  return TipoToken::KW_SYSTEM;
        if (w=="out")     return TipoToken::KW_OUT;
        return TipoToken::ID;
    }

public:
    std::vector<std::string> erros;
    std::vector<std::string> avisos;

    Lexer(const std::string& fonte)
        : src(fonte), pos(0), linha(1), coluna(1) {}

    std::vector<Token> tokenizar() {
        std::vector<Token> tokens;
        while (true) {
            skip();
            if (pos >= src.size()) {
                tokens.push_back({TipoToken::TOKEN_EOF, "", "EOF", linha, coluna});
                break;
            }

            int l = linha, c = coluna;
            char ch = cur();

            // Identificador ou keyword: [a-zA-Z_][a-zA-Z0-9_]*
            if (isalpha(ch) || ch == '_') {
                std::string w;
                while (pos < src.size() && (isalnum(cur()) || cur() == '_'))
                    w += avancar();
                if (KEYWORDS.count(w)) {
                    std::string sub = w;
                    for (char& x : sub) x = toupper(x);
                    tokens.push_back({kwTipo(w), w, sub, l, c});
                } else {
                    // Verifica se parece com uma keyword (possivel typo)
                    // Ignora IDs curtos (<=2 chars) para evitar falsos positivos
                    // Emite apenas aviso, token continua sendo ID valido
                    std::string kw = (w.size() >= 3) ? keywordMaisProxima(w) : "";
                    if (!kw.empty()) {
                        avisos.push_back("L" + std::to_string(l) + ":C" + std::to_string(c)
                            + " aviso: identificador '" + w + "' - voce quis dizer '" + kw + "'?");
                    }
                    tokens.push_back({TipoToken::ID, w, "ID", l, c});
                }
                continue;
            }

            // Número: [0-9]+
            if (isdigit(ch)) {
                std::string w;
                while (pos < src.size() && isdigit(cur())) w += avancar();
                if (pos < src.size() && (isalpha(cur()) || cur() == '_')) {
                    while (pos < src.size() && (isalnum(cur()) || cur() == '_'))
                        w += avancar();
                    // Extrai a parte alfabetica como sugestao de identificador
                    std::string parteAlfa;
                    for (char x : w) if (isalpha(x) || x == '_') parteAlfa += x;
                    std::string sugestao = "";
                    if (!parteAlfa.empty()) {
                        std::string kw = keywordMaisProxima(parteAlfa);
                        sugestao = " - voce quis dizer '" + (!kw.empty() ? kw : parteAlfa) + "'?";
                    }
                    erros.push_back("L" + std::to_string(l) + ":C" + std::to_string(c)
                        + " numero invalido '" + w + "'" + sugestao);
                    tokens.push_back({TipoToken::TOKEN_ERRO, w, "NUM_INVALIDO", l, c});
                } else {
                    tokens.push_back({TipoToken::NUMBER, w, "NUMBER", l, c});
                }
                continue;
            }

            // Operador &&
            if (ch == '&') {
                avancar();
                if (cur() == '&') {
                    avancar();
                    tokens.push_back({TipoToken::OP_AND, "&&", "AND", l, c});
                } else {
                    erros.push_back("L" + std::to_string(l) + ":C" + std::to_string(c)
                        + " '&' isolado, use '&&'");
                    tokens.push_back({TipoToken::TOKEN_ERRO, "&", "AND_INC", l, c});
                }
                continue;
            }

            // Operadores simples
            auto itOp = OPERADORES.find(ch);
            if (itOp != OPERADORES.end()) {
                avancar();
                tokens.push_back({itOp->second.first, std::string(1,ch), itOp->second.second, l, c});
                continue;
            }

            // Separadores
            auto itSep = SEPARADORES.find(ch);
            if (itSep != SEPARADORES.end()) {
                avancar();
                tokens.push_back({itSep->second.first, std::string(1,ch), itSep->second.second, l, c});
                continue;
            }

            // Caractere desconhecido
            avancar();
            erros.push_back("L" + std::to_string(l) + ":C" + std::to_string(c)
                + " caractere desconhecido '" + ch + "'");
            tokens.push_back({TipoToken::TOKEN_ERRO, std::string(1,ch), "DESCONHECIDO", l, c});
        }
        return tokens;
    }
};

const std::set<std::string> Lexer::KEYWORDS = {
    "class","public","static","void","main","String","extends",
    "return","if","else","while","true","false","this","new",
    "int","boolean","length","println","System","out"
};
const std::map<char,std::pair<TipoToken,std::string>> Lexer::OPERADORES = {
    {'>',{TipoToken::OP_GT,"GT"}},   {'<',{TipoToken::OP_LT,"LT"}},
    {'+',{TipoToken::OP_PLUS,"PLUS"}},
    {'-',{TipoToken::OP_MINUS,"MINUS"}}, {'*',{TipoToken::OP_TIMES,"TIMES"}},
    {'!',{TipoToken::OP_NOT,"NOT"}},  {'=',{TipoToken::OP_ASSIGN,"ASSIGN"}}
};
const std::map<char,std::pair<TipoToken,std::string>> Lexer::SEPARADORES = {
    {'{',{TipoToken::SEP_LBRACE,"LBRACE"}},{'}',{TipoToken::SEP_RBRACE,"RBRACE"}},
    {'(',{TipoToken::SEP_LPAREN,"LPAREN"}},{')',{TipoToken::SEP_RPAREN,"RPAREN"}},
    {'[',{TipoToken::SEP_LBRACK,"LBRACK"}},{']',{TipoToken::SEP_RBRACK,"RBRACK"}},
    {';',{TipoToken::SEP_SEMI,"SEMI"}},    {',',{TipoToken::SEP_COMMA,"COMMA"}},
    {'.',{TipoToken::SEP_DOT,"DOT"}}
};



