#pragma once
// Referencni simulator ISA tak, jak ji pouziva kompilator (syntax.cpp + memoryManagment/*).
// Spousti textove instrukce z fullCompiledCode::code vcetne skoku a zaznamenava RAM, registry a SP.
//
// PREDPOKLADY (pokud neco nesedi s tvym CPU, oprav to tady - vsechny testy se tim ridi):
//   - registry R0..R5 jsou 16bitove; R0 je jen pro cteni; R3 = stack pointer, R4 = pomocny pro vypocty s SP, R5 = adresa skoku
//   - registr na miste RAM operandu = neprima adresa, napr. movRamRam R1 R2 kopiruje RAM[R1] -> RAM[R2]
//   - mov*: 1. operand odkud, 2. operand kam;  set*: 1. operand kam, 2. operand co
//   - add8 A B [C]: C = A + B (B je registr nebo cislo, bez C se vysledek zapise do A); pocita 16bitove, protoze se tim pocitaji adresy
//   - rm8 A B [C]: totez s odcitanim
//   - increment Rx: Rx = Rx + 1 (16 bit, primo v registru)
//   - jump N / jumpReg Rx: absolutni skok na index instrukce (zatim radky, bez *5); konec programu = skok za posledni instrukci
//   - *16 instrukce: little endian (dolni bajt na nizsi adrese)
//   - adresa 0 je rezervovana (ramOutOkBit), pristup na ni je chyba
#include <array>
#include <cstdint>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace sim {

struct RamWrite {
    uint16_t addr;
    uint8_t before;
    uint8_t after;
};

struct TraceRow {
    int pc;
    std::array<uint16_t, 6> regs;  // po provedeni
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
    std::array<uint16_t, 6> reg{};
    std::vector<Error> errors;
    std::vector<TraceRow> trace;
    bool recordTrace = false;
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
            if (!exec(pc, code[pc], next)) return false;
            if (recordTrace) trace.push_back({pc, reg, curWrites});
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

    static bool isInt(const std::string &s) {
        if (s.empty()) return false;
        size_t i = (s[0] == '-') ? 1 : 0;
        if (i >= s.size()) return false;
        for (; i < s.size(); i++)
            if (s[i] < '0' || s[i] > '9') return false;
        return true;
    }

    static int regOf(const std::string &t) {
        if (t.size() == 2 && t[0] == 'R' && t[1] >= '0' && t[1] <= '5') return t[1] - '0';
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
        if (r == 0) {
            fail("zapis do R0 (je jen pro cteni)");
            return;
        }
        if (bad) return;
        reg[r] = v & 0xFFFF;
        regsWritten.insert(r);
    }

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
        } else if (op == "movRegReg" || op == "movRegReg16") {
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
            int r = parseReg();
            setReg(r, reg[r] + 1);
        } else if (op == "add8" || op == "rm8") {
            int a = parseReg();
            int b = parseRegOrValue();
            int dst = a;
            if (pos < tok.size()) dst = parseReg();
            int v = op == "add8" ? reg[a] + b : reg[a] - b;
            setReg(dst, v);
        } else if (op == "jump") {
            nextPc = parseValue();
        } else if (op == "jumpReg") {
            nextPc = reg[parseReg()];
        } else {
            fail("instrukce '" + op + "' neni v simulatoru (zatim) podporovana");
        }

        if (!bad && pos < tok.size()) fail("prebyvajici operand '" + tok[pos] + "'");
        return !bad;
    }
};

} // namespace sim
