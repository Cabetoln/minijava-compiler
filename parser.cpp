
class Parser {
private:
    std::vector<Token> tokens;
    size_t pos;
    int prevLinha = 1, prevColuna = 1;
    SymbolTable symbolTable;
    Program programa;
    std::string currentClass;
    std::string currentMethod;

    Token current() {
        return pos < tokens.size() ? tokens[pos] : tokens.back();
    }

    Token peek(int offset = 1) {
        return pos + offset < tokens.size() ? tokens[pos + offset] : tokens.back();
    }

    void avancar() {
        if (pos < tokens.size()) {
            prevLinha = tokens[pos].linha;
            prevColuna = tokens[pos].coluna;
            pos++;
        }
    }

    bool match(TipoToken tipo) {
        if (current().tipo == tipo) {
            avancar();
            return true;
        }
        return false;
    }

    // Separadores de fim de instrução: quando faltam, o erro pertence à linha anterior
    bool isSufixSeparator(TipoToken tipo) {
        return tipo == TipoToken::SEP_SEMI;
    }

    bool expect(TipoToken tipo, const std::string& msg) {
        if (current().tipo != tipo) {
            int linha = isSufixSeparator(tipo) ? prevLinha : current().linha;
            int coluna = isSufixSeparator(tipo) ? prevColuna : current().coluna;
            erros.push_back("L" + std::to_string(linha) + ":C" +
                std::to_string(coluna) + " esperado " + msg);
            return false;
        }
        avancar();
        return true;
    }

    std::string scopeAtual() {
        return currentMethod.empty()
            ? currentClass + ":GLOBAL"
            : currentClass + "." + currentMethod;
    }

    // ── Tipos e argumentos ──────────────────────────
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

    std::vector<Param> parseArgs() {
        std::vector<Param> args;
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

    // Def_V → Type Id ; Def_V | λ
    // ID só é declaração quando o próximo token também é ID (ex.: "T left ;").
    std::vector<VarDecl> parseVarDecl() {
        std::vector<VarDecl> vars;
        while (true) {
            if (current().tipo != TipoToken::KW_INT &&
                current().tipo != TipoToken::KW_BOOLEAN &&
                !(current().tipo == TipoToken::ID && peek().tipo == TipoToken::ID))
                break;

            std::string type = parseType();
            if (current().tipo == TipoToken::ID) {
                std::string varName = current().valor;
                int line = current().linha;
                avancar();
                symbolTable.addSymbol(varName, type, "VARIABLE", scopeAtual(), line);
                expect(TipoToken::SEP_SEMI, ";");
                vars.push_back({type, varName, line});
            } else {
                erros.push_back("L" + std::to_string(current().linha) + ":C" +
                    std::to_string(current().coluna) + " esperado identificador na declaração de variável");
                break;
            }
        }
        return vars;
    }

    // ── Métodos ─────────────────────────────────────
    std::vector<MethodDecl> parseMethodDecl() {
        std::vector<MethodDecl> metodos;
        while (current().tipo == TipoToken::KW_PUBLIC) {
            avancar();
            MethodDecl m;
            m.tipoRetorno = parseType();
            if (current().tipo == TipoToken::ID) {
                m.nome = current().valor;
                m.linha = current().linha;
                currentMethod = m.nome;
                avancar();

                symbolTable.addSymbol(m.nome, m.tipoRetorno, "METHOD", currentClass, m.linha);

                expect(TipoToken::SEP_LPAREN, "(");
                m.params = parseArgs();
                for (const auto& arg : m.params) {
                    symbolTable.addSymbol(arg.nome, arg.tipo, "PARAMETER",
                                          currentClass + "." + m.nome, m.linha);
                }
                expect(TipoToken::SEP_RPAREN, ")");
                expect(TipoToken::SEP_LBRACE, "{");

                m.vars = parseVarDecl();
                m.comandos = parseStmtList();

                expect(TipoToken::KW_RETURN, "return");
                m.retorno = parseExpression();
                expect(TipoToken::SEP_SEMI, ";");
                expect(TipoToken::SEP_RBRACE, "}");

                currentMethod = "";
                metodos.push_back(std::move(m));
            } else {
                erros.push_back("L" + std::to_string(current().linha) + ":C" +
                    std::to_string(current().coluna) + " esperado identificador no método");
                break;
            }
        }
        return metodos;
    }

    // ── Comandos ────────────────────────────────────
    bool inicioDeComando(TipoToken t) {
        return t == TipoToken::ID || t == TipoToken::KW_IF ||
               t == TipoToken::KW_WHILE || t == TipoToken::KW_SYSTEM;
    }

    // L_com → Com L'_com  (aqui aceitamos zero-ou-mais por robustez)
    std::vector<StmtPtr> parseStmtList() {
        std::vector<StmtPtr> cmds;
        while (inicioDeComando(current().tipo)) {
            StmtPtr s = parseStmt();
            if (!s) break;
            cmds.push_back(std::move(s));
        }
        return cmds;
    }

    // Bloco obrigatoriamente entre chaves: { L_com }
    std::vector<StmtPtr> parseBlocoChaves() {
        expect(TipoToken::SEP_LBRACE, "{");
        auto cmds = parseStmtList();
        expect(TipoToken::SEP_RBRACE, "}");
        return cmds;
    }

    StmtPtr parseStmt() {
        Token t = current();

        // if ( Exp ) { L_com } (else { L_com })?
        if (t.tipo == TipoToken::KW_IF) {
            avancar();
            auto s = std::make_unique<IfStmt>();
            s->linha = t.linha; s->coluna = t.coluna;
            expect(TipoToken::SEP_LPAREN, "(");
            s->cond = parseExpression();
            expect(TipoToken::SEP_RPAREN, ")");
            s->entao = parseBlocoChaves();
            if (match(TipoToken::KW_ELSE)) {
                s->temSenao = true;
                s->senao = parseBlocoChaves();
            }
            return s;
        }

        // while ( Exp ) { L_com }
        if (t.tipo == TipoToken::KW_WHILE) {
            avancar();
            auto s = std::make_unique<WhileStmt>();
            s->linha = t.linha; s->coluna = t.coluna;
            expect(TipoToken::SEP_LPAREN, "(");
            s->cond = parseExpression();
            expect(TipoToken::SEP_RPAREN, ")");
            s->corpo = parseBlocoChaves();
            return s;
        }

        // System . out . println ( Exp ) ;
        if (t.tipo == TipoToken::KW_SYSTEM) {
            avancar();
            auto s = std::make_unique<PrintStmt>();
            s->linha = t.linha; s->coluna = t.coluna;
            expect(TipoToken::SEP_DOT, ".");
            expect(TipoToken::KW_OUT, "out");
            expect(TipoToken::SEP_DOT, ".");
            expect(TipoToken::KW_PRINTLN, "println");
            expect(TipoToken::SEP_LPAREN, "(");
            s->valor = parseExpression();
            expect(TipoToken::SEP_RPAREN, ")");
            expect(TipoToken::SEP_SEMI, ";");
            return s;
        }

        // Id Com_Ass  →  Id = Exp ;  |  Id [ Exp ] = Exp ;
        if (t.tipo == TipoToken::ID) {
            std::string id = t.valor;
            avancar();
            if (match(TipoToken::OP_ASSIGN)) {
                auto s = std::make_unique<AssignStmt>();
                s->linha = t.linha; s->coluna = t.coluna;
                s->nome = id;
                s->valor = parseExpression();
                expect(TipoToken::SEP_SEMI, ";");
                return s;
            } else if (match(TipoToken::SEP_LBRACK)) {
                auto s = std::make_unique<ArrayAssignStmt>();
                s->linha = t.linha; s->coluna = t.coluna;
                s->nome = id;
                s->indice = parseExpression();
                expect(TipoToken::SEP_RBRACK, "]");
                expect(TipoToken::OP_ASSIGN, "=");
                s->valor = parseExpression();
                expect(TipoToken::SEP_SEMI, ";");
                return s;
            } else {
                erros.push_back("L" + std::to_string(t.linha) + ":C" +
                    std::to_string(t.coluna) + " esperado '=' ou '[' após identificador '" + id + "'");
                return nullptr;
            }
        }

        return nullptr;
    }

    // ── Expressões (com precedência) ────────────────
    ExpPtr parseExpression() { return parseAndExp(); }

    // And_exp → Rel_exp (&& Rel_exp)*
    ExpPtr parseAndExp() {
        ExpPtr esq = parseRelExp();
        while (current().tipo == TipoToken::OP_AND) {
            Token op = current(); avancar();
            auto bin = std::make_unique<BinExp>();
            bin->op = "&&"; bin->linha = op.linha; bin->coluna = op.coluna;
            bin->esq = std::move(esq);
            bin->dir = parseRelExp();
            esq = std::move(bin);
        }
        return esq;
    }

    // Rel_exp → Add_exp (< Add_exp)*   (gramática só tem '<')
    ExpPtr parseRelExp() {
        ExpPtr esq = parseAddExp();
        while (current().tipo == TipoToken::OP_LT) {
            Token op = current(); avancar();
            auto bin = std::make_unique<BinExp>();
            bin->op = "<"; bin->linha = op.linha; bin->coluna = op.coluna;
            bin->esq = std::move(esq);
            bin->dir = parseAddExp();
            esq = std::move(bin);
        }
        return esq;
    }

    // Add_exp → Mul_exp ((+|-) Mul_exp)*
    ExpPtr parseAddExp() {
        ExpPtr esq = parseMulExp();
        while (current().tipo == TipoToken::OP_PLUS || current().tipo == TipoToken::OP_MINUS) {
            Token op = current(); avancar();
            auto bin = std::make_unique<BinExp>();
            bin->op = (op.tipo == TipoToken::OP_PLUS) ? "+" : "-";
            bin->linha = op.linha; bin->coluna = op.coluna;
            bin->esq = std::move(esq);
            bin->dir = parseMulExp();
            esq = std::move(bin);
        }
        return esq;
    }

    // Mul_exp → Un_exp (* Un_exp)*
    ExpPtr parseMulExp() {
        ExpPtr esq = parseUnaryExp();
        while (current().tipo == TipoToken::OP_TIMES) {
            Token op = current(); avancar();
            auto bin = std::make_unique<BinExp>();
            bin->op = "*"; bin->linha = op.linha; bin->coluna = op.coluna;
            bin->esq = std::move(esq);
            bin->dir = parseUnaryExp();
            esq = std::move(bin);
        }
        return esq;
    }

    // Un_exp → ! Un_exp | Psf_exp
    ExpPtr parseUnaryExp() {
        if (current().tipo == TipoToken::OP_NOT) {
            Token op = current(); avancar();
            auto n = std::make_unique<NotExp>();
            n->linha = op.linha; n->coluna = op.coluna;
            n->operando = parseUnaryExp();
            return n;
        }
        return parsePostfixExp();
    }

    // Psf_exp → Pri_exp Psf'_exp
    // Psf'_exp → [ Exp ] | . length | . Id ( L_exp ) , repetido
    ExpPtr parsePostfixExp() {
        ExpPtr base = parsePrimaryExp();
        while (true) {
            if (current().tipo == TipoToken::SEP_LBRACK) {
                avancar();
                auto a = std::make_unique<ArrayAccessExp>();
                a->arranjo = std::move(base);
                a->indice = parseExpression();
                expect(TipoToken::SEP_RBRACK, "]");
                base = std::move(a);
            } else if (current().tipo == TipoToken::SEP_DOT) {
                avancar();
                if (match(TipoToken::KW_LENGTH)) {
                    auto l = std::make_unique<LengthExp>();
                    l->alvo = std::move(base);
                    base = std::move(l);
                } else if (current().tipo == TipoToken::ID) {
                    auto c = std::make_unique<CallExp>();
                    c->metodo = current().valor;
                    c->linha = current().linha; c->coluna = current().coluna;
                    c->alvo = std::move(base);
                    avancar();
                    expect(TipoToken::SEP_LPAREN, "(");
                    c->args = parseListExp();
                    expect(TipoToken::SEP_RPAREN, ")");
                    base = std::move(c);
                } else {
                    erros.push_back("L" + std::to_string(current().linha) + ":C" +
                        std::to_string(current().coluna) + " esperado 'length' ou identificador após '.'");
                    break;
                }
            } else {
                break;
            }
        }
        return base;
    }

    // Pri_exp → ( Exp ) | true | false | Id | Number | this | new Id ( ) | new int [ Exp ]
    ExpPtr parsePrimaryExp() {
        Token t = current();

        if (match(TipoToken::SEP_LPAREN)) {
            ExpPtr e = parseExpression();
            expect(TipoToken::SEP_RPAREN, ")");
            return e;   // parênteses só agrupam
        }
        if (match(TipoToken::KW_TRUE) || match(TipoToken::KW_FALSE)) {
            auto b = std::make_unique<BoolExp>();
            b->valor = (t.tipo == TipoToken::KW_TRUE);
            b->linha = t.linha; b->coluna = t.coluna;
            return b;
        }
        if (t.tipo == TipoToken::ID) {
            avancar();
            auto id = std::make_unique<IdExp>();
            id->nome = t.valor; id->linha = t.linha; id->coluna = t.coluna;
            return id;
        }
        if (t.tipo == TipoToken::NUMBER) {
            avancar();
            auto num = std::make_unique<NumberExp>();
            num->valor = t.valor; num->linha = t.linha; num->coluna = t.coluna;
            return num;
        }
        if (match(TipoToken::KW_THIS)) {
            auto th = std::make_unique<ThisExp>();
            th->linha = t.linha; th->coluna = t.coluna;
            return th;
        }
        if (match(TipoToken::KW_NEW)) {
            if (match(TipoToken::KW_INT)) {
                expect(TipoToken::SEP_LBRACK, "[");
                auto na = std::make_unique<NewArrayExp>();
                na->linha = t.linha; na->coluna = t.coluna;
                na->tamanho = parseExpression();
                expect(TipoToken::SEP_RBRACK, "]");
                return na;
            } else if (current().tipo == TipoToken::ID) {
                auto no = std::make_unique<NewObjectExp>();
                no->classe = current().valor;
                no->linha = t.linha; no->coluna = t.coluna;
                avancar();
                expect(TipoToken::SEP_LPAREN, "(");
                expect(TipoToken::SEP_RPAREN, ")");
                return no;
            }
            erros.push_back("L" + std::to_string(current().linha) + ":C" +
                std::to_string(current().coluna) + " esperado 'int' ou identificador de classe após 'new'");
            return nullptr;
        }

        erros.push_back("L" + std::to_string(t.linha) + ":C" +
            std::to_string(t.coluna) + " expressão inválida");
        return nullptr;
    }

    // L_exp → Exp (, Exp)* | λ
    std::vector<ExpPtr> parseListExp() {
        std::vector<ExpPtr> args;
        if (current().tipo == TipoToken::SEP_RPAREN) return args;
        args.push_back(parseExpression());
        while (match(TipoToken::SEP_COMMA))
            args.push_back(parseExpression());
        return args;
    }

public:
    std::vector<std::string> erros;

    Parser(const std::vector<Token>& toks)
        : tokens(toks), pos(0), currentClass(""), currentMethod("") {}

    bool parse() {
        if (tokens.empty() || tokens.back().tipo != TipoToken::TOKEN_EOF) {
            erros.push_back("Arquivo sem fim (EOF)");
            return false;
        }

        parseMainClass();
        programa.classes = parseClassDecl();

        if (current().tipo != TipoToken::TOKEN_EOF) {
            erros.push_back("L" + std::to_string(current().linha) + ":C" +
                std::to_string(current().coluna) + " código após final das classes");
        }

        return erros.empty();
    }

    // Main_C → class Id { public static void main ( String [ ] Id ) { L_com } }
    void parseMainClass() {
        expect(TipoToken::KW_CLASS, "class");
        if (expect(TipoToken::ID, "identificador")) {
            programa.principal.nome = tokens[pos-1].valor;
            programa.principal.linha = tokens[pos-1].linha;
            currentClass = programa.principal.nome;
            symbolTable.addSymbol(currentClass, "CLASS", "CLASS", "GLOBAL", tokens[pos-1].linha);
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
        if (expect(TipoToken::ID, "identificador"))
            programa.principal.paramMain = tokens[pos-1].valor;
        expect(TipoToken::SEP_RPAREN, ")");
        expect(TipoToken::SEP_LBRACE, "{");

        currentMethod = "main";
        programa.principal.comandos = parseStmtList();
        currentMethod = "";

        expect(TipoToken::SEP_RBRACE, "}");   // fecha main
        expect(TipoToken::SEP_RBRACE, "}");   // fecha classe principal
    }

    // Def_C → class Id Def'_C | λ
    std::vector<ClassDecl> parseClassDecl() {
        std::vector<ClassDecl> classes;
        while (match(TipoToken::KW_CLASS)) {
            ClassDecl cd;
            if (expect(TipoToken::ID, "identificador")) {
                cd.nome = tokens[pos-1].valor;
                cd.linha = tokens[pos-1].linha;
                currentClass = cd.nome;
                symbolTable.addSymbol(cd.nome, "CLASS", "CLASS", "GLOBAL", cd.linha);
            }
            if (match(TipoToken::KW_EXTENDS)) {
                if (expect(TipoToken::ID, "identificador")) {
                    cd.pai = tokens[pos-1].valor;
                    cd.temPai = true;
                }
            }
            expect(TipoToken::SEP_LBRACE, "{");
            cd.vars = parseVarDecl();
            cd.metodos = parseMethodDecl();
            expect(TipoToken::SEP_RBRACE, "}");
            classes.push_back(std::move(cd));
        }
        return classes;
    }

    SymbolTable& getSymbolTable() { return symbolTable; }
    const Program& getPrograma() const { return programa; }
};
