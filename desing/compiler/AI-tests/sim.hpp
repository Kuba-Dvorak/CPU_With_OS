#pragma once
// Referencni simulator ISA tak, jak ji pouziva kompilator (syntax.cpp).
// Spousti textove instrukce z fullCompiledCode::code a zaznamenava, co delaji s RAM a registry.
//
// PREDPOKLADY (pokud neco nesedi s tvym CPU, oprav to tady - vsechny testy se tim ridi):
//   - registry R0..R5 jsou 16bitove, R0 je jen pro cteni (zapisuje do nej jen ALU / increment)
//   - "Ram1" / "Ram2" jako adresa = RAM[R1] / RAM[R2]
//   - "<segment> <offset>" jako adresa = baze segmentu + offset (bazi dosadi linker, tady Config)
//   - mov*: 1. operand odkud, 2. operand kam;  set*: 1. operand kam, 2. operand co
//   - *16 instrukce: little endian (dolni bajt na nizsi adrese)
//   - increment Rx: R0 = Rx + 1 (16 bit), Rx se nemeni
//   - adresa 0 je rezervovana (ramOutOkBit), pristup na ni je chyba
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace sim {

struct Config {
    std::map<std::string, int> segmentBase = {{"static", 0x4000}, {"dynamic", 0x8000}};
    int maxSteps = 100000;
};

struct RamWrite {
    uint16_t addr;
    uint8_t before;
    uint8_t after;
};

struct TraceRow {
    int line;
    std::string text;
    std::array<uint16_t, 6> regs;
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
    Config cfg;
    std::vector<Error> errors;
    std::vector<TraceRow> trace;
    bool recordTrace = false;
    std::set<int> regsWritten;
    int steps = 0;

    uint16_t read16(int addr) const { return ram[addr & 0xFFFF] | (ram[(addr + 1) & 0xFFFF] << 8); }

    // vrati false pri chybe (chyba je zapsana do errors)
    bool run(const std::vector<std::string> &code) {
        for (int line = 0; line < (int)code.size(); line++) {
            if (++steps > cfg.maxSteps) {
                fail(line, code[line], "prekrocen limit kroku");
                return false;
            }
            curWrites.clear();
            if (!exec(line, code[line])) return false;
            if (recordTrace) trace.push_back({line, code[line], reg, curWrites});
        }
        return true;
    }

private:
    std::vector<RamWrite> curWrites;
    std::vector<std::string> tok;
    size_t pos = 0;
    int curLine = 0;
    std::string curText;
    bool bad = false;

    void fail(int line, const std::string &text, const std::string &msg) {
        errors.push_back({line, text, msg});
        bad = true;
    }
    void fail(const std::string &msg) { fail(curLine, curText, msg); }

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

    int regIndex(const std::string &t) {
        if (t.size() == 2 && t[0] == 'R' && t[1] >= '0' && t[1] <= '5') return t[1] - '0';
        fail("ocekavan registr, nalezeno '" + t + "'");
        return 0;
    }

    int parseReg() { return regIndex(next()); }

    int parseAddr() {
        std::string t = next();
        if (t == "Ram1") return reg[1];
        if (t == "Ram2") return reg[2];
        auto seg = cfg.segmentBase.find(t);
        if (seg != cfg.segmentBase.end()) {
            std::string off = next();
            if (!isInt(off)) {
                fail("za segmentem '" + t + "' chybi offset, nalezeno '" + off + "'");
                return 1;
            }
            int a = seg->second + std::stoi(off);
            if (a < 0 || a > 0xFFFF) fail("adresa mimo RAM: " + t + " " + off);
            return a & 0xFFFF;
        }
        if (isInt(t)) return std::stoi(t) & 0xFFFF;
        fail("neznamy adresni operand '" + t + "'");
        return 1;
    }

    // hodnota: cislo, nebo "<segment> <offset>" = adresa (pouziva se pro ukladani pointeru)
    int parseValue() {
        std::string t = next();
        if (cfg.segmentBase.count(t)) {
            pos--;
            return parseAddr();
        }
        if (isInt(t)) return std::stoi(t);
        fail("neznama hodnota '" + t + "'");
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
        curWrites.push_back({(uint16_t)(a & 0xFFFF), ram[a & 0xFFFF], v});
        ram[a & 0xFFFF] = v;
    }

    void setReg(int r, uint16_t v) {
        if (r == 0) {
            fail("zapis do R0 (je jen pro cteni)");
            return;
        }
        reg[r] = v;
        regsWritten.insert(r);
    }

    bool exec(int line, const std::string &text) {
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
            if (!bad) setReg(r, v);
        } else if (op == "movRegReg" || op == "movRegReg16") {
            int a = parseReg(), b = parseReg();
            if (!bad) setReg(b, reg[a]);
        } else if (op == "movRamReg") {
            int a = parseAddr(), r = parseReg();
            if (!bad) setReg(r, rd(a));
        } else if (op == "movRamReg16") {
            int a = parseAddr(), r = parseReg();
            if (!bad) setReg(r, rd(a) | (rd(a + 1) << 8));
        } else if (op == "movRegRam") {
            int r = parseReg(), a = parseAddr();
            if (!bad) wr(a, reg[r] & 0xFF);
        } else if (op == "movRegRam16") {
            int r = parseReg(), a = parseAddr();
            if (!bad) {
                wr(a, reg[r] & 0xFF);
                wr(a + 1, reg[r] >> 8);
            }
        } else if (op == "movRamRam") {
            int a = parseAddr(), b = parseAddr();
            if (!bad) wr(b, rd(a));
        } else if (op == "movRamRam16") {
            int a = parseAddr(), b = parseAddr();
            if (!bad) {
                uint8_t lo = rd(a), hi = rd(a + 1);
                wr(b, lo);
                wr(b + 1, hi);
            }
        } else if (op == "setRam8") {
            int a = parseAddr(), v = parseValue();
            if (!bad) wr(a, v & 0xFF);
        } else if (op == "setRam16") {
            int a = parseAddr(), v = parseValue();
            if (!bad) {
                wr(a, v & 0xFF);
                wr(a + 1, (v >> 8) & 0xFF);
            }
        } else if (op == "increment") {
            int r = parseReg();
            if (!bad) {
                reg[0] = (reg[r] + 1) & 0xFFFF;
                regsWritten.insert(0);
            }
        } else {
            fail("instrukce '" + op + "' neni v simulatoru (zatim) podporovana");
        }

        if (!bad && pos < tok.size()) fail("prebyvajici operand '" + tok[pos] + "'");
        return !bad;
    }
};

} // namespace sim
