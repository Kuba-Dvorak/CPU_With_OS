#pragma once
// Sada "calls": cele volani funkce pres zasobnik (callDiffFunq + getCalled + getReturnCMD + endFunq).
// Postavi se maly program (skok pres tela funkci, main s promennymi a volanimi), spusti se od radku 0
// a kontroluje se: dobehne, SP se vrati, vysledky sedi, kod pokracuje za volanim, nic se nepise mimo zasobnik.
// Kazdy scenar bezi pro nekolik pocatecnich SP (to, co by nastavil OS).
#include <functional>
#include <random>
#include "adapter.hpp"
#include "common.hpp"

namespace suite_calls {

using adapter::Place;

const int STACK_LIMIT = 0x800;  // vse, co program zapise, musi lezet v [SP0, SP0 + STACK_LIMIT)

struct Check {
    std::string what;
    int off, size;
    long long expected;
};

struct Slot {
    std::string name;
    int off, size;
};
struct FrameInfo {
    std::string fn;
    std::vector<Slot> slots;
};

struct Program {
    adapter::World w;
    fullCompiledCode code;
    adapter::Frame main;
    std::vector<Check> checks;
    std::vector<FrameInfo> frames;
    std::mt19937 rng{1};

    Program() {
        adapter::defineNumber(w, 2);
        adapter::defineStruct(w, "Pair", {{"number", "x", false}, {"number", "y", false}});
        code.addInstruction(" jump -1 ");  // preskoci tela funkci, opravi se v startMain()
    }

    // funkce s jednim radkem v tele: return <neco>
    adapter::Function &fn(std::string name, std::vector<std::string> pn, std::vector<std::string> pt, std::vector<bool> pr,
                          std::string out) {
        fns.emplace_back();
        adapter::Function &f = fns.back();
        adapter::defineFunction(f, name, pn, pt, pr, out, w, code);
        starts.push_back({name, adapter::startLine(f)});
        FrameInfo fi{name, {{"výstup", 0, adapter::typeSize(w, out)}, {"návrat", adapter::typeSize(w, out), 2}}};
        for (size_t i = 0; i < pn.size(); i++)
            fi.slots.push_back({pn[i] + (pr[i] ? " (ref)" : ""), adapter::paramOffset(w, pn[i]), pr[i] ? 2 : adapter::typeSize(w, pt[i])});
        frames.push_back(fi);
        return f;
    }

    void startMain() {
        starts.push_back({"main", code.currentLine});
        code.changeBackFunq(" jump " + std::to_string(code.currentLine) + " ", 0);
        adapter::openScope(main);
    }

    variable *var(std::string name, std::string type, long long init) {
        int size = adapter::typeSize(w, type);
        int off = adapter::allocVar(main, size, name);
        adapter::setLocal((int)init, off, size, code);
        return adapter::addVar(w, name, type, off, false);
    }

    // r = f(args); vysledek se zkopiruje z vystupu volani do promenne r
    void assign(variable *r, adapter::Function &f, std::vector<variable *> args) {
        std::vector<std::pair<variable *, std::vector<std::string>>> a;
        for (auto *v : args) a.push_back({v, {}});
        int out = adapter::callFunction(main, f, a, w, code);
        int size = adapter::typeSize(w, r->myTypee->name);
        adapter::copy({size, out, false, false, 1}, {size, r->localPointer0, false, false, 2}, code);
        main.endLine();
    }

    void expect(std::string what, variable *v, long long value) {
        checks.push_back({what, v->localPointer0, adapter::typeSize(w, v->myTypee->name), value});
    }

    // jen cast promenne (pole struktury): fieldOff bajtu od zacatku, size bajtu
    void expectAt(std::string what, variable *v, int fieldOff, int size, long long value) {
        checks.push_back({what, v->localPointer0 + fieldOff, size, value});
    }

    // lokalni promenna v tele funkce (za parametry)
    variable *local(adapter::Function &f, std::string name, std::string type) {
        int off = adapter::allocVar(adapter::bodyFrame(f), adapter::typeSize(w, type), name);
        return adapter::addVar(w, name, type, off, false);
    }

    // volani z libovolneho ramce; args = promenna + cesta (napr. {pp, {"y"}} = pp.y); vrati offset vystupu
    int call(adapter::Frame &from, adapter::Function &f, std::vector<std::pair<variable *, std::vector<std::string>>> args) {
        return adapter::callFunction(from, f, args, w, code);
    }

    // vystup volani jako pojmenovana promenna (tak, jak by to delal kompilator pro f(g(a)))
    variable *named(std::string name, std::string type, int off) { return adapter::addVar(w, name, type, off, false); }

    void finish() {
        int off = adapter::allocVar(main, 2, "marker");
        adapter::setLocal(0xBEEF, off, 2, code);
        checks.push_back({"kód za posledním voláním proběhl", off, 2, 0xBEEF});
    }

    std::deque<adapter::Function> fns;
    std::vector<std::pair<std::string, int>> starts;  // kde v kodu zacina ktera funkce
};

struct Scenario {
    std::string name, desc;
    std::function<void(Program &)> build;
};

inline Place ret(Program &p, std::string param, std::string type, bool isRef = false) {
    return {adapter::typeSize(p.w, type), adapter::paramOffset(p.w, param), isRef, false, 1};
}

// Quad {a, b, c, d}: 4 cisla, aby funkce mohla vratit vsechny sve parametry naraz (kontrola poradi)
inline void defineQuad(Program &p) {
    adapter::defineStruct(p.w, "Quad", {{"number", "a", false}, {"number", "b", false}, {"number", "c", false}, {"number", "d", false}});
}
inline long long quad(long long a, long long b, long long c, long long d) { return a | b << 16 | c << 32 | d << 48; }

// Quad t; t.a..t.d = parametry v poradi; return t
inline void returnQuadOf(Program &p, adapter::Function &f, std::string tName, std::vector<std::string> params) {
    auto *t = p.local(f, tName, "Quad");
    const char *keys[4] = {"a", "b", "c", "d"};
    for (int i = 0; i < 4; i++) adapter::writeVar(t, {keys[i]}, ret(p, params[i], "number"), p.code);
    adapter::returnFrom(f, {8, t->localPointer0, false, false, 1}, p.code);
}

inline std::vector<Scenario> scenarios() {
    return {
        {"identita", "f(x) = x;  r = f(a)", [](Program &p) {
             auto &f = p.fn("f", {"fx"}, {"number"}, {false}, "number");
             adapter::returnFrom(f, ret(p, "fx", "number"), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x1234);
             auto *r = p.var("r", "number", 0);
             p.assign(r, f, {a});
             p.expect("r = f(a)", r, 0x1234);
             p.expect("a se nezměnilo", a, 0x1234);
         }},
        {"druhý parametr", "f(x, y) = y;  r = f(a, b)", [](Program &p) {
             auto &f = p.fn("f2", {"f2x", "f2y"}, {"number", "number"}, {false, false}, "number");
             adapter::returnFrom(f, ret(p, "f2y", "number"), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x1111);
             auto *b = p.var("b", "number", 0x2222);
             auto *r = p.var("r", "number", 0);
             p.assign(r, f, {a, b});
             p.expect("r = f(a, b) = b", r, 0x2222);
         }},
        {"struktura", "f(q: Pair) = q;  r = f(p)", [](Program &p) {
             auto &f = p.fn("fp", {"fq"}, {"Pair"}, {false}, "Pair");
             adapter::returnFrom(f, ret(p, "fq", "Pair"), p.code);
             p.startMain();
             auto *a = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *r = p.var("r", "Pair", 0);
             p.assign(r, f, {a});
             p.expect("r = f(p), 4 B", r, 0x0A0B0C0D);
         }},
        {"reference", "f(ref x) = *x;  r = f(a)", [](Program &p) {
             auto &f = p.fn("fr", {"frx"}, {"number"}, {true}, "number");
             adapter::returnFrom(f, ret(p, "frx", "number", true), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x4321);
             auto *r = p.var("r", "number", 0);
             p.assign(r, f, {a});
             p.expect("r = f(&a) = a", r, 0x4321);
         }},
        {"dvě volání", "f(x) = x;  r1 = f(a);  r2 = f(b)", [](Program &p) {
             auto &f = p.fn("f", {"fx"}, {"number"}, {false}, "number");
             adapter::returnFrom(f, ret(p, "fx", "number"), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x00AA);
             auto *b = p.var("b", "number", 0x00BB);
             auto *r1 = p.var("r1", "number", 0);
             auto *r2 = p.var("r2", "number", 0);
             p.assign(r1, f, {a});
             p.assign(r2, f, {b});
             p.expect("r1 = f(a)", r1, 0x00AA);
             p.expect("r2 = f(b)", r2, 0x00BB);
         }},
        {"dvě různé funkce", "f(x) = x;  fp(q: Pair) = q;  r1 = f(a);  r2 = fp(p)", [](Program &p) {
             auto &f = p.fn("f", {"fx"}, {"number"}, {false}, "number");
             adapter::returnFrom(f, ret(p, "fx", "number"), p.code);
             auto &fp = p.fn("fp", {"fq"}, {"Pair"}, {false}, "Pair");
             adapter::returnFrom(fp, ret(p, "fq", "Pair"), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x0A0A);
             auto *pp = p.var("pp", "Pair", 0x01020304);
             auto *r1 = p.var("r1", "number", 0);
             auto *r2 = p.var("r2", "Pair", 0);
             p.assign(r1, f, {a});
             p.assign(r2, fp, {pp});
             p.expect("r1 = f(a)", r1, 0x0A0A);
             p.expect("r2 = fp(p), 4 B", r2, 0x01020304);
         }},
        {"vnořené volání", "f(x) = x;  g(y) = f(y);  r = g(a)", [](Program &p) {
             auto &f = p.fn("f", {"fx"}, {"number"}, {false}, "number");
             adapter::returnFrom(f, ret(p, "fx", "number"), p.code);
             auto &g = p.fn("g", {"gy"}, {"number"}, {false}, "number");
             int out = adapter::callFunction(adapter::bodyFrame(g), f, {{adapter::param(p.w, "gy"), {}}}, p.w, p.code);
             adapter::returnFrom(g, {2, out, false, false, 1}, p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x5A5A);
             auto *r = p.var("r", "number", 0);
             p.assign(r, g, {a});
             p.expect("r = g(a)", r, 0x5A5A);
         }},
        {"tři úrovně", "f(x) = x;  g(y) = f(y);  h(z) = g(z);  r = h(a)", [](Program &p) {
             auto &f = p.fn("f", {"fx"}, {"number"}, {false}, "number");
             adapter::returnFrom(f, ret(p, "fx", "number"), p.code);
             auto &g = p.fn("g", {"gy"}, {"number"}, {false}, "number");
             int og = adapter::callFunction(adapter::bodyFrame(g), f, {{adapter::param(p.w, "gy"), {}}}, p.w, p.code);
             adapter::returnFrom(g, {2, og, false, false, 1}, p.code);
             auto &h = p.fn("h", {"hz"}, {"number"}, {false}, "number");
             int oh = adapter::callFunction(adapter::bodyFrame(h), g, {{adapter::param(p.w, "hz"), {}}}, p.w, p.code);
             adapter::returnFrom(h, {2, oh, false, false, 1}, p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x7E57);
             auto *r = p.var("r", "number", 0);
             p.assign(r, h, {a});
             p.expect("r = h(a)", r, 0x7E57);
         }},
        // ------------------------------------------------ dalsi scenare: vic parametru, reference, lokalni promenne
        {"tři parametry různých velikostí", "fa(x, q: Pair, ref c) = x;  fc(x, q: Pair, ref c) = *c;  r1 = fa(pp.y, pp, c);  r2 = fc(a, pp, c)",
         [](Program &p) {
             auto &fa = p.fn("fa", {"fax", "faq", "fac"}, {"number", "Pair", "number"}, {false, false, true}, "number");
             adapter::returnFrom(fa, ret(p, "fax", "number"), p.code);
             auto &fc = p.fn("fc", {"fcx", "fcq", "fcc"}, {"number", "Pair", "number"}, {false, false, true}, "number");
             adapter::returnFrom(fc, ret(p, "fcc", "number", true), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x1A1A);
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *c = p.var("c", "number", 0x3C3C);
             auto *r1 = p.var("r1", "number", 0);
             auto *r2 = p.var("r2", "number", 0);
             int o1 = p.call(p.main, fa, {{pp, {"y"}}, {pp, {}}, {c, {}}});
             adapter::copy({2, o1, false, false, 1}, {2, r1->localPointer0, false, false, 2}, p.code);
             p.main.endLine();
             int o2 = p.call(p.main, fc, {{a, {}}, {pp, {}}, {c, {}}});
             adapter::copy({2, o2, false, false, 1}, {2, r2->localPointer0, false, false, 2}, p.code);
             p.main.endLine();
             p.expect("r1 = fa(pp.y, …) = pp.y", r1, 0x0A0B);
             p.expect("r2 = fc(…, &c) = c", r2, 0x3C3C);
             p.expect("pp se nezměnilo", pp, 0x0A0B0C0D);
         }},
        {"čtení pole přes referenci", "f(ref q: Pair) { t = q.y; return t };  r = f(pp)", [](Program &p) {
             auto &f = p.fn("frq", {"frqq"}, {"Pair"}, {true}, "number");
             int t = adapter::allocTemp(adapter::bodyFrame(f), 2);
             adapter::readVar(adapter::param(p.w, "frqq"), {"y"}, {2, t, false, false, 1}, p.code);
             adapter::returnFrom(f, {2, t, false, false, 1}, p.code);
             p.startMain();
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *r = p.var("r", "number", 0);
             p.assign(r, f, {pp});
             p.expect("r = f(&pp) = pp.y", r, 0x0A0B);
         }},
        {"zápis přes referenci", "s(ref q: Pair, v) { q.x = v; return v };  r = s(pp, a)", [](Program &p) {
             auto &f = p.fn("sw", {"swq", "swv"}, {"Pair", "number"}, {true, false}, "number");
             adapter::writeVar(adapter::param(p.w, "swq"), {"x"}, ret(p, "swv", "number"), p.code);
             adapter::returnFrom(f, ret(p, "swv", "number"), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x7777);
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *r = p.var("r", "number", 0);
             p.assign(r, f, {pp, a});
             p.expectAt("pp.x = a (zapsáno funkcí)", pp, 0, 2, 0x7777);
             p.expectAt("pp.y se nezměnilo", pp, 2, 2, 0x0A0B);
             p.expect("r = a", r, 0x7777);
         }},
        {"zápis přes dvě úrovně referencí", "s(ref q: Pair, v) { q.y = v; return v };  g(ref q: Pair, v) = s(q, v);  r = g(pp, a)",
         [](Program &p) {
             auto &s = p.fn("s2", {"s2q", "s2v"}, {"Pair", "number"}, {true, false}, "number");
             adapter::writeVar(adapter::param(p.w, "s2q"), {"y"}, ret(p, "s2v", "number"), p.code);
             adapter::returnFrom(s, ret(p, "s2v", "number"), p.code);
             auto &g = p.fn("g2", {"g2q", "g2v"}, {"Pair", "number"}, {true, false}, "number");
             int o = p.call(adapter::bodyFrame(g), s, {{adapter::param(p.w, "g2q"), {}}, {adapter::param(p.w, "g2v"), {}}});
             adapter::returnFrom(g, {2, o, false, false, 1}, p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x6B6B);
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *r = p.var("r", "number", 0);
             p.assign(r, g, {pp, a});
             p.expectAt("pp.x se nezměnilo", pp, 0, 2, 0x0C0D);
             p.expectAt("pp.y = a (zapsáno o dvě úrovně níž)", pp, 2, 2, 0x6B6B);
             p.expect("r = a", r, 0x6B6B);
         }},
        {"lokální proměnné přežijí volání", "f(x) = x;  h(x, y) { l = x; f(y); return l };  g(x, y) { l1 = y; l2 = x; return f(l1) }",
         [](Program &p) {
             auto &f = p.fn("f", {"fx"}, {"number"}, {false}, "number");
             adapter::returnFrom(f, ret(p, "fx", "number"), p.code);
             auto &h = p.fn("hl", {"hlx", "hly"}, {"number", "number"}, {false, false}, "number");
             auto *l = p.local(h, "hll", "number");
             adapter::copy(ret(p, "hlx", "number"), {2, l->localPointer0, false, false, 2}, p.code);
             adapter::endLine(adapter::bodyFrame(h));
             p.call(adapter::bodyFrame(h), f, {{adapter::param(p.w, "hly"), {}}});
             adapter::endLine(adapter::bodyFrame(h));
             adapter::returnFrom(h, {2, l->localPointer0, false, false, 1}, p.code);
             auto &g = p.fn("gl", {"glx", "gly"}, {"number", "number"}, {false, false}, "number");
             auto *l1 = p.local(g, "gll1", "number");
             auto *l2 = p.local(g, "gll2", "number");
             adapter::copy(ret(p, "gly", "number"), {2, l1->localPointer0, false, false, 2}, p.code);
             adapter::copy(ret(p, "glx", "number"), {2, l2->localPointer0, false, false, 2}, p.code);
             adapter::endLine(adapter::bodyFrame(g));
             int o = p.call(adapter::bodyFrame(g), f, {{l1, {}}});
             adapter::returnFrom(g, {2, o, false, false, 1}, p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x1111);
             auto *b = p.var("b", "number", 0x2222);
             auto *r1 = p.var("r1", "number", 0);
             auto *r2 = p.var("r2", "number", 0);
             p.assign(r1, h, {a, b});
             p.assign(r2, g, {a, b});
             p.expect("r1 = h(a, b) = a (lokální l přežila volání f)", r1, 0x1111);
             p.expect("r2 = g(a, b) = b", r2, 0x2222);
         }},
        {"výsledky volání jako argumenty", "g(x) = x;  first(x, y) = x;  second(x, y) = y;  r1 = first(g(a), g(b));  r2 = second(g(a), g(b))",
         [](Program &p) {
             auto &g = p.fn("gid", {"gidx"}, {"number"}, {false}, "number");
             adapter::returnFrom(g, ret(p, "gidx", "number"), p.code);
             auto &first = p.fn("first", {"fix", "fiy"}, {"number", "number"}, {false, false}, "number");
             adapter::returnFrom(first, ret(p, "fix", "number"), p.code);
             auto &second = p.fn("second", {"sex", "sey"}, {"number", "number"}, {false, false}, "number");
             adapter::returnFrom(second, ret(p, "sey", "number"), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x0A0A);
             auto *b = p.var("b", "number", 0x0B0B);
             auto *r1 = p.var("r1", "number", 0);
             auto *r2 = p.var("r2", "number", 0);
             int k = 0;
             for (auto [fn, r] : {std::pair{&first, r1}, std::pair{&second, r2}}) {
                 auto *t1 = p.named("tmpA" + num(k), "number", p.call(p.main, g, {{a, {}}}));
                 auto *t2 = p.named("tmpB" + num(k), "number", p.call(p.main, g, {{b, {}}}));
                 int o = p.call(p.main, *fn, {{t1, {}}, {t2, {}}});
                 adapter::copy({2, o, false, false, 1}, {2, r->localPointer0, false, false, 2}, p.code);
                 p.main.endLine();
                 k++;
             }
             p.expect("r1 = first(g(a), g(b)) = a", r1, 0x0A0A);
             p.expect("r2 = second(g(a), g(b)) = b", r2, 0x0B0B);
         }},
        {"vnořené volání s prohozenými parametry", "f(a, ref q: Pair, c) { q.x = a; return c };  g(x, ref q: Pair, y) = f(y, q, x);  r = g(a, pp, b)",
         [](Program &p) {
             auto &f = p.fn("fsw", {"fswa", "fswq", "fswc"}, {"number", "Pair", "number"}, {false, true, false}, "number");
             adapter::writeVar(adapter::param(p.w, "fswq"), {"x"}, ret(p, "fswa", "number"), p.code);
             adapter::returnFrom(f, ret(p, "fswc", "number"), p.code);
             auto &g = p.fn("gsw", {"gswx", "gswq", "gswy"}, {"number", "Pair", "number"}, {false, true, false}, "number");
             int o = p.call(adapter::bodyFrame(g), f,
                            {{adapter::param(p.w, "gswy"), {}}, {adapter::param(p.w, "gswq"), {}}, {adapter::param(p.w, "gswx"), {}}});
             adapter::returnFrom(g, {2, o, false, false, 1}, p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x00A1);
             auto *b = p.var("b", "number", 0x00B2);
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *r = p.var("r", "number", 0);
             p.assign(r, g, {a, pp, b});
             p.expect("r = g(a, pp, b) = a", r, 0x00A1);
             p.expectAt("pp.x = b", pp, 0, 2, 0x00B2);
             p.expectAt("pp.y se nezměnilo", pp, 2, 2, 0x0A0B);
         }},
        {"šest úrovní se strukturou", "f1(q: Pair) = q;  fk(q: Pair) = f(k-1)(q) pro k = 2..6;  r = f6(pp)", [](Program &p) {
             adapter::Function *prev = nullptr;
             for (int k = 1; k <= 6; k++) {
                 std::string n = "lv" + num(k), q = n + "q";
                 auto &f = p.fn(n, {q}, {"Pair"}, {false}, "Pair");
                 if (!prev) adapter::returnFrom(f, ret(p, q, "Pair"), p.code);
                 else {
                     int o = p.call(adapter::bodyFrame(f), *prev, {{adapter::param(p.w, q), {}}});
                     adapter::returnFrom(f, {4, o, false, false, 1}, p.code);
                 }
                 prev = &f;
             }
             p.startMain();
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *r = p.var("r", "Pair", 0);
             p.assign(r, *prev, {pp});
             p.expect("r = f6(pp), 4 B přes 6 rámců", r, 0x0A0B0C0D);
         }},
        {"volání ve vnořeném scopu", "f(x) = x;  main: r1, r2; { c; r1 = f(c) }  d; r2 = f(d)", [](Program &p) {
             auto &f = p.fn("f", {"fx"}, {"number"}, {false}, "number");
             adapter::returnFrom(f, ret(p, "fx", "number"), p.code);
             p.startMain();
             auto *r1 = p.var("r1", "number", 0);
             auto *r2 = p.var("r2", "number", 0);
             adapter::openScope(p.main);
             auto *c = p.var("c", "number", 0x0C0C);
             p.assign(r1, f, {c});
             adapter::closeScope(p.main, p.w);
             auto *d = p.var("d", "number", 0x0D0D);
             p.assign(r2, f, {d});
             p.expect("r1 = f(c) ve vnořeném scopu", r1, 0x0C0C);
             p.expect("r2 = f(d) po konci scopu", r2, 0x0D0D);
         }},
        {"sestavení struktury ve funkci", "mk(x, y) { Pair t; t.x = x; t.y = y; return t };  r = mk(a, b)", [](Program &p) {
             auto &f = p.fn("mk", {"mkx", "mky"}, {"number", "number"}, {false, false}, "Pair");
             auto *t = p.local(f, "mkt", "Pair");
             adapter::writeVar(t, {"x"}, ret(p, "mkx", "number"), p.code);
             adapter::writeVar(t, {"y"}, ret(p, "mky", "number"), p.code);
             adapter::returnFrom(f, {4, t->localPointer0, false, false, 1}, p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x1234);
             auto *b = p.var("b", "number", 0x5678);
             auto *r = p.var("r", "Pair", 0);
             p.assign(r, f, {a, b});
             p.expect("r = mk(a, b) = {a, b}", r, 0x56781234);
         }},
        // ------------------------------------------------ funkce A s x parametry vola B s y parametry, x != y
        {"A(1) volá B(4)", "B(a, b, c, d) = Quad{a, b, c, d};  A(x) { l1 = 0x1111; l2 = 0x2222; l3 = 0x3333; return B(l1, x, l2, l3) };  r = A(v)",
         [](Program &p) {
             defineQuad(p);
             auto &b = p.fn("b14", {"b14a", "b14b", "b14c", "b14d"}, {"number", "number", "number", "number"}, {false, false, false, false}, "Quad");
             returnQuadOf(p, b, "b14t", {"b14a", "b14b", "b14c", "b14d"});
             auto &a = p.fn("a14", {"a14x"}, {"number"}, {false}, "Quad");
             std::vector<variable *> ls;
             for (int i = 1; i <= 3; i++) {
                 auto *l = p.local(a, "a14l" + num(i), "number");
                 adapter::setLocal(0x1111 * i, l->localPointer0, 2, p.code);
                 ls.push_back(l);
             }
             adapter::endLine(adapter::bodyFrame(a));
             int o = p.call(adapter::bodyFrame(a), b, {{ls[0], {}}, {adapter::param(p.w, "a14x"), {}}, {ls[1], {}}, {ls[2], {}}});
             adapter::returnFrom(a, {8, o, false, false, 1}, p.code);
             p.startMain();
             auto *v = p.var("v", "number", 0x7A7A);
             auto *r = p.var("r", "Quad", 0);
             p.assign(r, a, {v});
             p.expect("r = Quad{0x1111, v, 0x2222, 0x3333}", r, quad(0x1111, 0x7A7A, 0x2222, 0x3333));
         }},
        {"A(4) volá B(1)", "B(x) = x;  A(a, b, c, d) = B(c);  r = A(p, q, s, t)", [](Program &p) {
             auto &b = p.fn("b41", {"b41x"}, {"number"}, {false}, "number");
             adapter::returnFrom(b, ret(p, "b41x", "number"), p.code);
             auto &a = p.fn("a41", {"a41a", "a41b", "a41c", "a41d"}, {"number", "number", "number", "number"}, {false, false, false, false}, "number");
             int o = p.call(adapter::bodyFrame(a), b, {{adapter::param(p.w, "a41c"), {}}});
             adapter::returnFrom(a, {2, o, false, false, 1}, p.code);
             p.startMain();
             auto *v1 = p.var("v1", "number", 0x0101);
             auto *v2 = p.var("v2", "number", 0x0202);
             auto *v3 = p.var("v3", "number", 0x0303);
             auto *v4 = p.var("v4", "number", 0x0404);
             auto *r = p.var("r", "number", 0);
             p.assign(r, a, {v1, v2, v3, v4});
             p.expect("r = A(v1, v2, v3, v4) = v3", r, 0x0303);
         }},
        {"A(2) volá B(4) dvakrát", "B(a, b, c, d) = Quad{a, b, c, d};  A(x, y) { B(y, y, x, x); return B(x, y, y, x) };  r = A(v, w)",
         [](Program &p) {
             defineQuad(p);
             auto &b = p.fn("b24", {"b24a", "b24b", "b24c", "b24d"}, {"number", "number", "number", "number"}, {false, false, false, false}, "Quad");
             returnQuadOf(p, b, "b24t", {"b24a", "b24b", "b24c", "b24d"});
             auto &a = p.fn("a24", {"a24x", "a24y"}, {"number", "number"}, {false, false}, "Quad");
             auto X = std::pair<variable *, std::vector<std::string>>{adapter::param(p.w, "a24x"), {}};
             auto Y = std::pair<variable *, std::vector<std::string>>{adapter::param(p.w, "a24y"), {}};
             p.call(adapter::bodyFrame(a), b, {Y, Y, X, X});
             adapter::endLine(adapter::bodyFrame(a));
             int o = p.call(adapter::bodyFrame(a), b, {X, Y, Y, X});
             adapter::returnFrom(a, {8, o, false, false, 1}, p.code);
             p.startMain();
             auto *v = p.var("v", "number", 0x00C1);
             auto *w = p.var("w", "number", 0x00D2);
             auto *r = p.var("r", "Quad", 0);
             p.assign(r, a, {v, w});
             p.expect("r = Quad{v, w, w, v}", r, quad(0xC1, 0xD2, 0xD2, 0xC1));
         }},
        {"A(3) s referencí volá B(5)", "B(ref q: Pair, a, b, c, d) { q.x = d; return Quad{a, b, c, d} };  A(x, ref q: Pair, y) = B(q, y, x, y, x);  r = A(v, pp, w)",
         [](Program &p) {
             defineQuad(p);
             auto &b = p.fn("b35", {"b35q", "b35a", "b35b", "b35c", "b35d"}, {"Pair", "number", "number", "number", "number"},
                            {true, false, false, false, false}, "Quad");
             adapter::writeVar(adapter::param(p.w, "b35q"), {"x"}, ret(p, "b35d", "number"), p.code);
             returnQuadOf(p, b, "b35t", {"b35a", "b35b", "b35c", "b35d"});
             auto &a = p.fn("a35", {"a35x", "a35q", "a35y"}, {"number", "Pair", "number"}, {false, true, false}, "Quad");
             auto X = std::pair<variable *, std::vector<std::string>>{adapter::param(p.w, "a35x"), {}};
             auto Y = std::pair<variable *, std::vector<std::string>>{adapter::param(p.w, "a35y"), {}};
             int o = p.call(adapter::bodyFrame(a), b, {{adapter::param(p.w, "a35q"), {}}, Y, X, Y, X});
             adapter::returnFrom(a, {8, o, false, false, 1}, p.code);
             p.startMain();
             auto *v = p.var("v", "number", 0x0A0A);
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *w = p.var("w", "number", 0x0B0B);
             auto *r = p.var("r", "Quad", 0);
             p.assign(r, a, {v, pp, w});
             p.expect("r = Quad{w, v, w, v}", r, quad(0x0B0B, 0x0A0A, 0x0B0B, 0x0A0A));
             p.expectAt("pp.x = v (zapsáno v B přes referenci z A)", pp, 0, 2, 0x0A0A);
             p.expectAt("pp.y se nezměnilo", pp, 2, 2, 0x0A0B);
         }},
        {"řetěz 1 → 3 → 0 → 2 parametry", "D(a, b) = b;  C() { l = 0x0C0C; return D(l, l) };  B(a, b, c) = C();  A(x) = B(x, x, x);  r = A(v)",
         [](Program &p) {
             auto &d = p.fn("ch_d", {"chda", "chdb"}, {"number", "number"}, {false, false}, "number");
             adapter::returnFrom(d, ret(p, "chdb", "number"), p.code);
             auto &c = p.fn("ch_c", {}, {}, {}, "number");
             auto *l = p.local(c, "chcl", "number");
             adapter::setLocal(0x0C0C, l->localPointer0, 2, p.code);
             adapter::endLine(adapter::bodyFrame(c));
             int oc = p.call(adapter::bodyFrame(c), d, {{l, {}}, {l, {}}});
             adapter::returnFrom(c, {2, oc, false, false, 1}, p.code);
             auto &b = p.fn("ch_b", {"chba", "chbb", "chbc"}, {"number", "number", "number"}, {false, false, false}, "number");
             int ob = p.call(adapter::bodyFrame(b), c, {});
             adapter::returnFrom(b, {2, ob, false, false, 1}, p.code);
             auto &a = p.fn("ch_a", {"chax"}, {"number"}, {false}, "number");
             auto X = std::pair<variable *, std::vector<std::string>>{adapter::param(p.w, "chax"), {}};
             int oa = p.call(adapter::bodyFrame(a), b, {X, X, X});
             adapter::returnFrom(a, {2, oa, false, false, 1}, p.code);
             p.startMain();
             auto *v = p.var("v", "number", 0x1357);
             auto *r = p.var("r", "number", 0);
             p.assign(r, a, {v});
             p.expect("r = A(v) = 0x0C0C (z C přes D)", r, 0x0C0C);
             p.expect("v se nezměnilo", v, 0x1357);
         }},
        {"pointingMode: zápis do ref number", "s(ref x, v) { x = v (pointing); return v };  r = s(a, b)", [](Program &p) {
             auto &f = p.fn("pn", {"pnx", "pnv"}, {"number", "number"}, {true, false}, "number");
             adapter::writeVar(adapter::param(p.w, "pnx"), {}, ret(p, "pnv", "number"), p.code, true);
             adapter::returnFrom(f, ret(p, "pnv", "number"), p.code);
             p.startMain();
             auto *a = p.var("a", "number", 0x1111);
             auto *b = p.var("b", "number", 0x2B2B);
             auto *r = p.var("r", "number", 0);
             p.assign(r, f, {a, b});
             p.expect("a = b (zapsáno přes referenci)", a, 0x2B2B);
             p.expect("b se nezměnilo", b, 0x2B2B);
             p.expect("r = b", r, 0x2B2B);
         }},
        {"pointingMode: celá struktura do ref Pair", "s(ref q: Pair, w: Pair) { q = w (pointing); return w };  r = s(pp, ww)", [](Program &p) {
             auto &f = p.fn("pq", {"pqq", "pqw"}, {"Pair", "Pair"}, {true, false}, "Pair");
             adapter::writeVar(adapter::param(p.w, "pqq"), {}, ret(p, "pqw", "Pair"), p.code, true);
             adapter::returnFrom(f, ret(p, "pqw", "Pair"), p.code);
             p.startMain();
             auto *pp = p.var("pp", "Pair", 0x0A0B0C0D);
             auto *ww = p.var("ww", "Pair", 0x51525354);
             auto *r = p.var("r", "Pair", 0);
             p.assign(r, f, {pp, ww});
             p.expectAt("pp.x = ww.x", pp, 0, 2, 0x5354);
             p.expectAt("pp.y = ww.y", pp, 2, 2, 0x5152);
             p.expect("r = ww", r, 0x51525354);
         }},
    };
}

inline std::vector<Result> runAll() {
    std::vector<Result> out;
    std::vector<int> sps = {0x1000, 0x5F00, 0xC000};
    uint32_t seed = 31337;
    for (auto &sc : scenarios()) {
        for (size_t si = 0; si < sps.size(); si++) {
            int sp0 = sps[si];
            Program p;
            sc.build(p);
            p.finish();

            Result r;
            r.suite = "calls";
            r.group = "volání";
            r.row = sc.name;
            r.col = "SP " + hex(sp0);
            r.code = p.code.code;
            r.layout = sc.desc + "  (SP na začátku " + hex(sp0) + ")";

            sim::Machine m;
            std::mt19937 rr(seed++);
            for (auto &b : m.ram) b = rr() & 0xFF;
            for (auto &x : m.reg) x = rr() & 0xFFFF;
            m.reg[3] = sp0;
            r.regsBefore = m.reg;
            m.recordTrace = true;
            m.maxSteps = 20000;
            m.run(r.code);
            takeRun(r, m);

            // nejdriv korenovy priznak (SP), az pak jeho dusledky (spatne hodnoty)
            if (m.errors.empty() && m.reg[3] != sp0) r.fail("SP se po návratu neobnovil");
            std::string checks = "[";
            for (size_t i = 0; i < p.checks.size(); i++) {
                auto &c = p.checks[i];
                long long act = 0;
                for (int k = 0; k < c.size; k++) act |= (long long)m.ram[(sp0 + c.off + k) & 0xFFFF] << (8 * k);
                bool ok = act == c.expected;
                if (!ok && m.errors.empty()) r.fail(c.what == "kód za posledním voláním proběhl" ? "kód za voláním neproběhl správně" : "špatná hodnota: " + c.what);
                checks += std::string(i ? "," : "") + "{\"what\":" + esc(c.what) + ",\"exp\":" + num(c.expected) + ",\"act\":" + num(act) +
                          ",\"ok\":" + boolj(ok) + "}";
            }
            checks += "]";
            int outside = 0;
            for (auto &wr : m.allWrites)
                if (wr.addr < sp0 || wr.addr >= sp0 + STACK_LIMIT) outside++;
            if (outside) r.fail("zápis mimo zásobník");

            std::vector<int> spSeries, pcs;
            for (auto &t : m.trace) spSeries.push_back((int)t.regs[3] - sp0), pcs.push_back(t.pc);
            std::string frames = arr(p.frames, [](const FrameInfo &f) {
                return "{\"fn\":" + esc(f.fn) + ",\"slots\":" + arr(f.slots, [](const Slot &s) {
                    return "{\"name\":" + esc(s.name) + ",\"off\":" + num(s.off) + ",\"size\":" + num(s.size) + "}";
                }) + "}";
            });
            r.extra = "{\"desc\":" + esc(sc.desc) + ",\"sp0\":" + num(sp0) + ",\"spEnd\":" + num(m.reg[3]) + ",\"steps\":" + num(m.steps) +
                      ",\"halted\":" + boolj(m.halted) + ",\"outside\":" + num(outside) + ",\"checks\":" + checks +
                      ",\"sp\":" + arr(spSeries, [](int x) { return num(x); }) + ",\"pcs\":" + arr(pcs, [](int x) { return num(x); }) +
                      ",\"frames\":" + frames + ",\"starts\":" + arr(p.starts, [](const std::pair<std::string, int> &x) {
                          return "{\"fn\":" + esc(x.first) + ",\"line\":" + num(x.second) + "}";
                      }) + "}";
            // trasa je v "extra" jako SP rada; plnou trasu nechame jen pro prvni SP (velikost reportu)
            if (si) r.trace.clear();
            out.push_back(r);
        }
    }
    return out;
}

} // namespace suite_calls
