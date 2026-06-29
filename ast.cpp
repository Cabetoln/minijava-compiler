#include <string>
#include <vector>
#include <memory>
#include <iostream>

// Nos da arvore sintatica abstrata. Cada no guarda linha/coluna para
// as mensagens de erro da fase semantica.

static std::string esp(int n) { return std::string(n * 2, ' '); }

// Expressoes
struct Exp {
    int linha = 0, coluna = 0;
    std::string tipo;
    virtual ~Exp() = default;
    virtual void print(std::ostream& os, int n) const = 0;
};
using ExpPtr = std::unique_ptr<Exp>;

static void printExp(std::ostream& os, int n, const ExpPtr& e) {
    if (!e) { os << esp(n) << "<erro>\n"; return; }
    e->print(os, n);
}

struct BinExp : Exp {                // && < + - *
    std::string op;
    ExpPtr esq, dir;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "Bin '" << op << "'\n";
        printExp(os, n + 1, esq);
        printExp(os, n + 1, dir);
    }
};

struct NotExp : Exp {
    ExpPtr operando;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "Not '!'\n";
        printExp(os, n + 1, operando);
    }
};

struct ArrayAccessExp : Exp {        // arranjo[indice]
    ExpPtr arranjo, indice;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "ArrayAccess\n";
        os << esp(n + 1) << "array:\n";  printExp(os, n + 2, arranjo);
        os << esp(n + 1) << "index:\n";  printExp(os, n + 2, indice);
    }
};

struct LengthExp : Exp {             // alvo.length
    ExpPtr alvo;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "Length\n";
        printExp(os, n + 1, alvo);
    }
};

struct CallExp : Exp {               // alvo.metodo(args)
    ExpPtr alvo;
    std::string metodo;
    std::vector<ExpPtr> args;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "Call '" << metodo << "'\n";
        os << esp(n + 1) << "target:\n"; printExp(os, n + 2, alvo);
        if (!args.empty()) {
            os << esp(n + 1) << "args:\n";
            for (const auto& a : args) printExp(os, n + 2, a);
        }
    }
};

struct IdExp : Exp {
    std::string nome;
    void print(std::ostream& os, int n) const override { os << esp(n) << "Id '" << nome << "'\n"; }
};

struct NumberExp : Exp {
    std::string valor;
    void print(std::ostream& os, int n) const override { os << esp(n) << "Number " << valor << "\n"; }
};

struct BoolExp : Exp {
    bool valor = false;
    void print(std::ostream& os, int n) const override { os << esp(n) << "Bool " << (valor ? "true" : "false") << "\n"; }
};

struct ThisExp : Exp {
    void print(std::ostream& os, int n) const override { os << esp(n) << "This\n"; }
};

struct NewObjectExp : Exp {          // new Classe()
    std::string classe;
    void print(std::ostream& os, int n) const override { os << esp(n) << "NewObject '" << classe << "'\n"; }
};

struct NewArrayExp : Exp {           // new int[tamanho]
    ExpPtr tamanho;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "NewArray int[]\n";
        printExp(os, n + 1, tamanho);
    }
};

// Comandos
struct Stmt {
    int linha = 0, coluna = 0;
    virtual ~Stmt() = default;
    virtual void print(std::ostream& os, int n) const = 0;
};
using StmtPtr = std::unique_ptr<Stmt>;

static void printStmtList(std::ostream& os, int n, const std::vector<StmtPtr>& cs) {
    if (cs.empty()) { os << esp(n) << "(vazio)\n"; return; }
    for (const auto& c : cs) c->print(os, n);
}

struct AssignStmt : Stmt {            // nome = valor;
    std::string nome;
    ExpPtr valor;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "Assign '" << nome << "'\n";
        printExp(os, n + 1, valor);
    }
};

struct ArrayAssignStmt : Stmt {       // nome[indice] = valor;
    std::string nome;
    ExpPtr indice, valor;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "ArrayAssign '" << nome << "'\n";
        os << esp(n + 1) << "index:\n"; printExp(os, n + 2, indice);
        os << esp(n + 1) << "value:\n"; printExp(os, n + 2, valor);
    }
};

struct IfStmt : Stmt {
    ExpPtr cond;
    std::vector<StmtPtr> entao, senao;
    bool temSenao = false;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "If\n";
        os << esp(n + 1) << "cond:\n"; printExp(os, n + 2, cond);
        os << esp(n + 1) << "then:\n"; printStmtList(os, n + 2, entao);
        if (temSenao) { os << esp(n + 1) << "else:\n"; printStmtList(os, n + 2, senao); }
    }
};

struct WhileStmt : Stmt {
    ExpPtr cond;
    std::vector<StmtPtr> corpo;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "While\n";
        os << esp(n + 1) << "cond:\n"; printExp(os, n + 2, cond);
        os << esp(n + 1) << "body:\n"; printStmtList(os, n + 2, corpo);
    }
};

struct PrintStmt : Stmt {             // System.out.println(valor);
    ExpPtr valor;
    void print(std::ostream& os, int n) const override {
        os << esp(n) << "Println\n";
        printExp(os, n + 1, valor);
    }
};

// Declaracoes
struct VarDecl  { std::string tipo, nome; int linha = 0; };
struct Param    { std::string tipo, nome; };

struct MethodDecl {
    std::string tipoRetorno, nome;
    int linha = 0;
    std::vector<Param>   params;
    std::vector<VarDecl> vars;
    std::vector<StmtPtr> comandos;
    ExpPtr retorno;
};

struct ClassDecl {
    std::string nome, pai;
    bool temPai = false;
    int linha = 0;
    std::vector<VarDecl>   vars;
    std::vector<MethodDecl> metodos;
};

struct MainClass {
    std::string nome, paramMain;
    int linha = 0;
    std::vector<StmtPtr> comandos;
};

struct Program {
    MainClass principal;
    std::vector<ClassDecl> classes;
};

// Impressao da arvore
inline void printPrograma(std::ostream& os, const Program& p) {
    os << "Program\n";
    os << esp(1) << "MainClass '" << p.principal.nome
       << "' (args: " << p.principal.paramMain << ")\n";
    os << esp(2) << "body:\n";
    printStmtList(os, 3, p.principal.comandos);

    for (const auto& c : p.classes) {
        os << esp(1) << "Class '" << c.nome << "'";
        if (c.temPai) os << " extends '" << c.pai << "'";
        os << "\n";

        if (!c.vars.empty()) {
            os << esp(2) << "fields:\n";
            for (const auto& v : c.vars) os << esp(3) << v.tipo << " " << v.nome << "\n";
        }
        for (const auto& m : c.metodos) {
            os << esp(2) << "Method '" << m.nome << "' -> " << m.tipoRetorno << "\n";
            if (!m.params.empty()) {
                os << esp(3) << "params:\n";
                for (const auto& pa : m.params) os << esp(4) << pa.tipo << " " << pa.nome << "\n";
            }
            if (!m.vars.empty()) {
                os << esp(3) << "locals:\n";
                for (const auto& v : m.vars) os << esp(4) << v.tipo << " " << v.nome << "\n";
            }
            os << esp(3) << "body:\n";
            printStmtList(os, 4, m.comandos);
            os << esp(3) << "return:\n";
            printExp(os, 4, m.retorno);
        }
    }
}
