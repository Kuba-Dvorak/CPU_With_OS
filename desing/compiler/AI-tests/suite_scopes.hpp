#pragma once
// Sada "scopes": prideleni pameti v ramci (segmentManager) a zivotnost promennych ve scopech.
// Nic se nespousti, kontroluji se offsety, ktere kompilator rozda, a co najde returnVar.
#include "adapter.hpp"
#include "common.hpp"

namespace suite_scopes {

inline std::vector<Result> runAll() {
    std::vector<Result> out;
    auto check = [&](std::string name, bool ok, std::string detail, bool onlyWarn = false) {
        Result r;
        r.suite = "scopes";
        r.group = "scopy";
        r.row = name;
        r.layout = detail;
        if (!ok) {
            if (onlyWarn) r.warn(name);
            else r.fail(name);
        }
        out.push_back(r);
    };

    adapter::World w;
    adapter::defineNumber(w, 2);
    adapter::Frame f;
    adapter::openScope(f);

    int x = adapter::allocVar(f, 2, "x");
    adapter::addVar(w, "x", "number", x, false);
    int y = adapter::allocVar(f, 4, "y");
    adapter::addVar(w, "y", "number", y, false);
    check("proměnné v jednom scopu jdou za sebou", x == 0 && y == 2, "x = SP+" + num(x) + ", y = SP+" + num(y) + " (čekáno 0 a 2)");

    adapter::openScope(f);
    int z = adapter::allocVar(f, 2, "z");
    adapter::addVar(w, "z", "number", z, false);
    check("vnořený scope začíná za proměnnými rodiče", z == 6, "z = SP+" + num(z) + " (čekáno 6)");
    adapter::closeScope(f, w);
    check("po konci scopu jsou jeho proměnné pryč", adapter::findVar(w, "z") == nullptr, "returnVar(\"z\") po konci scopu");
    check("proměnné rodiče po konci vnořeného scopu zůstanou", adapter::findVar(w, "x") && adapter::findVar(w, "y"), "returnVar(\"x\"), returnVar(\"y\")");

    adapter::openScope(f);
    int s2 = adapter::allocVar(f, 2, "s2");
    adapter::addVar(w, "s2", "number", s2, false);
    check("sourozenecký scope znovu použije stejnou paměť", s2 == z, "s2 = SP+" + num(s2) + ", z bylo SP+" + num(z));
    adapter::closeScope(f, w);

    int v = adapter::allocVar(f, 2, "v");
    adapter::addVar(w, "v", "number", v, false);
    check("po konci scopu pokračuje rodič na svém místě", v == 6, "v = SP+" + num(v) + " (čekáno 6)");

    adapter::openScope(f);
    int xi = adapter::allocVar(f, 2, "x");
    adapter::addVar(w, "x", "number", xi, false);
    variable *found = adapter::findVar(w, "x");
    check("zastínění: najde se nejvnitřnější proměnná", found && found->localPointer0 == xi,
          "vnitřní x = SP+" + num(xi) + ", nalezeno SP+" + (found ? num(found->localPointer0) : std::string("nic")));
    adapter::closeScope(f, w);
    found = adapter::findVar(w, "x");
    check("po konci scopu se vrátí vnější proměnná", found && found->localPointer0 == x,
          "nalezeno SP+" + (found ? num(found->localPointer0) : std::string("nic")) + " (čekáno " + num(x) + ")");

    int t1 = adapter::allocTemp(f, 2);
    int varsEnd = v + 2;
    check("dočasná paměť leží za proměnnými", t1 >= varsEnd, "temp = SP+" + num(t1) + ", proměnné končí na SP+" + num(varsEnd));
    adapter::endLine(f);
    int t2 = adapter::allocTemp(f, 2);
    check("endLine uvolní dočasnou paměť", t2 == t1, "temp po endLine = SP+" + num(t2) + " (čekáno " + num(t1) + ")");

    adapter::openScope(f);
    int t3 = adapter::allocTemp(f, 2);
    check("nový scope začne s prázdnou dočasnou pamětí", t3 == varsEnd, "temp ve novém scopu = SP+" + num(t3) + " (čekáno " + num(varsEnd) + ")");
    adapter::closeScope(f, w);

    // number q = a + b;  -> nejdriv se prideli q, teprve potom se pocita do temp
    // (poradi alokace -> prirazeni je pravidlo kompilatoru, test hlida jen ze se pak neprekryji)
    adapter::endLine(f);
    int q = adapter::allocVar(f, 2, "q");
    int tmp = adapter::allocTemp(f, 2);
    check("dočasná paměť výrazu se nepřekrývá s právě deklarovanou proměnnou", q + 2 <= tmp || tmp + 2 <= q,
          "q = SP+" + num(q) + ", potom temp = SP+" + num(tmp));

    // parametry funkce: inputsInner ve stejnem poradi jako v definici, ramec [vystup][navrat 2 B][parametry]
    {
        adapter::World fw;
        adapter::defineNumber(fw, 2);
        adapter::Function fn;
        fullCompiledCode code;
        adapter::defineFunction(fn, "f", {"pa", "pr", "pb"}, {"number", "number", "number"}, {false, true, false}, "number", fw, code);
        auto &in = adapter::inputs(fn);
        std::string got;
        bool ok = in.size() == 3;
        const char *names[3] = {"pa", "pr", "pb"};
        int offs[3] = {4, 6, 8};
        bool refs[3] = {false, true, false};
        for (size_t i = 0; i < in.size(); i++) {
            got += (i ? ", " : "") + in[i]->name + " = SP+" + num(in[i]->localPointer0) + (in[i]->isRef ? " (ref)" : "");
            if (i < 3) ok = ok && in[i]->name == names[i] && in[i]->localPointer0 == offs[i] && in[i]->isRef == refs[i];
        }
        check("inputsInner: parametry ve správném pořadí a na správných offsetech", ok,
              "nalezeno [" + got + "], čekáno [pa = SP+4, pr = SP+6 (ref), pb = SP+8]");
    }
    return out;
}

} // namespace suite_scopes
