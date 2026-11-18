#pragma once
// Sada "ops": aritmetika a logika nad libovolne velkymi cisly (syntaxOperations.cpp) pres basicNode::evaluateOperator.
// Dva operandy ruzne velikosti (1..4 B, znamenkova cisla v doplnkovem kodu) lezi v ramci na SP+0 a SP+8,
// spusti se vygenerovany kod a vysledek z vraceneho place se porovna s vypoctem v C++.
#include <map>
#include <random>
#include "adapter.hpp"
#include "common.hpp"

namespace suite_ops {

using i128 = __int128;

struct Op {
    std::string sym;
    operators op;
};

inline const std::vector<Op> &ops() {
    static const std::vector<Op> v = {
        {"+", operators::plus},          {"-", operators::remove},   {"*", operators::multiply}, {"/", operators::divade},
        {"&", operators::andOperator},   {"|", operators::orOperator}, {"^", operators::xorOperator},
        {"==", operators::equal},        {"=!", operators::notEqual}, {">", operators::bigger},  {"<", operators::lesser},
        {"=>", operators::ebig},         {"=<", operators::esml},
    };
    return v;
}

inline i128 lo(int size) { return -((i128)1 << (8 * size - 1)); }
inline i128 hi(int size) { return ((i128)1 << (8 * size - 1)) - 1; }
inline i128 umask(int size) { return ((i128)1 << (8 * size)) - 1; }
inline i128 toSigned(i128 u, int size) {
    u &= umask(size);
    return (u & ((i128)1 << (8 * size - 1))) ? u - ((i128)1 << (8 * size)) : u;
}
inline std::string str(i128 v) {
    if (v == 0) return "0";
    bool neg = v < 0;
    unsigned __int128 u = neg ? -(unsigned __int128)v : v;
    std::string s;
    while (u) s = char('0' + int(u % 10)) + s, u /= 10;
    return neg ? "-" + s : s;
}

// referencni vysledek; ok = false -> tuhle dvojici nema smysl testovat (deleni nulou, preteceni podilu)
inline bool reference(operators op, i128 a, int sa, i128 b, int sb, i128 &out) {
    int big = std::max(sa, sb);
    switch (op) {
    case operators::plus: out = a + b; return true;
    case operators::remove: out = a - b; return true;
    case operators::multiply: out = a * b; return true;
    case operators::divade:
        if (b == 0 || (a == lo(sa) && b == -1)) return false;
        out = a / b;  // zaokrouhleni k nule jako v C
        return true;
    // logicke operace doplnuji kratsi operand nulami (logicalOperation: reg6 = 0), vysledek ma vetsi z velikosti
    case operators::andOperator: out = toSigned((a & umask(sa)) & (b & umask(sb)), big); return true;
    case operators::orOperator: out = toSigned((a & umask(sa)) | (b & umask(sb)), big); return true;
    case operators::xorOperator: out = toSigned((a & umask(sa)) ^ (b & umask(sb)), big); return true;
    case operators::equal: out = a == b; return true;
    case operators::notEqual: out = a != b; return true;
    case operators::bigger: out = a > b; return true;
    case operators::lesser: out = a < b; return true;
    case operators::ebig: out = a >= b; return true;
    case operators::esml: out = a <= b; return true;
    }
    return false;
}

const int A_OFF = 0, B_OFF = 8, FRAME = 16, STACK_LIMIT = 0x400;

struct Run {
    Result r;
    i128 expected = 0, actual = 0;
    int resSize = 0;
};

inline Run runOne(const Op &o, i128 a, int sa, i128 b, int sb, uint32_t seed, bool trace) {
    Run out;
    Result &r = out.r;
    r.suite = "ops";
    r.group = "evaluateOperator";
    r.row = o.sym;
    r.col = num(sa) + " B × " + num(sb) + " B";

    adapter::Frame f;
    adapter::openScope(f);
    adapter::allocVar(f, FRAME, "operandy");
    fullCompiledCode code;
    place res = adapter::evalOperator(o.op, f, {sa, A_OFF, false, false, 1}, {sb, B_OFF, false, false, 1}, code);
    r.code = code.code;
    out.resSize = res.size;
    reference(o.op, a, sa, b, sb, out.expected);

    sim::Machine m;
    std::mt19937 rng(seed);
    for (auto &x : m.ram) x = rng() & 0xFF;
    m.randomizeRegs(rng);
    int sp = 0x1000 + (rng() % 0xA0) * 0x100 + (rng() % 0x100);
    m.reg[3] = sp;
    for (int k = 0; k < sa; k++) m.ram[sp + A_OFF + k] = (uint8_t)(a >> (8 * k));
    for (int k = 0; k < sb; k++) m.ram[sp + B_OFF + k] = (uint8_t)(b >> (8 * k));
    r.regsBefore = m.reg;
    r.layout = "a = " + str(a) + " (" + num(sa) + " B na SP+" + num(A_OFF) + "), b = " + str(b) + " (" + num(sb) + " B na SP+" +
               num(B_OFF) + "), SP = " + hex(sp) + "; výsledek " + num(res.size) + " B na SP+" + num(res.pointer);

    if (res.size <= 0 || res.pointer < 0) {
        r.fail("evaluateOperator nevrátil platné place");
    } else {
        m.recordTrace = trace;
        m.maxTrace = 600;
        m.run(r.code);
        takeRun(r, m);
        i128 u = 0;
        int n = std::min(res.size, 15);
        for (int k = 0; k < n; k++) u |= (i128)m.ram[(sp + res.pointer + k) & 0xFFFF] << (8 * k);
        out.actual = toSigned(u, n);
        if (m.errors.empty()) {
            if (m.reg[3] != sp) r.fail("změněn SP (R3)");
            else if (out.actual != out.expected) r.fail("špatný výsledek");
            int outside = 0, operand = 0;
            for (auto &w : m.allWrites) {
                if (w.addr >= sp && w.addr < sp + FRAME) operand++;
                else if (w.addr < sp || w.addr >= sp + STACK_LIMIT) outside++;
            }
            if (outside) r.fail("zápis mimo zásobník");
            // v kompilatoru jsou operandy vzdy docasne kopie, takze prepsani operandu je jen varovani
            else if (operand) r.warn("přepisuje operand");
        }
    }
    r.extra = "{\"a\":" + esc(str(a)) + ",\"b\":" + esc(str(b)) + ",\"sa\":" + num(sa) + ",\"sb\":" + num(sb) +
              ",\"exp\":" + esc(str(out.expected)) + ",\"act\":" + esc(str(out.actual)) + ",\"resSize\":" + num(res.size) +
              ",\"steps\":" + num(m.steps) + ",\"instr\":" + num(r.code.size()) + "}";
    return out;
}

// hodnoty: okraje rozsahu + nahodne
inline std::vector<i128> values(int size, std::mt19937 &rng, int nRandom) {
    std::vector<i128> v = {0, 1, -1, 2, -2, hi(size), lo(size)};
    if (size > 1) v.push_back(127), v.push_back(-128), v.push_back(255), v.push_back(256);
    for (int i = 0; i < nRandom; i++) v.push_back(lo(size) + (i128)(((unsigned long long)rng() << 32 | rng()) % (unsigned long long)(umask(size) + 1)));
    return v;
}

inline std::string runAll(int &fails, int &total) {
    struct Cell {
        int pass = 0, fail = 0;
        bool have = false;
        Result first;
        std::map<std::string, int> reasons;
        long long steps = 0;
        int instr = 0;
    };
    std::map<std::pair<int, int>, Cell> cells;  // (op, sizes)
    std::vector<std::string> cols;
    std::mt19937 rng(2468);
    uint32_t seed = 50000;
    for (int sa = 1; sa <= 4; sa++)
        for (int sb = 1; sb <= 4; sb++) cols.push_back(num(sa) + " B × " + num(sb) + " B");

    for (size_t oi = 0; oi < ops().size(); oi++) {
        const Op &o = ops()[oi];
        int nRandom = o.op == operators::divade ? 5 : 12;
        for (int sa = 1; sa <= 4; sa++)
            for (int sb = 1; sb <= 4; sb++) {
                int ci = (sa - 1) * 4 + (sb - 1);
                Cell &c = cells[{(int)oi, ci}];
                auto va = values(sa, rng, nRandom), vb = values(sb, rng, nRandom);
                std::vector<std::pair<i128, i128>> pairs;
                for (size_t i = 0; i < va.size(); i++) pairs.push_back({va[i], vb[(i * 7 + 3) % vb.size()]});
                for (i128 x : {(i128)0, (i128)-1, hi(sa)})
                    for (i128 y : {(i128)1, (i128)-1, hi(sb), lo(sb)}) pairs.push_back({x, y});
                for (auto [a, b] : pairs) {
                    i128 tmp;
                    if (!reference(o.op, a, sa, b, sb, tmp)) continue;
                    uint32_t s = seed++;
                    Run run = runOne(o, a, sa, b, sb, s, false);
                    c.steps = std::max<long long>(c.steps, std::atoll(run.r.extra.c_str() + run.r.extra.find("\"steps\":") + 8));
                    c.instr = std::max<int>(c.instr, run.r.code.size());
                    if (run.r.status == "fail") {
                        c.fail++;
                        c.reasons[run.r.reason]++;
                        if (!c.have) c.first = runOne(o, a, sa, b, sb, s, true).r, c.have = true;  // znovu s trasou
                    } else {
                        c.pass++;
                        if (!c.have && c.pass == 1) c.first = runOne(o, a, sa, b, sb, s, true).r;
                    }
                }
            }
    }

    for (auto &[k, c] : cells) fails += c.fail, total += c.pass + c.fail;
    std::string out = "{\"rows\":" + arr(ops(), [](const Op &o) { return esc(o.sym); }) + ",\"cols\":" + arr(cols, esc) + ",\"cells\":[";
    bool first = true;
    for (auto &[k, c] : cells) {
        std::vector<std::pair<std::string, int>> rs(c.reasons.begin(), c.reasons.end());
        out += std::string(first ? "" : ",") + "{\"row\":" + num(k.first) + ",\"col\":" + num(k.second) + ",\"pass\":" + num(c.pass) +
               ",\"fail\":" + num(c.fail) + ",\"steps\":" + num(c.steps) + ",\"instr\":" + num(c.instr) +
               ",\"reasons\":" + arr(rs, [](const std::pair<std::string, int> &p) { return "[" + esc(p.first) + "," + num(p.second) + "]"; }) +
               ",\"example\":" + resultJson(c.first) + "}";
        first = false;
    }
    return out + "]}";
}

} // namespace suite_ops
