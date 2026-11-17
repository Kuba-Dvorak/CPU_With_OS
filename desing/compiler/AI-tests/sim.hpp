#pragma once
// Referencni simulator ISA tak, jak ji pouziva kompilator (syntax.cpp, memoryManagment/*, syntaxOperations.cpp).
// Spousti textove instrukce z fullCompiledCode::code vcetne skoku a zaznamenava RAM, registry a latche.
//
// PREDPOKLADY (pokud neco nesedi s tvym CPU, oprav to tady - vsechny testy se tim ridi):
//   - registry R0..R7 jsou 16bitove; R3 = stack pointer, R4 = pomocny pro vypocty s SP; signReg1/2 jsou datove registry; RNone se cte jako 0
//   - latche LACI/LACO (carry in/out), LC (vysledek porovnani), LSkip (preskoceni), LShiftLow (bit vysunuty shiftem) maji hodnotu 0/1
//   - registr na miste RAM operandu = neprima adresa, napr. movRamRam R1 R2 kopiruje RAM[R1] -> RAM[R2]
//   - mov*: 1. operand odkud, 2. operand kam;  set*: 1. operand kam, 2. operand co;  aritmetika: co1, co2, kam (bez "kam" se pise do co1)
//   - addNoC A B C: C = A + B, 16 bit, bez carry (pouziva se na adresy)
//   - addReg8 A B C: C = (A + B + LACI) & 0xFF, LACO = preteceni z 8 bitu;  addReg16: totez v 16 bitech (taky bere LACI a nastavuje LACO)
//   - rm8 A B C: C = A - B, 16 bit, bez carry (pouziva se na SP)
//   - mut8 A B C: C = (A & 0xFF) * (B & 0xFF), 16bitovy vysledek, bez znamenka
//   - neg8 A B: B = ~A & 0xFF;  neg16: totez 16 bit;  neg1 A B: B = (A == 0 ? 1 : 0)
//   - and8 / or8 / xorReg A B C: C = (A op B) & 0xFF
//   - setBySign8 A B: B = nejvyssi bit dolniho bajtu A ? 0xFF : 0x00 (znamenkove rozsireni)
//   - cmpEqual A: LC = (A & 0xFF) == 0;  cmpSign A: LC = nejvyssi bit dolniho bajtu A
//   - shiftL1 A B: bity 0..6 B = bity 1..7 A, bit 7 B se NEMENI, LShiftLow = nejnizsi bit A  (potvrzeno 15.11.)
//   - putTo A: bit 7 A = LShiftLow (nastavi 0 i 1, ostatni bity beze zmeny)
//   - increment A [B]: B = A + 1 (16 bit), bez B primo v A
//   - LSkip = 1: dalsi instrukce se preskoci a LSkip se vynuluje
//   - jump N / jumpReg A: absolutni skok na index instrukce (zatim radky, bez *5); konec programu = skok za posledni instrukci
//   - *16 instrukce: little endian (dolni bajt na nizsi adrese); adresa 0 je rezervovana, pristup na ni je chyba
#include <array>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace sim {

// indexy registru a latchu (0..7 = R0..R7, aby m.reg[3] byl SP)
enum Reg { SIGN1 = 8, SIGN2 = 9, RNONE = 10, LACI = 11, LACO = 12, LC = 13, LSKIP = 14, LSHIFT = 15, NREG = 16 };
inline const char *REG_NAMES[NREG] = {"R0",   "R1",   "R2",   "R3",   "R4", "R5",    "R6",    "R7",
                                      "signReg1", "signReg2", "RNone", "LACI", "LACO", "LC", "LSkip", "LShiftLow"};
inline bool isLatch(int r) { return r >= LACI; }

struct RamWrite {
    uint16_t addr;
    uint8_t before;
    uint8_t after;
};

struct TraceRow {
    int pc;
    std::array<uint16_t, NREG> regs;  // po provedeni
    std::vector<RamWrite> writes;
};

struct Error {
    int line;
    std::string text;
    std::string msg;
};

class Machine {
public:
    std::array<uint8_t, 65536> ram{};
    std::array<uint16_t, NREG> reg{};
    std::vector<Error> errors;
    std::vector<TraceRow> trace;
    bool recordTrace = false;
    size_t maxTrace = 4000;
    std::set<int> regsWritten;
    std::vector<RamWrite> allWrites;
    int steps = 0;
    int maxSteps = 200000;
    bool halted = false;

    uint16_t read16(int a) const { return ram[a & 0xFFFF] | (ram[(a + 1) & 0xFFFF] << 8); }
    void write16(int a, uint16_t v) {
        ram[a & 0xFFFF] = v & 0xFF;
        ram[(a + 1) & 0xFFFF] = v >> 8;
    }
    // nahodne registry + latche 0/1 (tak, jak muzou zustat po predchozim kodu, napr. carry LACI)
    template <class Rng> void randomizeRegs(Rng &rng) {
        for (int i = 0; i < NREG; i++) reg[i] = isLatch(i) ? rng() & 1 : rng() & 0xFFFF;
        reg[RNONE] = 0;
        reg[LSKIP] = 0;  // LSkip se po pouziti vzdy vynuluje, na zacatku kodu tedy byva 0
    }

    // spusti od radku 0, dokud pc nevyjede za konec kodu
    bool run(const std::vector<std::string> &code) {
        int pc = 0;
        while (pc != (int)code.size()) {
            if (pc < 0 || pc > (int)code.size()) {
                errors.push_back({pc, "", "skok mimo kod (pc = " + std::to_string(pc) + ")"});
                return false;
            }
            if (++steps > maxSteps) {
                errors.push_back({pc, code[pc], "prekrocen limit " + std::to_string(maxSteps) + " kroku (nekonecna smycka?)"});
                return false;
            }
            curWrites.clear();
            int next = pc + 1;
            if (reg[LSKIP]) {
                reg[LSKIP] = 0;  // preskocena instrukce se neprovede
            } else if (!exec(pc, code[pc], next)) {
                return false;
            }
            if (recordTrace && trace.size() < maxTrace) trace.push_back({pc, reg, curWrites});
            pc = next;
        }
        halted = true;
        return true;
    }

private:
    std::vector<RamWrite> curWrites;
    std::vector<std::string> tok;
    size_t pos = 0;
    int curLine = 0;
    std::string curText;
    bool bad = false;

    void fail(const std::string &msg) {
        if (!bad) errors.push_back({curLine, curText, msg});
        bad = true;
    }

    std::string next() {
        if (pos >= tok.size()) {
            fail("chybi operand");
            return "";
        }
        return tok[pos++];
    }
    bool more() const { return pos < tok.size(); }

    static bool isInt(const std::string &s) {
        if (s.empty()) return false;
        size_t i = (s[0] == '-') ? 1 : 0;
        if (i >= s.size()) return false;
        for (; i < s.size(); i++)
            if (s[i] < '0' || s[i] > '9') return false;
        return true;
    }

    static int regOf(const std::string &t) {
        for (int i = 0; i < NREG; i++)
            if (t == REG_NAMES[i]) return i;
        return -1;
    }

    int parseReg() {
        std::string t = next();
        int r = regOf(t);
        if (r < 0) fail("ocekavan registr, nalezeno '" + t + "'");
        return r < 0 ? 0 : r;
    }

    // registr = neprima adresa, cislo = primo adresa
    int parseAddr() {
        std::string t = next();
        int r = regOf(t);
        if (r >= 0) return reg[r];
        if (isInt(t)) return std::stoi(t) & 0xFFFF;
        fail("neznamy adresni operand '" + t + "'");
        return 1;
    }

    int parseValue() {
        std::string t = next();
        if (isInt(t)) return std::stoi(t);
        fail("ocekavano cislo, nalezeno '" + t + "'");
        return 0;
    }

    // registr nebo cislo
    int parseRegOrValue() {
        std::string t = next();
        int r = regOf(t);
        if (r >= 0) return reg[r];
        if (isInt(t)) return std::stoi(t);
        fail("ocekavan registr nebo cislo, nalezeno '" + t + "'");
        return 0;
    }

    void checkAddr(int a) {
        if ((a & 0xFFFF) == 0) fail("pristup na adresu 0 (rezervovana)");
    }

    uint8_t rd(int a) {
        checkAddr(a);
        return ram[a & 0xFFFF];
    }

    void wr(int a, uint8_t v) {
        checkAddr(a);
        if (bad) return;
        RamWrite w{(uint16_t)(a & 0xFFFF), ram[a & 0xFFFF], v};
        curWrites.push_back(w);
        allWrites.push_back(w);
        ram[a & 0xFFFF] = v;
    }

    void setReg(int r, int v) {
        if (bad) return;
        if (r == RNONE) {
            fail("zapis do RNone");
            return;
        }
        reg[r] = isLatch(r) ? (v != 0) : (v & 0xFFFF);
        regsWritten.insert(r);
    }

    // aritmetika "co1 co2 [kam]": vrati cil (bez kam = co1)
    int dest(int a) { return more() ? parseReg() : a; }

    bool exec(int line, const std::string &text, int &nextPc) {
        tok.clear();
        pos = 0;
        curLine = line;
        curText = text;
        bad = false;
        std::istringstream in(text);
        for (std::string t; in >> t;) tok.push_back(t);
        if (tok.empty()) {
            fail("prazdna instrukce");
            return false;
        }
        std::string op = next();

        if (op == "setReg") {
            int r = parseReg();
            int v = parseValue();
            setReg(r, v);
        } else if (op == "movRegReg") {
            int a = parseReg(), b = parseReg();
            setReg(b, reg[a] & 0xFF);
        } else if (op == "movRegReg16") {
            int a = parseReg(), b = parseReg();
            setReg(b, reg[a]);
        } else if (op == "movRamReg") {
            int a = parseAddr(), r = parseReg();
            setReg(r, rd(a));
        } else if (op == "movRamReg16") {
            int a = parseAddr(), r = parseReg();
            int v = rd(a) | (rd(a + 1) << 8);
            setReg(r, v);
        } else if (op == "movRegRam") {
            int r = parseReg(), a = parseAddr();
            wr(a, reg[r] & 0xFF);
        } else if (op == "movRegRam16") {
            int r = parseReg(), a = parseAddr();
            uint16_t v = reg[r];
            wr(a, v & 0xFF);
            wr(a + 1, v >> 8);
        } else if (op == "movRamRam") {
            int a = parseAddr(), b = parseAddr();
            wr(b, rd(a));
        } else if (op == "movRamRam16") {
            int a = parseAddr(), b = parseAddr();
            uint8_t lo = rd(a), hi = rd(a + 1);
            wr(b, lo);
            wr(b + 1, hi);
        } else if (op == "setRam8") {
            int a = parseAddr(), v = parseValue();
            wr(a, v & 0xFF);
        } else if (op == "setRam16") {
            int a = parseAddr(), v = parseValue();
            wr(a, v & 0xFF);
            wr(a + 1, (v >> 8) & 0xFF);
        } else if (op == "increment") {
            int a = parseReg();
            setReg(dest(a), reg[a] + 1);
        } else if (op == "addNoC" || op == "rm8") {
            int a = parseReg();
            int b = parseRegOrValue();
            int d = dest(a);
            setReg(d, op == "addNoC" ? reg[a] + b : reg[a] - b);
        } else if (op == "addReg8") {
            int a = parseReg();
            int b = parseRegOrValue();
            int d = dest(a);
            int s = (reg[a] & 0xFF) + (b & 0xFF) + reg[LACI];
            setReg(d, s & 0xFF);
            setReg(LACO, s > 0xFF);
        } else if (op == "addReg16") {
            int a = parseReg();
            int b = parseRegOrValue();
            int d = dest(a);
            int s = reg[a] + (b & 0xFFFF) + reg[LACI];
            setReg(d, s & 0xFFFF);
            setReg(LACO, s > 0xFFFF);
        } else if (op == "mut8") {
            int a = parseReg(), b = parseReg();
            int d = dest(a);
            setReg(d, (reg[a] & 0xFF) * (reg[b] & 0xFF));
        } else if (op == "neg8" || op == "neg16" || op == "neg1") {
            int a = parseReg();
            int d = dest(a);
            int v = op == "neg8" ? ~reg[a] & 0xFF : op == "neg16" ? ~reg[a] & 0xFFFF : reg[a] == 0;
            setReg(d, v);
        } else if (op == "and8" || op == "or8" || op == "xorReg") {
            int a = parseReg();
            int b = parseRegOrValue();
            int d = dest(a);
            int v = op == "and8" ? reg[a] & b : op == "or8" ? reg[a] | b : reg[a] ^ b;
            setReg(d, v & 0xFF);
        } else if (op == "setBySign8") {
            int a = parseReg(), b = parseReg();
            setReg(b, (reg[a] & 0x80) ? 0xFF : 0x00);
        } else if (op == "cmpEqual") {
            int a = parseReg();
            setReg(LC, (reg[a] & 0xFF) == 0);
        } else if (op == "cmpSign") {
            int a = parseReg();
            setReg(LC, (reg[a] & 0x80) != 0);
        } else if (op == "shiftL1") {
            int a = parseReg(), b = parseReg();
            int v = reg[a];
            setReg(LSHIFT, v & 1);
            setReg(b, (reg[b] & 0x80) | ((v & 0xFF) >> 1));
        } else if (op == "putTo") {
            int a = parseReg();
            setReg(a, (reg[a] & 0xFF7F) | (reg[LSHIFT] << 7));
        } else if (op == "jump") {
            if (!more()) fail("skok bez cile (nedoplneny jump)");
            else nextPc = parseValue();
        } else if (op == "jumpReg") {
            nextPc = reg[parseReg()];
        } else {
            fail("instrukce '" + op + "' neni v simulatoru (zatim) podporovana");
        }

        if (!bad && more()) fail("prebyvajici operand '" + tok[pos] + "'");
        return !bad;
    }
};

} // namespace sim
