#pragma once
// Spolecne veci pro vsechny sady testu: vysledek testu, porovnani RAM, JSON.
#include <array>
#include <cstdio>
#include <set>
#include <string>
#include <vector>
#include "sim.hpp"

// ---------------------------------------------------------------- JSON

inline std::string esc(const std::string &s) {
    std::string o = "\"";
    for (char c : s) {
        if (c == '"' || c == '\\') o += '\\', o += c;
        else if (c == '\n') o += "\\n";
        else if ((unsigned char)c < 0x20) o += ' ';
        else o += c;
    }
    return o + "\"";
}

template <class T, class F>
std::string arr(const std::vector<T> &v, F f) {
    std::string o = "[";
    for (size_t i = 0; i < v.size(); i++) o += (i ? "," : "") + f(v[i]);
    return o + "]";
}

inline std::string num(long long v) { return std::to_string(v); }
inline std::string boolj(bool b) { return b ? "true" : "false"; }

inline std::string hex(int v, int w = 4) {
    char b[16];
    snprintf(b, sizeof b, "0x%0*X", w, v);
    return b;
}

// ---------------------------------------------------------------- vysledek

struct Diff {
    int addr, expected, actual;
    bool inTarget;
};

struct Result {
    std::string suite, group;  // sada a skupina testu
    std::string row, col;      // kde v matici reportu lezi
    std::string status = "pass";  // pass / warn / fail
    std::string reason;        // kratky duvod (seskupuje se podle nej)
    std::string layout;        // popis rozlozeni pameti
    std::vector<std::string> code;
    std::vector<sim::Error> errors;
    std::vector<Diff> diffs;
    int diffCount = 0;
    std::set<int> regsWritten;
    std::array<uint16_t, 6> regsBefore{};
    std::vector<sim::TraceRow> trace;
    std::string extra = "{}";  // data specificka pro sadu (JSON objekt)

    void fail(const std::string &why) {
        if (status != "fail") reason = why;
        status = "fail";
    }
    void warn(const std::string &why) {
        if (status == "pass") status = "warn", reason = why;
    }
};

// porovna celou RAM; target = rozsahy [od, do), kam se smelo zapisovat
inline void compareRam(Result &r, const sim::Machine &m, const std::array<uint8_t, 65536> &expected,
                       const std::vector<std::pair<int, int>> &target) {
    for (int a = 0; a < 65536; a++) {
        if (m.ram[a] == expected[a]) continue;
        bool in = false;
        for (auto [s, e] : target) in |= (a >= s && a < e);
        r.diffCount++;
        if (r.diffs.size() < 24) r.diffs.push_back({a, expected[a], m.ram[a], in});
    }
    if (r.diffCount) {
        bool outside = false;
        for (auto &d : r.diffs) outside |= !d.inTarget;
        r.fail(outside ? "zapis do RAM mimo cil" : "spatna hodnota v cili");
    }
}

inline void takeRun(Result &r, const sim::Machine &m) {
    r.errors = m.errors;
    r.regsWritten = m.regsWritten;
    r.trace = m.trace;
    if (!m.errors.empty()) r.fail("simulator: " + m.errors[0].msg);
}

inline std::string resultJson(const Result &r) {
    std::string o = "{";
    o += "\"suite\":" + esc(r.suite) + ",\"group\":" + esc(r.group) + ",\"row\":" + esc(r.row) + ",\"col\":" + esc(r.col);
    o += ",\"status\":" + esc(r.status) + ",\"reason\":" + esc(r.reason) + ",\"layout\":" + esc(r.layout);
    o += ",\"code\":" + arr(r.code, esc);
    o += ",\"errors\":" + arr(r.errors, [](const sim::Error &e) {
        return "{\"line\":" + num(e.line) + ",\"text\":" + esc(e.text) + ",\"msg\":" + esc(e.msg) + "}";
    });
    o += ",\"diffCount\":" + num(r.diffCount);
    o += ",\"diffs\":" + arr(r.diffs, [](const Diff &d) {
        return "{\"addr\":" + num(d.addr) + ",\"exp\":" + num(d.expected) + ",\"act\":" + num(d.actual) +
               ",\"inTarget\":" + boolj(d.inTarget) + "}";
    });
    std::vector<int> rw(r.regsWritten.begin(), r.regsWritten.end());
    o += ",\"regsWritten\":" + arr(rw, [](int x) { return num(x); });
    std::vector<int> rb(r.regsBefore.begin(), r.regsBefore.end());
    o += ",\"regsBefore\":" + arr(rb, [](int x) { return num(x); });
    o += ",\"trace\":" + arr(r.trace, [](const sim::TraceRow &t) {
        std::vector<int> rg(t.regs.begin(), t.regs.end());
        return "{\"pc\":" + num(t.pc) + ",\"regs\":" + arr(rg, [](int x) { return num(x); }) +
               ",\"writes\":" + arr(t.writes, [](const sim::RamWrite &w) {
                   return "[" + num(w.addr) + "," + num(w.before) + "," + num(w.after) + "]";
               }) + "}";
    });
    o += ",\"extra\":" + r.extra;
    return o + "}";
}
