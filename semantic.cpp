#include <string>
#include <vector>
#include <map>
#include <set>
#include <iostream>

// ─────────────────────────────────────────────
//  ANÁLISE SEMÂNTICA
//
//  Percorre a AST em duas passagens:
//    1. Coleta classes, campos e assinaturas de métodos (ambiente global).
//    2. Verifica tipos, resolução de nomes, herança e compatibilidades.
// ─────────────────────────────────────────────

static const std::string ERRO_TIPO = "error";

struct MethodSig {
    std::string retorno;
    std::vector<std::string> tiposParams;
    std::vector<std::string> nomesParams;
    int linha = 0;
};

struct ClassInfo {
    std::string nome, pai;
    bool temPai = false;
    int linha = 0;
    bool ehMain = false;
    std::map<std::string, std::string> campos;     // nome -> tipo
    std::map<std::string, MethodSig>   metodos;     // nome -> assinatura
};

class SemanticAnalyzer {
    const Program& prog;
    std::map<std::string, ClassInfo> classes;

    // contexto da passagem 2
    std::string classeAtual;
    std::string retornoAtual;
    std::map<std::string, std::string> escopoMetodo;   // params + locais

    void erroLC(int l, int c, const std::string& msg) {
        erros.push_back("L" + std::to_string(l) + ":C" + std::to_string(c) + " " + msg);
    }
    void erro(const Exp* e, const std::string& msg)  { erroLC(e->linha, e->coluna, msg); }
    void erro(const Stmt* s, const std::string& msg) { erroLC(s->linha, s->coluna, msg); }

    bool ok(const std::string& t) { return !t.empty() && t != ERRO_TIPO; }

    bool classeExiste(const std::string& t) { return classes.count(t) > 0; }

    bool tipoValido(const std::string& t) {
        return t == "int" || t == "boolean" || t == "int[]" || classeExiste(t);
    }

    // origem é subclasse (ou igual) de base?
    bool ehSubclasse(const std::string& origem, const std::string& base) {
        std::string c = origem;
        int guard = 0;
        while (!c.empty() && guard++ < 10000) {
            if (c == base) return true;
            auto it = classes.find(c);
            if (it == classes.end() || !it->second.temPai) break;
            c = it->second.pai;
        }
        return false;
    }

    // Compatibilidade de atribuição/argumento.
    bool atribuivel(const std::string& destino, const std::string& origem) {
        if (!ok(destino) || !ok(origem)) return true;   // suprime cascata de erros
        if (destino == origem) return true;
        if (classeExiste(destino) && classeExiste(origem) && ehSubclasse(origem, destino))
            return true;
        return false;
    }

    bool buscarCampo(const std::string& classe, const std::string& nome, std::string& tipoOut) {
        std::string c = classe;
        int guard = 0;
        while (!c.empty() && guard++ < 10000) {
            auto it = classes.find(c);
            if (it == classes.end()) break;
            auto f = it->second.campos.find(nome);
            if (f != it->second.campos.end()) { tipoOut = f->second; return true; }
            if (!it->second.temPai) break;
            c = it->second.pai;
        }
        return false;
    }

    bool buscarMetodo(const std::string& classe, const std::string& nome, MethodSig& sigOut) {
        std::string c = classe;
        int guard = 0;
        while (!c.empty() && guard++ < 10000) {
            auto it = classes.find(c);
            if (it == classes.end()) break;
            auto m = it->second.metodos.find(nome);
            if (m != it->second.metodos.end()) { sigOut = m->second; return true; }
            if (!it->second.temPai) break;
            c = it->second.pai;
        }
        return false;
    }

    // Resolução de nomes: parâmetros/locais → campos da classe → campos herdados.
    bool resolverVar(const std::string& nome, std::string& tipoOut) {
        auto it = escopoMetodo.find(nome);
        if (it != escopoMetodo.end()) { tipoOut = it->second; return true; }
        return buscarCampo(classeAtual, nome, tipoOut);
    }

    // ── PASSAGEM 1: coleta de declarações ───────────
    void coletarClasses() {
        // Classe principal (só contém main).
        {
            ClassInfo mc;
            mc.nome = prog.principal.nome;
            mc.linha = prog.principal.linha;
            mc.ehMain = true;
            classes[mc.nome] = mc;
        }

        for (const auto& c : prog.classes) {
            if (classes.count(c.nome)) {
                erroLC(c.linha, 1, "classe '" + c.nome + "' já declarada");
                continue;
            }
            ClassInfo ci;
            ci.nome = c.nome;
            ci.pai = c.pai;
            ci.temPai = c.temPai;
            ci.linha = c.linha;

            for (const auto& v : c.vars) {
                if (ci.campos.count(v.nome))
                    erroLC(v.linha, 1, "campo '" + v.nome + "' já declarado na classe '" + c.nome + "'");
                else
                    ci.campos[v.nome] = v.tipo;
            }
            for (const auto& m : c.metodos) {
                if (ci.metodos.count(m.nome)) {
                    erroLC(m.linha, 1, "método '" + m.nome + "' já declarado na classe '" + c.nome + "'");
                    continue;
                }
                MethodSig sig;
                sig.retorno = m.tipoRetorno;
                sig.linha = m.linha;
                for (const auto& p : m.params) {
                    sig.tiposParams.push_back(p.tipo);
                    sig.nomesParams.push_back(p.nome);
                }
                ci.metodos[m.nome] = sig;
            }

            // Classe vazia (sem campos e sem métodos) é erro semântico.
            if (c.vars.empty() && c.metodos.empty())
                erroLC(c.linha, 1, "classe '" + c.nome + "' é vazia (sem atributos ou métodos)");

            classes[c.nome] = ci;
        }
    }

    // Validações que dependem de todas as classes já coletadas.
    void validarHierarquia() {
        for (const auto& par : classes) {
            const ClassInfo& ci = par.second;
            if (ci.ehMain) continue;

            // extends de classe inexistente / ciclos
            if (ci.temPai) {
                if (!classeExiste(ci.pai)) {
                    erroLC(ci.linha, 1, "classe '" + ci.nome + "' estende '" + ci.pai + "' que não existe");
                } else {
                    // detecta ciclo
                    std::set<std::string> vistos;
                    std::string c = ci.nome;
                    while (!c.empty()) {
                        if (vistos.count(c)) {
                            erroLC(ci.linha, 1, "ciclo de herança envolvendo a classe '" + ci.nome + "'");
                            break;
                        }
                        vistos.insert(c);
                        auto it = classes.find(c);
                        if (it == classes.end() || !it->second.temPai) break;
                        c = it->second.pai;
                    }
                }
            }

            // tipos de campos válidos
            for (const auto& f : ci.campos)
                if (!tipoValido(f.second))
                    erroLC(ci.linha, 1, "tipo inválido '" + f.second + "' no campo '" + f.first + "'");

            // tipos de métodos válidos + consistência de sobrescrita
            for (const auto& mp : ci.metodos) {
                const MethodSig& sig = mp.second;
                if (!tipoValido(sig.retorno))
                    erroLC(sig.linha, 1, "tipo de retorno inválido '" + sig.retorno + "' no método '" + mp.first + "'");
                for (const auto& tp : sig.tiposParams)
                    if (!tipoValido(tp))
                        erroLC(sig.linha, 1, "tipo de parâmetro inválido '" + tp + "' no método '" + mp.first + "'");

                // sobrescrita: assinatura deve coincidir com a do pai
                if (ci.temPai) {
                    MethodSig herdado;
                    if (buscarMetodo(ci.pai, mp.first, herdado)) {
                        bool mesma = herdado.retorno == sig.retorno &&
                                     herdado.tiposParams == sig.tiposParams;
                        if (!mesma)
                            erroLC(sig.linha, 1, "sobrescrita incompatível do método '" + mp.first +
                                   "' herdado de '" + ci.pai + "'");
                    }
                }
            }
        }
    }

    // ── PASSAGEM 2: verificação de tipos ────────────
    void checarPrograma() {
        // main
        classeAtual = prog.principal.nome;
        retornoAtual = "";
        escopoMetodo.clear();
        for (const auto& s : prog.principal.comandos)
            checarStmt(s.get());

        // demais classes
        for (const auto& c : prog.classes) {
            if (!classeExiste(c.nome)) continue;
            classeAtual = c.nome;
            for (const auto& m : c.metodos)
                checarMetodo(c, m);
        }
    }

    void checarMetodo(const ClassDecl& c, const MethodDecl& m) {
        retornoAtual = m.tipoRetorno;
        escopoMetodo.clear();

        for (const auto& p : m.params) {
            if (escopoMetodo.count(p.nome))
                erroLC(m.linha, 1, "parâmetro '" + p.nome + "' duplicado no método '" + m.nome + "'");
            escopoMetodo[p.nome] = p.tipo;
        }
        for (const auto& v : m.vars) {
            if (escopoMetodo.count(v.nome))
                erroLC(v.linha, 1, "variável '" + v.nome + "' redeclarada no método '" + m.nome + "'");
            escopoMetodo[v.nome] = v.tipo;
        }

        for (const auto& s : m.comandos)
            checarStmt(s.get());

        // tipo do return compatível com o tipo declarado
        std::string tr = checarExp(m.retorno.get());
        if (ok(tr) && !atribuivel(m.tipoRetorno, tr))
            erroLC(m.linha, 1, "return de tipo '" + tr + "' incompatível com retorno '" +
                   m.tipoRetorno + "' do método '" + m.nome + "'");
    }

    // ── Comandos ────────────────────────────────────
    void checarStmt(const Stmt* s) {
        if (!s) return;

        if (auto a = dynamic_cast<const AssignStmt*>(s)) {
            std::string td;
            if (!resolverVar(a->nome, td)) {
                erro(a, "variável '" + a->nome + "' não declarada");
                checarExp(a->valor.get());
                return;
            }
            std::string tv = checarExp(a->valor.get());
            if (ok(tv) && !atribuivel(td, tv))
                erro(a, "atribuição incompatível: '" + td + "' = '" + tv + "'");
            return;
        }

        if (auto aa = dynamic_cast<const ArrayAssignStmt*>(s)) {
            std::string td;
            if (!resolverVar(aa->nome, td))
                erro(aa, "variável '" + aa->nome + "' não declarada");
            else if (td != "int[]")
                erro(aa, "indexação de '" + aa->nome + "' que não é int[] (é '" + td + "')");
            std::string ti = checarExp(aa->indice.get());
            if (ok(ti) && ti != "int")
                erro(aa, "índice de array deve ser int (é '" + ti + "')");
            std::string tv = checarExp(aa->valor.get());
            if (ok(tv) && tv != "int")
                erro(aa, "valor atribuído ao array deve ser int (é '" + tv + "')");
            return;
        }

        if (auto i = dynamic_cast<const IfStmt*>(s)) {
            std::string tc = checarExp(i->cond.get());
            if (ok(tc) && tc != "boolean")
                erro(i, "condição do if deve ser boolean (é '" + tc + "')");
            for (const auto& st : i->entao) checarStmt(st.get());
            for (const auto& st : i->senao) checarStmt(st.get());
            return;
        }

        if (auto w = dynamic_cast<const WhileStmt*>(s)) {
            std::string tc = checarExp(w->cond.get());
            if (ok(tc) && tc != "boolean")
                erro(w, "condição do while deve ser boolean (é '" + tc + "')");
            for (const auto& st : w->corpo) checarStmt(st.get());
            return;
        }

        if (auto p = dynamic_cast<const PrintStmt*>(s)) {
            std::string tv = checarExp(p->valor.get());
            if (ok(tv) && tv != "int")
                erro(p, "System.out.println espera int (recebeu '" + tv + "')");
            return;
        }
    }

    // ── Expressões: devolve o tipo (ou ERRO_TIPO) ───
    std::string checarExp(const Exp* e) {
        if (!e) return ERRO_TIPO;

        if (dynamic_cast<const NumberExp*>(e)) return "int";
        if (dynamic_cast<const BoolExp*>(e))   return "boolean";

        if (auto id = dynamic_cast<const IdExp*>(e)) {
            std::string t;
            if (!resolverVar(id->nome, t)) {
                erro(e, "variável '" + id->nome + "' não declarada");
                return ERRO_TIPO;
            }
            return t;
        }

        if (dynamic_cast<const ThisExp*>(e)) return classeAtual;

        if (auto no = dynamic_cast<const NewObjectExp*>(e)) {
            if (!classeExiste(no->classe)) {
                erro(e, "classe '" + no->classe + "' não declarada");
                return ERRO_TIPO;
            }
            return no->classe;
        }

        if (auto na = dynamic_cast<const NewArrayExp*>(e)) {
            std::string ts = checarExp(na->tamanho.get());
            if (ok(ts) && ts != "int")
                erro(e, "tamanho de 'new int[]' deve ser int (é '" + ts + "')");
            return "int[]";
        }

        if (auto n = dynamic_cast<const NotExp*>(e)) {
            std::string t = checarExp(n->operando.get());
            if (ok(t) && t != "boolean")
                erro(e, "operador '!' exige boolean (é '" + t + "')");
            return "boolean";
        }

        if (auto b = dynamic_cast<const BinExp*>(e)) {
            std::string te = checarExp(b->esq.get());
            std::string td = checarExp(b->dir.get());
            if (b->op == "&&") {
                if (ok(te) && te != "boolean") erro(e, "operando esquerdo de '&&' deve ser boolean (é '" + te + "')");
                if (ok(td) && td != "boolean") erro(e, "operando direito de '&&' deve ser boolean (é '" + td + "')");
                return "boolean";
            }
            if (b->op == "<") {
                if (ok(te) && te != "int") erro(e, "operando esquerdo de '<' deve ser int (é '" + te + "')");
                if (ok(td) && td != "int") erro(e, "operando direito de '<' deve ser int (é '" + td + "')");
                return "boolean";
            }
            // + - *
            if (ok(te) && te != "int") erro(e, "operando esquerdo de '" + b->op + "' deve ser int (é '" + te + "')");
            if (ok(td) && td != "int") erro(e, "operando direito de '" + b->op + "' deve ser int (é '" + td + "')");
            return "int";
        }

        if (auto a = dynamic_cast<const ArrayAccessExp*>(e)) {
            std::string ta = checarExp(a->arranjo.get());
            std::string ti = checarExp(a->indice.get());
            if (ok(ta) && ta != "int[]")
                erro(e, "indexação de tipo não-array '" + ta + "'");
            if (ok(ti) && ti != "int")
                erro(e, "índice deve ser int (é '" + ti + "')");
            return "int";
        }

        if (auto l = dynamic_cast<const LengthExp*>(e)) {
            std::string t = checarExp(l->alvo.get());
            if (ok(t) && t != "int[]")
                erro(e, "'.length' exige int[] (é '" + t + "')");
            return "int";
        }

        if (auto c = dynamic_cast<const CallExp*>(e)) {
            std::string talvo = checarExp(c->alvo.get());
            // verifica os argumentos sempre (pega erros aninhados)
            std::vector<std::string> targs;
            for (const auto& ar : c->args) targs.push_back(checarExp(ar.get()));

            if (!ok(talvo)) return ERRO_TIPO;
            if (!classeExiste(talvo)) {
                erro(e, "chamada de método em tipo não-classe '" + talvo + "'");
                return ERRO_TIPO;
            }
            MethodSig sig;
            if (!buscarMetodo(talvo, c->metodo, sig)) {
                erro(e, "método '" + c->metodo + "' não existe na classe '" + talvo + "'");
                return ERRO_TIPO;
            }
            if (targs.size() != sig.tiposParams.size()) {
                erro(e, "método '" + c->metodo + "' espera " +
                     std::to_string(sig.tiposParams.size()) + " argumento(s), recebeu " +
                     std::to_string(targs.size()));
            } else {
                for (size_t k = 0; k < targs.size(); k++)
                    if (ok(targs[k]) && !atribuivel(sig.tiposParams[k], targs[k]))
                        erro(e, "argumento " + std::to_string(k + 1) + " de '" + c->metodo +
                             "' deve ser '" + sig.tiposParams[k] + "' (é '" + targs[k] + "')");
            }
            return sig.retorno;
        }

        return ERRO_TIPO;
    }

public:
    std::vector<std::string> erros;

    SemanticAnalyzer(const Program& p) : prog(p) {}

    bool analyze() {
        coletarClasses();
        validarHierarquia();
        checarPrograma();
        return erros.empty();
    }
};
