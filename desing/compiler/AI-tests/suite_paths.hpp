#pragma once
// Sada "paths": variable::call / variable::set s cestou pres pole struktur a reference (a.b.c...).
// Typy se nadefinuji v kompilatoru a zaroven v nezavislem modelu tady; model spocita,
// kam ma cesta vest, a vysledek v RAM se porovna.
#include <functional>
#include <map>
#include <random>
#include "adapter.hpp"
#include "common.hpp"

namespace suite_paths {

using adapter::Place;

struct MField {
    std::string key, type;
    bool ref;
    int off;
};
struct MType {
    int size = 0;
    std::vector<MField> fields;
};

// nezavisly model typu (stejne typy, jake se nadefinuji v kompilatoru)
inline std::map<std::string, MType> model() {
    std::map<std::string, MType> t;
    t["number"].size = 2;
    auto add = [&](std::string n, std::vector<std::tuple<std::string, std::string, bool>> fs) {
        MType mt;
        for (auto &[k, ty, ref] : fs) {
            mt.fields.push_back({k, ty, ref, mt.size});
            mt.size += ref ? 2 : t[ty].size;
        }
        t[n] = mt;
    };
    add("Pair", {{"x", "number", false}, {"y", "number", false}});
    add("Node", {{"val", "number", false}, {"next", "Node", true}, {"p", "Pair", false}, {"rp", "Pair", true}});
    add("Box", {{"n", "Node", false}, {"rn", "Node", true}, {"z", "number", false}, {"rb", "Box", true}});
    return t;
}

inline void defineInCompiler(adapter::World &w, std::map<std::string, MType> &M) {
    adapter::defineNumber(w, 2);
    for (std::string n : {"Pair", "Node", "Box"}) {
        std::vector<adapter::Field> fs;
        for (auto &f : M[n].fields) fs.push_back({f.type, f.key, f.ref});
        adapter::defineStruct(w, n, fs);
    }
}

struct VarDef {
    std::string name, type;
    bool ref;
    int off;
};

// promenne v ramci (offset vuci SP)
inline std::vector<VarDef> frameVars() {
    return {{"b0", "Box", false, 0x00}, {"n0", "Node", false, 0x10}, {"p0", "Pair", false, 0x1A},
            {"rb", "Box", true, 0x1E},  {"rn", "Node", true, 0x20},  {"rp", "Pair", true, 0x22}};
}
const int TEMP = 0x40;  // druha strana kopirovani (docasna pamet v ramci)

inline const char *COLS[10] = {"čtení → hodnota", "čtení → pointer", "zápis ← hodnota", "zápis ← pointer",
                       "čtení → hodnota (R)", "čtení → pointer (R)", "zápis ← hodnota (R)", "zápis ← pointer (R)",
                       "zápis ← hodnota", "zápis ← hodnota (R)"};  // posledni dva = pointingMode

struct Outcome {
    Result r;
    std::string shape;
    int col;
};

inline Outcome runOne(std::mt19937 &rng, uint32_t seed, std::map<std::string, MType> &M) {
    adapter::World w;
    defineInCompiler(w, M);
    auto vars = frameVars();
    std::map<std::string, variable *> cv;
    for (auto &v : vars) cv[v.name] = adapter::addVar(w, v.name, v.type, v.off, v.ref);

    sim::Machine m;
    std::mt19937 rr(seed);
    for (auto &b : m.ram) b = rr() & 0xFF;
    for (auto &x : m.reg) x = rr() & 0xFFFF;
    int sp = 0x3000 + (rr() % 0x40) * 0x100 + (rr() % 0x100);
    m.reg[3] = sp;

    // globalni objekty + objekty na zasobniku; vsechny reference ukazuji na objekt spravneho typu
    std::map<std::string, std::vector<int>> pool;
    int g = 0x1000;
    for (std::string n : {"number", "Pair", "Node", "Box"})
        for (int i = 0; i < 4; i++) pool[n].push_back(g), g += M[n].size + 3;
    for (auto &v : vars)
        if (!v.ref) pool[v.type].push_back(sp + v.off);
    auto pick = [&](const std::string &t) { return pool[t][rr() % pool[t].size()]; };
    std::function<void(int, const std::string &)> fillRefs = [&](int addr, const std::string &t) {
        for (auto &f : M[t].fields) {
            if (f.ref) m.write16(addr + f.off, pick(f.type));
            else if (f.type != "number") fillRefs(addr + f.off, f.type);
        }
    };
    for (auto &[t, addrs] : pool)
        for (int a : addrs) fillRefs(a, t);
    for (auto &v : vars)
        if (v.ref) m.write16(sp + v.off, pick(v.type));

    // nahodna cesta
    const VarDef &v = vars[rng() % vars.size()];
    int depth = rng() % 5;
    std::vector<std::string> keys;
    std::string shape = v.ref ? "ref" : "hodnota";
    std::string type = v.type;
    bool isRef = v.ref, viaReg = false;
    int addr = sp + v.off;
    for (int d = 0; d < depth && type != "number"; d++) {
        auto &fs = M[type].fields;
        const MField &f = fs[rng() % fs.size()];
        if (isRef) addr = m.read16(addr), viaReg = true;
        addr += f.off;
        isRef = f.ref;
        type = f.type;
        keys.push_back(f.key);
        shape += f.ref ? ".ref" : ".pole";
    }
    int size = M[type].size;

    bool isSet = rng() % 2, otherRef = rng() % 2, otherInReg = rng() % 4 == 0;
    int otherReg = 1 + rng() % 2;
    bool pointing = isSet && !otherRef && rng() % 2;  // pointingMode: zapis do cile reference
    int col = pointing ? 8 + (otherInReg ? 1 : 0) : (isSet ? 2 : 0) + (otherRef ? 1 : 0) + (otherInReg ? 4 : 0);
    int tempAddr = sp + TEMP;
    if (otherRef) m.write16(tempAddr, isSet ? pick(type) : (int)(rr() & 0xFFFF));
    if (otherInReg) m.reg[otherReg] = tempAddr;

    Result r;
    r.suite = "paths";
    r.group = isSet ? "set" : "call";
    r.row = shape;
    r.col = COLS[col];
    r.regsBefore = m.reg;
    std::string path = v.name;
    for (auto &k : keys) path += "." + k;
    r.layout = (isSet ? path + (pointing ? " = temp (pointingMode)" : " = temp") : "temp = " + path) + "; SP = " + hex(sp) + ", cíl cesty " + hex(addr) +
               (isRef ? " (ref)" : "") + ", temp SP+" + num(TEMP) + (otherRef ? " (pointer)" : "") +
               (otherInReg ? ", adresa temp v R" + num(otherReg) : "");

    // ocekavani: copyFromTo semantika mezi cilem cesty a temp
    auto expected = m.ram;
    auto copyExp = [&](int fromSlot, bool fromPtr, int toSlot, bool toPtr, int toSize) {
        if (toPtr) {
            int val = fromPtr ? m.read16(fromSlot) : fromSlot;
            expected[toSlot] = val & 0xFF;
            expected[toSlot + 1] = val >> 8;
            return std::make_pair(toSlot, toSlot + 2);
        }
        int src = fromPtr ? m.read16(fromSlot) : fromSlot;
        for (int i = 0; i < toSize; i++) expected[toSlot + i] = m.ram[src + i];
        return std::make_pair(toSlot, toSlot + toSize);
    };
    std::pair<int, int> target;
    if (pointing && isRef) target = copyExp(tempAddr, false, m.read16(addr), false, size);  // cely objekt do cile reference
    else if (isSet) target = copyExp(tempAddr, otherRef, addr, isRef, isRef ? 2 : size);
    else target = copyExp(addr, isRef, tempAddr, otherRef, size);

    // v registru = pointer -1, stejne jako to kompilator predava sam sobe
    Place other{size, otherInReg ? -1 : TEMP, otherRef, otherInReg, otherReg};
    fullCompiledCode code;
    if (isSet) adapter::writeVar(cv[v.name], keys, other, code, pointing);
    else adapter::readVar(cv[v.name], keys, other, code);
    r.code = code.code;
    r.extra = "{\"viaReg\":" + boolj(viaReg) + ",\"depth\":" + num(keys.size()) + "}";

    if (r.code.empty()) {
        r.fail(viaReg ? "nic se nevygenerovalo (cíl je za referencí → copyFromTo odmítne place v registru)"
                      : "nic se nevygenerovalo");
        return {r, shape, col};
    }
    m.run(r.code);
    takeRun(r, m);
    compareRam(r, m, expected, {target});
    if (m.errors.empty() && m.reg[3] != sp) r.fail("změněn SP (R3)");
    return {r, shape, col};
}

inline std::string runAll() {
    auto M = model();
    std::mt19937 rng(4242);
    const int N = 6000;
    struct Cell {
        int pass = 0, fail = 0;
        bool have = false;
        Result first;
        std::map<std::string, int> reasons;
    };
    std::map<std::pair<std::string, int>, Cell> cells;
    for (int i = 0; i < N; i++) {
        Outcome o = runOne(rng, 9000 + i, M);
        Cell &c = cells[{o.shape, o.col}];
        if (o.r.status == "fail") {
            c.fail++;
            c.reasons[o.r.reason]++;
            if (!c.have) c.first = o.r, c.have = true;
        } else {
            c.pass++;
            if (!c.have && c.pass == 1) c.first = o.r;  // ukazka i pro prosle
        }
    }
    std::string o = "{\"runs\":" + num(N) + ",\"cols\":" + arr(std::vector<std::string>(COLS, COLS + 10), esc) + ",\"cells\":[";
    bool first = true;
    for (auto &[k, c] : cells) {
        std::vector<std::pair<std::string, int>> rs(c.reasons.begin(), c.reasons.end());
        o += std::string(first ? "" : ",") + "{\"shape\":" + esc(k.first) + ",\"col\":" + num(k.second) +
             ",\"pass\":" + num(c.pass) + ",\"fail\":" + num(c.fail) +
             ",\"reasons\":" + arr(rs, [](const std::pair<std::string, int> &p) { return "[" + esc(p.first) + "," + num(p.second) + "]"; }) +
             ",\"example\":" + resultJson(c.first) + "}";
        first = false;
    }
    return o + "]}";
}

} // namespace suite_paths
