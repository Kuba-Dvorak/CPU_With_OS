#pragma once
// Sada "copy": copyFromTo a setRamLocal.
// Sloty lezi na zasobniku (offset vuci SP v R3) nebo je jejich adresa v R1/R2,
// pointery ukazuji do globalni oblasti. Kod se spusti a porovna se cela RAM.
#include <random>
#include "adapter.hpp"
#include "common.hpp"

namespace suite_copy {

using adapter::Place;

inline const char *TABLE[5] = {"hodnota → hodnota", "pointer → hodnota", "hodnota → pointer", "pointer → pointer",
                               "hodnota ⇒ cíl pointeru (pointing)"};
const int NTABLES = 5;
inline const char *LOC[3] = {"zásobník", "R1", "R2"};

struct Case {
    int table;     // 0..3 (bit0 = from je pointer, bit1 = to je pointer), 4 = hodnota -> pointer v pointingMode
    int fromLoc;   // 0 = zasobnik, 1 = R1, 2 = R2
    int toLoc;
    int size;
    int sp;
    int fromOff, toOff;  // offsety slotu vuci SP
    int dataAddr;        // kam ukazuje pointer ve "from" (absolutni); u pointing kam ukazuje pointer v "to"
};

inline std::string rowId(int table, int fl, int tl) { return std::string(TABLE[table]) + " | " + LOC[fl] + " → " + LOC[tl]; }

inline Result run(const Case &c, uint32_t seed, bool trace) {
    Result r;
    r.suite = "copy";
    r.group = "copyFromTo";
    r.row = rowId(c.table, c.fromLoc, c.toLoc);
    bool pointing = c.table == 4;
    bool fromPtr = !pointing && (c.table & 1), toPtr = pointing || (c.table & 2);

    sim::Machine m;
    std::mt19937 rng(seed);
    for (auto &b : m.ram) b = rng() & 0xFF;
    for (auto &x : m.reg) x = rng() & 0xFFFF;
    m.reg[3] = c.sp;

    int fromSlot = c.sp + c.fromOff, toSlot = c.sp + c.toOff;
    if (fromPtr) m.write16(fromSlot, c.dataAddr);
    if (pointing) m.write16(toSlot, c.dataAddr);
    if (c.fromLoc) m.reg[c.fromLoc] = fromSlot;
    if (c.toLoc) m.reg[c.toLoc] = toSlot;
    r.regsBefore = m.reg;

    r.layout = "SP = " + hex(c.sp) + ", z: SP+" + num(c.fromOff) + " (" + hex(fromSlot) + ")";
    if (fromPtr) r.layout += " → data " + hex(c.dataAddr);
    r.layout += ", do: SP+" + num(c.toOff) + " (" + hex(toSlot) + ")";
    if (pointing) r.layout += " → cíl " + hex(c.dataAddr);
    if (c.fromLoc) r.layout += std::string(", ") + LOC[c.fromLoc] + " = adresa z";
    if (c.toLoc) r.layout += std::string(", ") + LOC[c.toLoc] + " = adresa do";

    auto expected = m.ram;
    int tLen, tStart = toSlot;
    if (pointing) {
        // zapise se do cile pointeru, pointer sam zustane
        for (int i = 0; i < c.size; i++) expected[(c.dataAddr + i) & 0xFFFF] = m.ram[(fromSlot + i) & 0xFFFF];
        tStart = c.dataAddr;
        tLen = c.size;
    } else if (toPtr) {
        int v = fromPtr ? m.read16(fromSlot) : fromSlot;
        expected[toSlot & 0xFFFF] = v & 0xFF;
        expected[(toSlot + 1) & 0xFFFF] = v >> 8;
        tLen = 2;
    } else {
        int src = fromPtr ? c.dataAddr : fromSlot;
        for (int i = 0; i < c.size; i++) expected[(toSlot + i) & 0xFFFF] = m.ram[(src + i) & 0xFFFF];
        tLen = c.size;
    }

    // v registru = pointer -1, presne tak, jak to kompilator sam predava (loadFromTyper)
    Place f{c.size, c.fromLoc ? -1 : c.fromOff, fromPtr, c.fromLoc != 0, c.fromLoc ? c.fromLoc : 1};
    Place t{c.size, c.toLoc ? -1 : c.toOff, toPtr, c.toLoc != 0, c.toLoc ? c.toLoc : 2};
    fullCompiledCode code;
    bool ok = adapter::copy(f, t, code, pointing);
    r.code = code.code;

    bool sameReg = c.fromLoc && c.fromLoc == c.toLoc;
    if (sameReg) {
        if (!ok && r.code.empty()) r.reason = "správně odmítnuto (stejný registr, vrací false)";
        else r.fail("stejný registr na obou stranách, ale něco se vygenerovalo");
        r.extra = "{\"instr\":" + num(r.code.size()) + "}";
        return r;
    }
    if (!ok) {
        r.fail("copyFromTo vrátilo false, nic se nezkopírovalo");
        r.extra = "{\"instr\":0}";
        return r;
    }

    m.recordTrace = trace;
    m.run(r.code);
    takeRun(r, m);
    compareRam(r, m, expected, {{tStart, tStart + tLen}});
    if (m.errors.empty() && m.reg[3] != c.sp) r.fail("změněn SP (R3)");
    r.extra = "{\"instr\":" + num(r.code.size()) + "}";
    return r;
}

inline std::vector<Result> runAll(std::string &fuzzOut) {
    std::vector<Result> out;
    uint32_t seed = 1;
    std::vector<int> sps = {0x1000, 0x40F8, 0xC000};
    std::vector<int> sizes = {1, 2, 3, 4, 8, 16};

    for (int t = 0; t < NTABLES; t++)
        for (int fl = 0; fl < 3; fl++)
            for (int tl = 0; tl < 3; tl++)
                for (int sp : sps)
                    for (int s : sizes) {
                        // pri SP 0x40F8 lezi slot i data pres hranici xxFF -> xx00
                        Case c{t, fl, tl, s, sp, 0x10, 0x40, sp == 0x40F8 ? 0x20FE : 0x2000};
                        Result r = run(c, seed++, sp == 0x1000 && s == 2);
                        r.col = "SP " + hex(sp) + " | " + num(s) + " B";
                        out.push_back(r);
                    }

    // fuzz po radcich
    std::mt19937 rng(777);
    const int N = 300;
    fuzzOut = "[";
    bool firstRow = true;
    for (int t = 0; t < NTABLES; t++)
        for (int fl = 0; fl < 3; fl++)
            for (int tl = 0; tl < 3; tl++) {
                int pass = 0, fail = 0;
                Result firstFail;
                bool have = false;
                for (int i = 0; i < N; i++) {
                    Case c;
                    c.table = t, c.fromLoc = fl, c.toLoc = tl;
                    c.size = 1 + rng() % 32;
                    c.sp = 0x0100 + rng() % 0xE000;
                    c.fromOff = rng() % 0x200;
                    c.toOff = 0x240 + rng() % 0x200;
                    c.dataAddr = 0x0010 + rng() % 0x00C0;  // pod nejnizsim SP
                    Result r = run(c, seed++, false);
                    if (r.status == "fail") {
                        fail++;
                        if (!have) firstFail = r, have = true;
                    } else pass++;
                }
                fuzzOut += std::string(firstRow ? "" : ",") + "{\"row\":" + esc(rowId(t, fl, tl)) + ",\"runs\":" + num(N) +
                           ",\"pass\":" + num(pass) + ",\"fail\":" + num(fail) + (have ? ",\"firstFail\":" + resultJson(firstFail) : "") + "}";
                firstRow = false;
            }
    fuzzOut += "]";

    // setRamLocal: cislo do lokalni pameti po bajtech (little endian)
    for (int i = 0; i < 48; i++) {
        Result r;
        r.suite = "copy";
        r.group = "setRamLocal";
        int size = 1 + i % 4;
        int sp = 0x0200 + (rng() % 0xD000);
        int off = rng() % 0x300;
        int value = rng() & 0x7FFFFFFF;
        r.row = num(size) + " B";
        r.col = num(i / 4);
        r.layout = "SP = " + hex(sp) + ", SP+" + num(off) + " = " + hex(value, 8);
        sim::Machine m;
        std::mt19937 rr(seed++);
        for (auto &b : m.ram) b = rr() & 0xFF;
        for (auto &x : m.reg) x = rr() & 0xFFFF;
        m.reg[3] = sp;
        r.regsBefore = m.reg;
        auto expected = m.ram;
        for (int k = 0; k < size; k++) expected[(sp + off + k) & 0xFFFF] = (value >> (8 * k)) & 0xFF;
        fullCompiledCode code;
        bool ok = adapter::setLocal(value, off, size, code);
        r.code = code.code;
        if (!ok) r.fail("setRamLocal vrátilo false");
        else {
            m.run(r.code);
            takeRun(r, m);
            compareRam(r, m, expected, {{sp + off, sp + off + size}});
            if (m.errors.empty() && m.reg[3] != sp) r.fail("změněn SP (R3)");
        }
        r.extra = "{\"instr\":" + num(r.code.size()) + "}";
        out.push_back(r);
    }
    // hranice: prilis velka pozice / velikost se ma odmitnout
    for (auto [pos, size] : std::vector<std::pair<int, int>>{{70000, 2}, {10, 70000}}) {
        Result r;
        r.suite = "copy";
        r.group = "setRamLocal";
        r.row = "hranice";
        r.col = "pozice " + num(pos) + ", velikost " + num(size);
        r.layout = r.col;
        fullCompiledCode code;
        bool ok = adapter::setLocal(1, pos, size, code);
        r.code = code.code;
        if (ok || !r.code.empty()) r.fail("mimo rozsah RAM, ale nebylo odmítnuto");
        else r.reason = "správně odmítnuto";
        r.extra = "{\"instr\":" + num(r.code.size()) + "}";
        out.push_back(r);
    }
    return out;
}

} // namespace suite_copy
