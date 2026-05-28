
class Parser {
private:
    std::vector<Token> tokens;
    size_t pos;
    SymbolTable symbolTable;
    std::string currentClass;
    std::string currentMethod;

    Token current() {
        return pos < tokens.size() ? tokens[pos] : tokens.back();
    }

    Token peek(int offset = 1) {
        return pos + offset < tokens.size() ? tokens[pos + offset] : tokens.back();
    }

    void avancar() {
        if (pos < tokens.size()) pos++;
    }

    bool match(TipoToken tipo) {
        if (current().tipo == tipo) {
            avancar();
            return true;
        }
        return false;
    }

    bool expect(TipoToken tipo, const std::string& msg) {
        if (current().tipo != tipo) {
            erros.push_back("L" + std::to_string(current().linha) + ":C" +
                std::to_string(current().coluna) + " esperado " + msg);
            return false;
        }
        avancar();
        return true;
    }

    std::string parseType() {
        if (match(TipoToken::KW_INT)) {
            if (match(TipoToken::SEP_LBRACK)) {
                expect(TipoToken::SEP_RBRACK, "]");
                return "int[]";
            }
            return "int";
        }
        if (match(TipoToken::KW_BOOLEAN)) return "boolean";
        if (current().tipo == TipoToken::ID) {
            std::string id = current().valor;
            avancar();
            return id;
        }
        erros.push_back("L" + std::to_string(current().linha) + ":C" +
            std::to_string(current().coluna) + " tipo inválido");
        return "error";
    }

    std::vector<std::pair<std::string, std::string>> parseArgs() {
        std::vector<std::pair<std::string, std::string>> args;
        if (current().tipo == TipoToken::SEP_RPAREN) {
            return args;
        }
        do {
            std::string type = parseType();
            if (current().tipo == TipoToken::ID) {
                std::string argName = current().valor;
                avancar();
                args.push_back({type, argName});
            } else {
                erros.push_back("L" + std::to_string(current().linha) + ":C" +
                    std::to_string(current().coluna) + " esperado identificador no argumento");
            }
        } while (match(TipoToken::SEP_COMMA));
        return args;
    }

    void parseVarDecl() {
        while (current().tipo == TipoToken::ID || current().tipo == TipoToken::KW_INT ||
               current().tipo == TipoToken::KW_BOOLEAN) {
            std::string type = parseType();
            if (current().tipo == TipoToken::ID) {
                std::string varName = current().valor;
                int line = current().linha;
                avancar();
                std::string scope = currentMethod.empty() ? currentClass + ":GLOBAL" : currentClass + "." + currentMethod;
                symbolTable.addSymbol(varName, type, "VARIABLE", scope, line);
                expect(TipoToken::SEP_SEMI, ";");
            } else {
                erros.push_back("L" + std::to_string(current().linha) + ":C" +
                    std::to_string(current().coluna) + " esperado identificador na declaração de variável");
                break;
            }
        }
    }

    void parseMethodDecl() {
        while (current().tipo == TipoToken::KW_PUBLIC) {
            avancar();
            std::string retType = parseType();
            if (current().tipo == TipoToken::ID) {
                std::string methodName = current().valor;
                int line = current().linha;
                currentMethod = methodName;
                avancar();
                expect(TipoToken::SEP_LPAREN, "(");

                auto args = parseArgs();
                for (const auto& arg : args) {
                    symbolTable.addSymbol(arg.second, arg.first, "PARAMETER", currentClass + "." + methodName, line);
                }

                expect(TipoToken::SEP_RPAREN, ")");
                expect(TipoToken::SEP_LBRACE, "{");

                symbolTable.addSymbol(methodName, retType, "METHOD", currentClass, line);

                parseVarDecl();
                parseCommand();

                expect(TipoToken::KW_RETURN, "return");
                parseExpression();
                expect(TipoToken::SEP_SEMI, ";");
                expect(TipoToken::SEP_RBRACE, "}");

                currentMethod = "";
            } else {
                erros.push_back("L" + std::to_string(current().linha) + ":C" +
                    std::to_string(current().coluna) + " esperado identificador no método");
                break;
            }
        }
    }

    void parseCommand() {
        while (current().tipo != TipoToken::SEP_RBRACE && current().tipo != TipoToken::KW_RETURN &&
               current().tipo != TipoToken::TOKEN_EOF) {
            if (match(TipoToken::SEP_LBRACE)) {
                parseCommand();
                expect(TipoToken::SEP_RBRACE, "}");
            } else if (match(TipoToken::KW_IF)) {
                expect(TipoToken::SEP_LPAREN, "(");
                parseExpression();
                expect(TipoToken::SEP_RPAREN, ")");
                parseCommand();
                if (match(TipoToken::KW_ELSE)) {
                    parseCommand();
                }
            } else if (match(TipoToken::KW_WHILE)) {
                expect(TipoToken::SEP_LPAREN, "(");
                parseExpression();
                expect(TipoToken::SEP_RPAREN, ")");
                parseCommand();
            } else if (current().tipo == TipoToken::KW_SYSTEM) {
                avancar();
                expect(TipoToken::SEP_DOT, ".");
                expect(TipoToken::KW_OUT, "out");
                expect(TipoToken::SEP_DOT, ".");
                expect(TipoToken::KW_PRINTLN, "println");
                expect(TipoToken::SEP_LPAREN, "(");
                parseExpression();
                expect(TipoToken::SEP_RPAREN, ")");
                expect(TipoToken::SEP_SEMI, ";");
            } else if (current().tipo == TipoToken::ID) {
                std::string id = current().valor;
                avancar();
                if (match(TipoToken::OP_ASSIGN)) {
                    parseExpression();
                    expect(TipoToken::SEP_SEMI, ";");
                } else if (match(TipoToken::SEP_LBRACK)) {
                    parseExpression();
                    expect(TipoToken::SEP_RBRACK, "]");
                    expect(TipoToken::OP_ASSIGN, "=");
                    parseExpression();
                    expect(TipoToken::SEP_SEMI, ";");
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }

    void parseExpression() {
        parseAndExp();
    }

    void parseAndExp() {
        parseCompExp();
        while (match(TipoToken::OP_AND)) {
            parseCompExp();
        }
    }

    void parseCompExp() {
        parseAddExp();
        if (current().tipo == TipoToken::OP_GT || current().tipo == TipoToken::OP_LT) {
            avancar();
            parseAddExp();
        }
    }

    void parseAddExp() {
        parseMulExp();
        while (current().tipo == TipoToken::OP_PLUS || current().tipo == TipoToken::OP_MINUS) {
            avancar();
            parseMulExp();
        }
    }

    void parseMulExp() {
        parseUnaryExp();
        while (current().tipo == TipoToken::OP_TIMES) {
            avancar();
            parseUnaryExp();
        }
    }

    void parseUnaryExp() {
        if (match(TipoToken::OP_NOT)) {
            parseUnaryExp();
        } else {
            parsePrimaryExp();
        }
    }

    void parsePrimaryExp() {
        if (current().tipo == TipoToken::ID) {
            avancar();
        } else if (current().tipo == TipoToken::NUMBER) {
            avancar();
        } else if (match(TipoToken::KW_TRUE) || match(TipoToken::KW_FALSE)) {
        } else if (match(TipoToken::KW_THIS)) {
        } else if (match(TipoToken::SEP_LPAREN)) {
            parseExpression();
            expect(TipoToken::SEP_RPAREN, ")");
        } else if (match(TipoToken::KW_NEW)) {
            if (match(TipoToken::KW_INT)) {
                expect(TipoToken::SEP_LBRACK, "[");
                parseExpression();
                expect(TipoToken::SEP_RBRACK, "]");
            } else if (current().tipo == TipoToken::ID) {
                avancar();
                expect(TipoToken::SEP_LPAREN, "(");
                expect(TipoToken::SEP_RPAREN, ")");
            }
        } else {
            return; // token inesperado, não tenta postfix
        }
        parsePostfixExp();
    }

    void parsePostfixExp() {
        while (true) {
            if (match(TipoToken::SEP_LBRACK)) {
                parseExpression();
                expect(TipoToken::SEP_RBRACK, "]");
            } else if (match(TipoToken::SEP_DOT)) {
                if (match(TipoToken::KW_LENGTH)) {
                } else if (current().tipo == TipoToken::ID) {
                    avancar();
                    if (match(TipoToken::SEP_LPAREN)) {
                        parseListExp();
                        expect(TipoToken::SEP_RPAREN, ")");
                    }
                } else {
                    break;
                }
            } else {
                break;
            }
        }
    }

    void parseListExp() {
        if (current().tipo == TipoToken::SEP_RPAREN) {
            return;
        }
        do {
            parseExpression();
        } while (match(TipoToken::SEP_COMMA));
    }

public:
    std::vector<std::string> erros;

    Parser(const std::vector<Token>& toks) : tokens(toks), pos(0), currentClass(""), currentMethod("") {}

    bool parse() {
        if (tokens.empty() || tokens.back().tipo != TipoToken::TOKEN_EOF) {
            erros.push_back("Arquivo sem fim (EOF)");
            return false;
        }

        expect(TipoToken::KW_CLASS, "class");
        if (expect(TipoToken::ID, "identificador")) {
            currentClass = tokens[pos-1].valor;
            int line = tokens[pos-1].linha;
            symbolTable.addSymbol(currentClass, "CLASS", "CLASS", "GLOBAL", line);
        }
        expect(TipoToken::SEP_LBRACE, "{");
        expect(TipoToken::KW_PUBLIC, "public");
        expect(TipoToken::KW_STATIC, "static");
        expect(TipoToken::KW_VOID, "void");
        expect(TipoToken::KW_MAIN, "main");
        expect(TipoToken::SEP_LPAREN, "(");
        expect(TipoToken::KW_STRING, "String");
        expect(TipoToken::SEP_LBRACK, "[");
        expect(TipoToken::SEP_RBRACK, "]");
        expect(TipoToken::ID, "identificador");
        expect(TipoToken::SEP_RPAREN, ")");
        expect(TipoToken::SEP_LBRACE, "{");

        currentMethod = "main";
        parseVarDecl();
        parseCommand();
        currentMethod = "";

        expect(TipoToken::SEP_RBRACE, "}");
        expect(TipoToken::SEP_RBRACE, "}");

        parseClassDecl();

        if (current().tipo != TipoToken::TOKEN_EOF) {
            erros.push_back("L" + std::to_string(current().linha) + ":C" +
                std::to_string(current().coluna) + " código após final da classe");
        }

        return erros.empty();
    }

    void parseClassDecl() {
        while (match(TipoToken::KW_CLASS)) {
            if (expect(TipoToken::ID, "identificador")) {
                currentClass = tokens[pos-1].valor;
                int line = tokens[pos-1].linha;
                symbolTable.addSymbol(currentClass, "CLASS", "CLASS", "GLOBAL", line);
            }
            if (match(TipoToken::KW_EXTENDS)) {
                expect(TipoToken::ID, "identificador");
            }
            expect(TipoToken::SEP_LBRACE, "{");
            parseVarDecl();
            parseMethodDecl();
            expect(TipoToken::SEP_RBRACE, "}");
        }
    }

    SymbolTable& getSymbolTable() {
        return symbolTable;
    }
};
