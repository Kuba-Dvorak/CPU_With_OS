#pragma once
// Sada "expr": vyrazy pres syntaxAnalyzer (parseBT -> placeToTree / operatorToLevel -> createBinary -> syntaxOperations).
// Nahodny vyraz se napise jako text, nezavisly referencni parser (vlastni tabulka priorit, leva asociativita)
// spocita tvar stromu i hodnotu vcetne velikosti mezivysledku, kompilator ho prelozi, simulator spusti a porovna se:
//   1. tvar stromu (plne ozavorkovany zapis z basicNode)  2. hodnota vysledku  3. SP, zapisy mimo zasobnik, kod za vyrazem
#include <map>
#include <random>
#include "suite_calls.hpp"
#include "suite_ops.hpp"

namespace suite_expr {

using suite_ops::i128;
using suite_ops::str;
using suite_ops::toSigned;

// operatory, ktere ma syntaxForBT (xor tam znak nema)
struct BinOp {
    std::string sym;
    operators op;
    int level;   // NEZAVISLA tabulka priorit (vyssi = vaze silneji), jako v C
    int weight;  // jak casto se generuje
};
inline const std::vector<BinOp> &binOps() {
    static const std::vector<BinOp> v = {
        {"*", operators::multiply, 6, 2},  {"/", operators::divade, 6, 1},  {"+", operators::plus, 5, 3},
        {"-", operators::remove, 5, 3},    {">", operators::bigger, 4, 1},  {"<", operators::lesser, 4, 1},
        {"=>", operators::ebig, 4, 1},     {"=<", operators::esml, 4, 1},   {"==", operators::equal, 3, 1},
        {"=!", operators::notEqual, 3, 1}, {"&", operators::andOperator, 2, 1}, {"|", operators::orOperator, 0, 1},
    };
    return v;
}
inline std::string symOf(operators op) {
    for (auto &b : binOps())
        if (b.op == op) return b.sym;
    return "?";
}

// ---------------------------------------------------------------- vyraz: posloupnost operandu a operatoru
struct Seq;
struct Operand {
    enum Kind { NUM, VAR, BRACKET, CALL } kind = NUM;
    long long num = 0;
    std::string name;      // promenna nebo funkce
    bool notPrefix = false;
    std::vector<Seq> inner;  // zavorka: 1 vyraz, volani: argumenty
};
struct Seq {
    std::vector<Operand> operands;
    std::vector<int> ops;  // index do binOps(), ops.size() == operands.size() - 1
};

inline std::string text(const Seq &s);
inline std::string text(const Operand &o) {
    std::string p = o.notPrefix ? "!" : "";
    switch (o.kind) {
    case Operand::NUM: return p + std::to_string(o.num);
    case Operand::VAR: return p + o.name;
    case Operand::BRACKET: return p + "(" + text(o.inner[0]) + ")";
    case Operand::CALL: {
        std::string t = lexSyntax.callFunq + " " + o.name + " " + btSyntax.functionCallStart + " ";
        for (size_t i = 0; i < o.inner.size(); i++) t += (i ? btSyntax.nextSign + " " : "") + text(o.inner[i]);
        return p + t + " " + btSyntax.functionCallEnd;
    }
    }
    return "";
}
inline std::string text(const Seq &s) {
    std::string t = text(s.operands[0]);
    for (size_t i = 0; i < s.ops.size(); i++) t += " " + binOps()[s.ops[i]].sym + " " + text(s.operands[i + 1]);
    return t;
}

// ---------------------------------------------------------------- reference
struct Val {
    i128 v;
    int size;
};
struct Env {
    std::map<std::string, long long> vars;  // vse "number" = 2 B
    bool ok = true;
    std::string why;
};

inline int numSize(long long n) {  // jako evaluateNumber, ale presne: bitova delka + znamenkovy bit
    if (n == 0) return 1;
    int bits = 0;
    while ((n >> bits) != 0) bits++;
    return (bits + 1 + 7) / 8;
}

inline Val evalSeq(const Seq &s, Env &e, std::string *shapeOut = nullptr);
inline Val evalOperand(const Operand &o, Env &e) {
    Val r{0, 1};
    switch (o.kind) {
    case Operand::NUM: r = {o.num, numSize(o.num)}; break;
    case Operand::VAR: r = {e.vars[o.name], 2}; break;
    case Operand::BRACKET: r = evalSeq(o.inner[0], e); break;
    case Operand::CALL: {
        // fid(x) = x, fsec(x, y) = y; parametry i vystup jsou number (2 B) -> argument se zkrati na 2 B
        std::vector<Val> args;
        for (auto &a : o.inner) args.push_back(evalSeq(a, e));
        r = {toSigned(args.back().v, 2), 2};
        break;
    }
    }
    if (o.notPrefix) r.v = r.v == 0;  // logicka negace, velikost zustava
    return r;
}

inline Val binop(const BinOp &b, Val x, Val y, Env &e) {
    i128 out;
    if (!suite_ops::reference(b.op, x.v, x.size, y.v, y.size, out)) {
        e.ok = false, e.why = "deleni nulou / preteceni podilu";
        return {0, 1};
    }
    int big = std::max(x.size, y.size), size = 1;
    switch (b.op) {
    case operators::plus:
    case operators::remove: size = big + 1; break;
    case operators::multiply: size = x.size + y.size; break;
    case operators::divade: size = x.size; break;
    case operators::andOperator:
    case operators::orOperator: size = big; break;
    default: size = 1;
    }
    if (size > 15) e.ok = false, e.why = "prilis velke mezivysledky";
    return {out, size};
}

// precedence climbing; vraci hodnotu a plne ozavorkovany tvar
inline Val climb(const Seq &s, size_t &i, int minLevel, Env &e, std::string &shape, std::vector<std::string> &leaf) {
    Val lhs = evalOperand(s.operands[i], e);
    shape = leaf[i];
    while (i < s.ops.size() && binOps()[s.ops[i]].level >= minLevel) {
        const BinOp &b = binOps()[s.ops[i]];
        i++;
        std::string rshape;
        Val rhs = climb(s, i, b.level + 1, e, rshape, leaf);
        lhs = binop(b, lhs, rhs, e);
        shape = "(" + shape + " " + b.sym + " " + rshape + ")";
    }
    return lhs;
}

inline std::string leafShape(const Operand &o) {
    std::string p = o.notPrefix ? "!" : "";
    switch (o.kind) {
    case Operand::NUM: return p + std::to_string(o.num);
    case Operand::VAR: return p + o.name;
    case Operand::BRACKET: return p + "[]";
    case Operand::CALL: return p + "call";
    }
    return "";
}

inline Val evalSeq(const Seq &s, Env &e, std::string *shapeOut) {
    std::vector<std::string> leaf;
    for (auto &o : s.operands) leaf.push_back(leafShape(o));
    size_t i = 0;
    std::string shape;
    Val v = climb(s, i, -1, e, shape, leaf);
    if (shapeOut) *shapeOut = shape;
    return v;
}

// tvar stromu, ktery postavil kompilator
inline std::string treeShape(basicNode *n, int depth = 0) {
    if (!n || depth > 64) return "∅";
    std::string p = n->singleOper == singleOperator::notOperBit ? "!" : "";
    switch (n->myArType) {
    case nodeTypes::aritmo: return "(" + treeShape(n->leftNode, depth + 1) + " " + symOf(n->oper) + " " + treeShape(n->rightNode, depth + 1) + ")";
    case nodeTypes::number: return p + std::to_string(n->wantedNum);
    case nodeTypes::variable: return p + (n->myPassePair.var ? n->myPassePair.var->name : "?");
    case nodeTypes::function: return p + "call";
    case nodeTypes::funqPlace: return p + "[]";
    default: return "?";
    }
}

// ---------------------------------------------------------------- generator
struct Features {
    bool vars = false, brackets = false, calls = false, nots = false;
};

// uvnitr volani muzou byt vsechny operatory (od zmeny syntaxe volani na "/_ ... _/" uz nic nekoliduje)
inline bool hasAngle(const BinOp &) { return false; }

inline int pickOp(std::mt19937 &rng, bool inCall) {
    int total = 0;
    for (auto &b : binOps())
        if (!(inCall && hasAngle(b))) total += b.weight;
    int x = rng() % total;
    for (size_t i = 0; i < binOps().size(); i++)
        if (!(inCall && hasAngle(binOps()[i])) && (x -= binOps()[i].weight) < 0) return i;
    return 0;
}

inline const char *VARS[4] = {"a", "b", "c", "d"};

inline Seq genSeq(std::mt19937 &rng, int nOps, const Features &f, int depth, bool inCall = false);
inline Operand genOperand(std::mt19937 &rng, const Features &f, int depth, bool inCall) {
    Operand o;
    int k = rng() % 10;
    if (f.calls && depth < 2 && k < 3) {
        o.kind = Operand::CALL;
        bool two = rng() % 2;
        o.name = two ? "fsec" : "fid";
        for (int i = 0; i < (two ? 2 : 1); i++) o.inner.push_back(genSeq(rng, rng() % 2, f, depth + 1, true));
    } else if (f.brackets && depth < 2 && k < 6) {
        o.kind = Operand::BRACKET;
        o.inner.push_back(genSeq(rng, 1 + rng() % 2, f, depth + 1, inCall));
    } else if (f.vars && k < 8) {
        o.kind = Operand::VAR;
        o.name = VARS[rng() % 4];
    } else {
        o.kind = Operand::NUM;
        o.num = rng() % 4 == 0 ? rng() % 300 : rng() % 20;
    }
    if (f.nots && o.kind != Operand::CALL && rng() % 4 == 0) o.notPrefix = true;
    return o;
}
inline Seq genSeq(std::mt19937 &rng, int nOps, const Features &f, int depth, bool inCall) {
    Seq s;
    s.operands.push_back(genOperand(rng, f, depth, inCall));
    for (int i = 0; i < nOps; i++) {
        s.ops.push_back(pickOp(rng, inCall));
        s.operands.push_back(genOperand(rng, f, depth, inCall));
    }
    return s;
}

// ---------------------------------------------------------------- jeden vyraz
struct Outcome {
    Result r;
    bool shapeOk = true, valueOk = true;
};

inline Outcome runOne(const Seq &s, const std::map<std::string, long long> &vals, uint32_t seed, bool trace) {
    Outcome out;
    Result &r = out.r;
    r.suite = "expr";
    r.group = "výrazy";
    std::string src = text(s);

    Env env;
    env.vars = vals;
    std::string expShape;
    Val exp = evalSeq(s, env, &expShape);

    suite_calls::Program p;
    auto &fid = p.fn("fid", {"fidx"}, {"number"}, {false}, "number");
    adapter::returnFrom(fid, {2, adapter::paramOffset(p.w, "fidx"), false, false, 1}, p.code);
    auto &fsec = p.fn("fsec", {"fsecx", "fsecy"}, {"number", "number"}, {false, false}, "number");
    adapter::returnFrom(fsec, {2, adapter::paramOffset(p.w, "fsecy"), false, false, 1}, p.code);
    p.startMain();
    for (auto &[n, v] : vals) p.var(n, "number", v);
    int resOff = adapter::allocVar(p.main, 16, "res");
    int exprStart = p.code.currentLine;
    std::deque<basicNode> nodes;
    place o = adapter::parseTree(p.main, src, p.w, p.code, nodes);
    std::string gotShape = nodes.empty() ? "∅" : treeShape(nodes.back().getRoot());
    int exprEnd = p.code.currentLine;
    int resSize = std::min(std::max(o.size, 0), 16);
    if (resSize > 0 && o.pointer >= 0) adapter::copy({resSize, o.pointer, false, false, 1}, {resSize, resOff, false, false, 2}, p.code);
    p.main.endLine();
    p.finish();

    r.code = p.code.code;
    r.layout = src;
    out.shapeOk = gotShape == expShape;
    if (!out.shapeOk) r.fail("špatný tvar stromu (priorita / asociativita)");

    sim::Machine m;
    std::mt19937 rr(seed);
    for (auto &b : m.ram) b = rr() & 0xFF;
    m.randomizeRegs(rr);
    int sp0 = 0x2000 + (rr() % 0x80) * 0x100;
    m.reg[3] = sp0;
    r.regsBefore = m.reg;
    m.recordTrace = trace;
    m.maxTrace = 800;
    m.maxSteps = 400000;
    i128 act = 0;
    if (o.size <= 0 || o.pointer < 0) {
        r.fail("parseBT nevrátil platné place");
        out.valueOk = false;
    } else {
        m.run(r.code);
        takeRun(r, m);
        i128 u = 0;
        for (int k = 0; k < resSize; k++) u |= (i128)m.ram[(sp0 + resOff + k) & 0xFFFF] << (8 * k);
        act = toSigned(u, resSize);
        out.valueOk = m.errors.empty() && act == exp.v;
        if (m.errors.empty()) {
            if (m.reg[3] != sp0) r.fail("SP se neobnovil");
            if (act != exp.v) r.fail("špatná hodnota");
            if (r.status != "fail") {
                for (auto &c : p.checks) {
                    long long v = 0;
                    for (int k = 0; k < c.size; k++) v |= (long long)m.ram[(sp0 + c.off + k) & 0xFFFF] << (8 * k);
                    if (v != c.expected) r.fail("kód za výrazem neproběhl správně");
                }
                for (auto &w : m.allWrites)
                    if (w.addr < sp0 || w.addr >= sp0 + 0x800) {
                        r.fail("zápis mimo zásobník");
                        break;
                    }
            }
        }
    }
    std::string varsJson = "{";
    for (auto &[n, v] : vals) varsJson += (varsJson.size() > 1 ? "," : "") + esc(n) + ":" + num(v);
    varsJson += "}";
    r.extra = "{\"src\":" + esc(src) + ",\"expShape\":" + esc(expShape) + ",\"gotShape\":" + esc(gotShape) +
              ",\"exp\":" + esc(str(exp.v)) + ",\"expSize\":" + num(exp.size) + ",\"act\":" + esc(str(act)) +
              ",\"resSize\":" + num(o.size) + ",\"shapeOk\":" + boolj(out.shapeOk) + ",\"valueOk\":" + boolj(out.valueOk) +
              ",\"vars\":" + varsJson + ",\"exprLines\":[" + num(exprStart) + "," + num(exprEnd) + "],\"steps\":" + num(m.steps) + "}";
    if (!trace) r.trace.clear();
    return out;
}

// ---------------------------------------------------------------- sady
struct Cell {
    int pass = 0, fail = 0, shapeFail = 0, valueFail = 0;
    bool have = false;
    Result first;
    std::map<std::string, int> reasons;
};

inline std::string cellJson(const std::string &key, const Cell &c) {
    std::vector<std::pair<std::string, int>> rs(c.reasons.begin(), c.reasons.end());
    return "{" + key + ",\"pass\":" + num(c.pass) + ",\"fail\":" + num(c.fail) + ",\"shapeFail\":" + num(c.shapeFail) +
           ",\"valueFail\":" + num(c.valueFail) +
           ",\"reasons\":" + arr(rs, [](const std::pair<std::string, int> &p) { return "[" + esc(p.first) + "," + num(p.second) + "]"; }) +
           ",\"example\":" + resultJson(c.first) + "}";
}

inline void add(Cell &c, const Seq &s, const std::map<std::string, long long> &vals, uint32_t seed) {
    Outcome o = runOne(s, vals, seed, false);
    if (o.r.status == "fail") {
        c.fail++;
        c.shapeFail += !o.shapeOk;
        c.valueFail += !o.valueOk;
        c.reasons[o.r.reason]++;
        if (!c.have) c.first = runOne(s, vals, seed, true).r, c.have = true;
    } else {
        c.pass++;
        if (!c.have && c.pass == 1) c.first = runOne(s, vals, seed, true).r;
    }
}

inline std::string runAll(int &pairFails, int &fails, int &total) {
    std::mt19937 rng(97531);
    uint32_t seed = 70000;

    // 1. dvojice operatoru: "7 op1 5 op2 3" pro vsechny kombinace
    std::string pairs = "[";
    for (size_t i = 0; i < binOps().size(); i++)
        for (size_t j = 0; j < binOps().size(); j++) {
            Seq s;
            for (long long n : {7, 5, 3}) {
                Operand o;
                o.num = n;
                s.operands.push_back(o);
            }
            s.ops = {(int)i, (int)j};
            Cell c;
            add(c, s, {}, seed++);
            pairFails += c.fail;
            pairs += std::string(pairs.size() > 1 ? "," : "") + cellJson("\"row\":" + num(i) + ",\"col\":" + num(j), c);
        }
    pairs += "]";

    // 2. nahodne vyrazy podle toho, co obsahuji, a podle poctu operatoru
    struct Row {
        std::string name;
        Features f;
        int n;
    };
    std::vector<Row> rows = {{"jen čísla", {}, 60},
                             {"proměnné", {true}, 60},
                             {"závorky", {true, true}, 60},
                             {"volání funkcí", {true, true, true}, 30},
                             {"negace !", {true, false, false, true}, 30}};
    std::vector<std::string> cols = {"1 operátor", "2", "3", "4", "5"};
    std::string cells = "[";
    for (size_t ri = 0; ri < rows.size(); ri++)
        for (size_t ci = 0; ci < cols.size(); ci++) {
            Cell c;
            for (int k = 0; k < rows[ri].n; k++) {
                for (int attempt = 0; attempt < 50; attempt++) {
                    Seq s = genSeq(rng, ci + 1, rows[ri].f, 0);
                    std::map<std::string, long long> vals;
                    if (rows[ri].f.vars)
                        for (auto *v : VARS) vals[v] = (long long)(rng() % 601) - 300;
                    Env e;
                    e.vars = vals;
                    evalSeq(s, e);
                    if (!e.ok) continue;
                    add(c, s, vals, seed++);
                    break;
                }
            }
            fails += c.fail, total += c.pass + c.fail;
            cells += std::string(cells.size() > 1 ? "," : "") + cellJson("\"row\":" + num(ri) + ",\"col\":" + num(ci), c);
        }
    cells += "]";

    return "{\"ops\":" + arr(binOps(), [](const BinOp &b) { return esc(b.sym); }) + ",\"pairs\":" + pairs +
           ",\"rows\":" + arr(rows, [](const Row &r) { return esc(r.name); }) + ",\"cols\":" + arr(cols, esc) + ",\"cells\":" + cells + "}";
}

} // namespace suite_expr
