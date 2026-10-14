// Testy pro copyFromTo (memoryManagment/memoryOperations.cpp).
// Vygenerovany kod se spusti v simulatoru (sim.hpp) a porovna se cela RAM s ocekavanym stavem.
// Vystup: JSON s vysledky (pro report.py) na stdout nebo do souboru z argv[1].
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include "adapter.hpp"
#include "sim.hpp"

using adapter::Side;

// ---------------------------------------------------------------- JSON

static std::string esc(const std::string &s) {
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
static std::string arr(const std::vector<T> &v, F f) {
    std::string o = "[";
    for (size_t i = 0; i < v.size(); i++) o += (i ? "," : "") + f(v[i]);
    return o + "]";
}

// ---------------------------------------------------------------- kombinace

struct Path {
    bool fromPtr, fromR1, toPtr, toR1;

    std::string id() const {
        return std::string(fromPtr ? "P" : "V") + (fromR1 ? "r" : "m") + "-" + (toPtr ? "P" : "V") + (toR1 ? "r" : "m");
    }
    // ktery ze 4 semantickych pripadu (komentar v copyFromTo)
    std::string kind() const {
        if (fromPtr && toPtr) return "ano a ano";
        if (!fromPtr && toPtr) return "ne a ano";
        if (fromPtr) return "ano a ne";
        return "ne a ne";
    }
    std::string where() const {
        if (fromR1 && toR1) return "obě v R1";
        if (fromR1) return "z R1";
        if (toR1) return "do R1";
        return "RAM → RAM";
    }
};

static std::vector<Path> allPaths() {
    std::vector<Path> v;
    for (int fp = 0; fp < 2; fp++)
        for (int tp = 0; tp < 2; tp++)
            for (int loc = 0; loc < 4; loc++)
                v.push_back({(bool)fp, (bool)(loc & 1), (bool)tp, (bool)(loc & 2)});
    // poradi: "ne a ne", "ano a ne", "ne a ano", "ano a ano"; uvnitr RAM, z R1, do R1, obe
    std::stable_sort(v.begin(), v.end(), [](const Path &a, const Path &b) {
        auto k = [](const Path &p) { return p.fromPtr * 1 + p.toPtr * 2; };
        auto l = [](const Path &p) { return p.fromR1 * 1 + p.toR1 * 2; };
        return std::make_pair(k(a), l(a)) < std::make_pair(k(b), l(b));
    });
    return v;
}

// ---------------------------------------------------------------- jeden test

struct Layout {
    std::string fromSeg;
    int fromOff;
    std::string toSeg;
    int toOff;
    std::string dataSeg;  // kam ukazuje pointer ve "from" (kdyz fromPtr)
    int dataOff;
};

struct Diff {
    int addr;
    int expected;
    int actual;
    bool inTarget;
};

struct Result {
    std::string group, pathId, placement, status, note;
    int fromSize = 0, toSize = 0;
    std::vector<std::string> code;
    std::vector<sim::Error> errors;
    std::vector<Diff> diffs;
    int diffCount = 0;
    std::set<int> regsWritten;
    std::vector<sim::TraceRow> trace;
    std::array<uint16_t, 6> regsBefore{};
    std::string layoutText;
};

static sim::Config cfg;

static int absAddr(const std::string &seg, int off) { return cfg.segmentBase.at(seg) + off; }

static std::string hex(int v, int w = 4) {
    char b[16];
    snprintf(b, sizeof b, "0x%0*X", w, v);
    return b;
}

// zeroExtend: ocekavani pro rozdilne velikosti (dolni bajty zkopirovat, zbytek 0)
static Result runCase(const std::string &group, const Path &p, const Layout &L, int fromSize, int toSize,
                      uint32_t seed, bool withTrace, bool zeroExtend = false) {
    Result r;
    r.group = group;
    r.pathId = p.id();
    r.fromSize = fromSize;
    r.toSize = toSize;

    sim::Machine m;
    m.cfg = cfg;
    std::mt19937 rng(seed);
    for (auto &b : m.ram) b = rng() & 0xFF;
    for (auto &x : m.reg) x = rng() & 0xFFFF;

    int fromSlot = absAddr(L.fromSeg, L.fromOff);
    int toSlot = absAddr(L.toSeg, L.toOff);
    int data = absAddr(L.dataSeg, L.dataOff);
    if (p.fromPtr) {
        m.ram[fromSlot] = data & 0xFF;
        m.ram[fromSlot + 1] = data >> 8;
    }
    if (p.fromR1) m.reg[1] = fromSlot;
    if (p.toR1) m.reg[1] = toSlot;
    r.regsBefore = m.reg;

    r.layoutText = "z: " + L.fromSeg + "+" + std::to_string(L.fromOff) + " (" + hex(fromSlot) + ")";
    if (p.fromPtr) r.layoutText += " -> data " + L.dataSeg + "+" + std::to_string(L.dataOff) + " (" + hex(data) + ")";
    r.layoutText += ", do: " + L.toSeg + "+" + std::to_string(L.toOff) + " (" + hex(toSlot) + ")";
    if (p.fromR1) r.layoutText += ", R1 = adresa z";
    if (p.toR1) r.layoutText += ", R1 = adresa do";

    // ocekavany stav RAM
    auto expected = m.ram;
    int tStart = toSlot, tLen;
    if (p.toPtr) {
        int val = p.fromPtr ? (m.ram[fromSlot] | (m.ram[fromSlot + 1] << 8)) : fromSlot;
        expected[toSlot] = val & 0xFF;
        expected[toSlot + 1] = val >> 8;
        tLen = 2;
    } else {
        int src = p.fromPtr ? data : fromSlot;
        for (int i = 0; i < toSize; i++) {
            if (zeroExtend && i >= fromSize) expected[toSlot + i] = 0;
            else expected[toSlot + i] = m.ram[src + i];
        }
        tLen = toSize;
    }

    Side f{p.fromR1, L.fromSeg, p.fromR1 ? -1 : L.fromOff, fromSize, p.fromPtr};
    Side t{p.toR1, L.toSeg, p.toR1 ? -1 : L.toOff, toSize, p.toPtr};
    r.code = adapter::generateCopy(f, t);

    m.recordTrace = withTrace;
    m.run(r.code);
    r.errors = m.errors;
    r.regsWritten = m.regsWritten;
    r.trace = m.trace;

    for (int a = 0; a < 65536; a++) {
        if (m.ram[a] != expected[a]) {
            r.diffCount++;
            if (r.diffs.size() < 24)
                r.diffs.push_back({a, expected[a], m.ram[a], a >= tStart && a < tStart + tLen});
        }
    }

    if (!r.errors.empty() || r.diffCount > 0) r.status = "fail";
    else r.status = "pass";
    if (r.code.empty()) r.note = "nevygenerována žádná instrukce";
    return r;
}

// ---------------------------------------------------------------- serializace

static std::string resultJson(const Result &r) {
    std::string o = "{";
    o += "\"group\":" + esc(r.group) + ",\"path\":" + esc(r.pathId) + ",\"placement\":" + esc(r.placement);
    o += ",\"status\":" + esc(r.status) + ",\"note\":" + esc(r.note);
    o += ",\"fromSize\":" + std::to_string(r.fromSize) + ",\"toSize\":" + std::to_string(r.toSize);
    o += ",\"layout\":" + esc(r.layoutText);
    o += ",\"code\":" + arr(r.code, esc);
    o += ",\"errors\":" + arr(r.errors, [](const sim::Error &e) {
        return "{\"line\":" + std::to_string(e.line) + ",\"text\":" + esc(e.text) + ",\"msg\":" + esc(e.msg) + "}";
    });
    o += ",\"diffCount\":" + std::to_string(r.diffCount);
    o += ",\"diffs\":" + arr(r.diffs, [](const Diff &d) {
        return "{\"addr\":" + std::to_string(d.addr) + ",\"exp\":" + std::to_string(d.expected) +
               ",\"act\":" + std::to_string(d.actual) + ",\"inTarget\":" + (d.inTarget ? "true" : "false") + "}";
    });
    std::vector<int> rw(r.regsWritten.begin(), r.regsWritten.end());
    o += ",\"regsWritten\":" + arr(rw, [](int x) { return std::to_string(x); });
    std::vector<uint16_t> rb(r.regsBefore.begin(), r.regsBefore.end());
    o += ",\"regsBefore\":" + arr(rb, [](uint16_t x) { return std::to_string(x); });
    o += ",\"trace\":" + arr(r.trace, [](const sim::TraceRow &t) {
        std::vector<uint16_t> rg(t.regs.begin(), t.regs.end());
        return "{\"line\":" + std::to_string(t.line) + ",\"text\":" + esc(t.text) +
               ",\"regs\":" + arr(rg, [](uint16_t x) { return std::to_string(x); }) +
               ",\"writes\":" + arr(t.writes, [](const sim::RamWrite &w) {
                   return "[" + std::to_string(w.addr) + "," + std::to_string(w.before) + "," + std::to_string(w.after) + "]";
               }) + "}";
    });
    return o + "}";
}

// ---------------------------------------------------------------- main

int main(int argc, char **argv) {
    std::vector<Path> paths = allPaths();
    std::vector<Result> results;

    struct Placement {
        std::string name;
        Layout L;
    };
    std::vector<Placement> placements = {
        {"stejný segment", {"static", 0x10, "static", 0x80, "static", 0x200}},
        {"dynamic → static", {"dynamic", 0x10, "static", 0x80, "dynamic", 0x300}},
        // cil, slot i data lezi tak, ze kopirovani prechazi pres hranici xxFF -> xx00
        {"přes hranici 0xFF", {"dynamic", 0xFD, "static", 0xFE, "static", 0x1FE}},
    };
    std::vector<int> sizes = {1, 2, 3, 4, 8, 16};

    uint32_t seed = 1;
    // 1) matice: vsechny cesty x umisteni x velikosti
    for (auto &p : paths) {
        for (auto &pl : placements) {
            for (int s : sizes) {
                bool trace = (pl.name == "stejný segment" && s == 2);
                Result r = runCase("matice", p, pl.L, s, s, seed++, trace);
                r.placement = pl.name;
                if (p.fromR1 && p.toR1 && r.code.empty()) {
                    r.status = "warn";
                    r.note = "obě strany v R1: přiřazení se potichu zahodí (žádná instrukce, žádná chyba)";
                }
                results.push_back(r);
            }
        }
    }

    // 2) nedefinovane pointery (-1): ocekavame, ze se nevygeneruje nic
    for (int which = 0; which < 2; which++) {
        Path p{false, false, false, false};
        Side f{false, "static", which == 0 ? -1 : 0x10, 2, false};
        Side t{false, "static", which == 1 ? -1 : 0x80, 2, false};
        Result r;
        r.group = "nedefinovane";
        r.pathId = p.id();
        r.placement = which == 0 ? "z = -1" : "do = -1";
        r.fromSize = r.toSize = 2;
        r.code = adapter::generateCopy(f, t);
        r.status = r.code.empty() ? "pass" : "fail";
        r.note = r.code.empty() ? "správně nic nevygenerováno" : "vygenerován kód pro nedefinovaný pointer";
        r.layoutText = r.placement;
        results.push_back(r);
    }

    // 3) rozdilne velikosti (otazka semantiky: ocekavame zero-extend / orez na dolni bajty)
    std::vector<std::pair<int, int>> mixes = {{1, 2}, {1, 4}, {2, 4}, {4, 2}, {2, 1}};
    for (auto &p : paths) {
        if (p.toPtr || (p.fromR1 && p.toR1)) continue;
        for (auto [fs, ts] : mixes) {
            Result r = runCase("velikosti", p, placements[0].L, fs, ts, seed++, false, true);
            r.placement = std::to_string(fs) + "B → " + std::to_string(ts) + "B";
            if (r.status == "fail" && r.errors.empty()) {
                r.status = "warn";
                r.note = "cíl je větší než zdroj: horní bajty se nevynulují, zkopíruje se, co leží za zdrojem";
            }
            results.push_back(r);
        }
    }

    // 4) fuzz: nahodna umisteni a velikosti
    std::mt19937 rng(12345);
    const int FUZZ = 300;
    std::string fuzzJson = "[";
    int fuzzFails = 0;
    for (size_t pi = 0; pi < paths.size(); pi++) {
        const Path &p = paths[pi];
        int pass = 0, fail = 0, warn = 0;
        Result firstFail;
        bool haveFail = false;
        for (int i = 0; i < FUZZ; i++) {
            const char *segs[2] = {"static", "dynamic"};
            int s = 1 + rng() % 32;
            Layout L;
            // tri nepřekryvajici se okna po 0x400 bajtech, poradi nahodne
            int win[3] = {0, 0x400, 0x800};
            std::shuffle(win, win + 3, rng);
            L.fromSeg = segs[rng() % 2];
            L.toSeg = segs[rng() % 2];
            L.dataSeg = segs[rng() % 2];
            L.fromOff = win[0] + rng() % 0x3C0;
            L.toOff = win[1] + rng() % 0x3C0;
            L.dataOff = win[2] + rng() % 0x3C0;
            // ruzne segmenty muzou mit stejne okno jen kdyz jsou v jinem segmentu - to je v poradku
            Result r = runCase("fuzz", p, L, s, s, seed++, false);
            if (p.fromR1 && p.toR1 && r.code.empty()) warn++;
            else if (r.status == "pass") pass++;
            else {
                fail++;
                if (!haveFail) firstFail = r, haveFail = true;
            }
        }
        fuzzFails += fail;
        if (pi) fuzzJson += ",";
        fuzzJson += "{\"path\":" + esc(p.id()) + ",\"runs\":" + std::to_string(FUZZ) + ",\"pass\":" + std::to_string(pass) +
                    ",\"fail\":" + std::to_string(fail) + ",\"warn\":" + std::to_string(warn);
        if (haveFail) {
            firstFail.placement = "fuzz";
            fuzzJson += ",\"firstFail\":" + resultJson(firstFail);
        }
        fuzzJson += "}";
    }
    fuzzJson += "]";

    // 5) pocet instrukci podle velikosti
    std::string countJson = "[";
    for (size_t pi = 0; pi < paths.size(); pi++) {
        const Path &p = paths[pi];
        Layout L = placements[0].L;
        std::vector<int> counts;
        for (int s = 1; s <= 16; s++) {
            Side f{p.fromR1, L.fromSeg, p.fromR1 ? -1 : L.fromOff, s, p.fromPtr};
            Side t{p.toR1, L.toSeg, p.toR1 ? -1 : L.toOff, s, p.toPtr};
            counts.push_back((int)adapter::generateCopy(f, t).size());
        }
        countJson += std::string(pi ? "," : "") + "{\"path\":" + esc(p.id()) + ",\"counts\":" +
                     arr(counts, [](int x) { return std::to_string(x); }) + "}";
    }
    countJson += "]";

    std::string o = "{\"suite\":\"copyFromTo\"";
    o += ",\"segments\":{";
    bool first = true;
    for (auto &[k, v] : cfg.segmentBase) {
        o += std::string(first ? "" : ",") + esc(k) + ":" + std::to_string(v);
        first = false;
    }
    o += "}";
    o += ",\"paths\":" + arr(paths, [](const Path &p) {
        return "{\"id\":" + esc(p.id()) + ",\"kind\":" + esc(p.kind()) + ",\"where\":" + esc(p.where()) +
               ",\"fromPtr\":" + (p.fromPtr ? "true" : "false") + ",\"toPtr\":" + (p.toPtr ? "true" : "false") +
               ",\"fromR1\":" + (p.fromR1 ? "true" : "false") + ",\"toR1\":" + (p.toR1 ? "true" : "false") + "}";
    });
    o += ",\"results\":" + arr(results, resultJson);
    o += ",\"fuzz\":" + fuzzJson;
    o += ",\"counts\":" + countJson;
    o += "}";

    int fails = 0;
    for (auto &r : results) fails += r.status == "fail";
    if (argc > 1) {
        std::ofstream(argv[1]) << o;
    } else {
        std::cout << o;
    }
    std::cerr << "copyFromTo: " << results.size() << " testu, " << fails << " selhalo, fuzz selhani: " << fuzzFails << "\n";
    return (fails || fuzzFails) ? 1 : 0;
}
