// TESTOVACI NAHRADA za lexicalAnalyzer.cpp (ten je zatim prazdny).
// run.sh ji pri sestaveni testu podstrci do kopie stromu v build/stage/ misto lexicalAnalyzer.cpp,
// tvuj soubor se nemeni. Az lexer napises, ./run.sh --lexer pusti testy s tvym.
//
// Drzi se kontraktu z komentaru v lexicalAnalyzer.cpp:
//   - kazdy finder zacina na startPosition (mezery preskoci); kdyz tam vec neni, exists = false
//   - position = prvni znak ZA nactenou veci
// Rozhodnuti, ktera z komentaru nevyplyvaji (oznacena "MOCK:"), jsou jen odhad.
#include "./lexicalSyntax.cpp"
#include <cctype>

syntaxDefiners lexSyntax;
syntaxForBT btSyntax;

std::string loadOneLine(std::string &origCode) { return ""; }

std::vector<syntaxDefiners> loadNodes() { return {}; }

static int mockSkip(const std::string &s, int p) {
    while (p >= 0 && p < (int)s.size() && s[p] == ' ') p++;
    return p;
}

static bool mockStarts(const std::string &s, int p, const std::string &what) {
    return p >= 0 && p + what.size() <= s.size() && s.compare(p, what.size(), what) == 0;
}

static bool mockNameStart(char c) { return std::isalpha((unsigned char)c) || c == '_'; }
static bool mockNameChar(char c) { return std::isalnum((unsigned char)c) || c == '_'; }

std::string loadName(std::string &lineCode, int startPosition) {
    int p = mockSkip(lineCode, startPosition);
    std::string n;
    if (p >= (int)lineCode.size() || !mockNameStart(lineCode[p])) return n;
    // MOCK: jmeno konci i pred koncem volani (napr. "a_/"), protoze "_" muze byt ve jmene
    while (p < (int)lineCode.size() && mockNameChar(lineCode[p]) && !mockStarts(lineCode, p, btSyntax.functionCallEnd))
        n += lineCode[p++];
    return n;
}

static goodConstructStart mockSymbol(std::string &lineCode, int startPosition, const std::string &sym) {
    int p = mockSkip(lineCode, startPosition);
    if (mockStarts(lineCode, p, sym)) return {p + (int)sym.size(), true};
    return {startPosition, false};
}

goodConstructStart findConstructStart(std::string &lineCode, int startPosition) {
    return mockSymbol(lineCode, startPosition, btSyntax.typeConstrStart);
}

goodConstructStart findConstructEnd(std::string &lineCode, int startPosition) {
    return mockSymbol(lineCode, startPosition, btSyntax.typeConstrEnd);
}

// promena: jmeno [.keys]; MOCK: hleda se v varList odzadu bez vypisu, "call" neni promenna
goodVar findVar(std::string &lineCode, int startPosition, varHandle &vars) {
    int p = mockSkip(lineCode, startPosition);
    std::string name = loadName(lineCode, p);
    if (name.empty() || name == lexSyntax.callFunq) return {nullptr, {}, startPosition, false};
    variable *v = nullptr;
    for (int i = (int)vars.varList.size() - 1; i >= 0; i--)
        if (vars.varList[i].name == name) {
            v = &vars.varList[i];
            break;
        }
    if (!v) return {nullptr, {}, startPosition, false};
    p += name.size();
    std::vector<std::string> keys;
    while (mockStarts(lineCode, p, btSyntax.keyingSymbol)) {
        std::string k = loadName(lineCode, p + 1);
        if (k.empty()) break;
        keys.push_back(k);
        p += 1 + k.size();
    }
    return {v, keys, p, true};
}

// volani: call jmeno /_ args _/; position = za otviracim symbolem, endOfFunq = za odpovidajicim zaviracim
// (hloubka se pocita podle functionCallStart/End z lexicalSyntax.cpp)
goodFunction findFunction(std::string &lineCode, int startPosition, functionHandle &funcs) {
    int p = mockSkip(lineCode, startPosition);
    if (!mockStarts(lineCode, p, lexSyntax.callFunq)) return {nullptr, startPosition, startPosition, false};
    p = mockSkip(lineCode, p + lexSyntax.callFunq.size());
    std::string name = loadName(lineCode, p);
    if (name.empty()) return {nullptr, startPosition, startPosition, false};
    p = mockSkip(lineCode, p + name.size());
    if (!mockStarts(lineCode, p, btSyntax.functionCallStart)) return {nullptr, startPosition, startPosition, false};
    basicFunction *f = nullptr;
    for (auto &fn : funcs.functionList)
        if (fn.name == name) f = &fn;
    if (!f) return {nullptr, startPosition, startPosition, false};
    const std::string &open = btSyntax.functionCallStart, &close = btSyntax.functionCallEnd;
    int start = p + (int)open.size(), depth = 1, e = start;
    while (e < (int)lineCode.size() && depth > 0) {
        if (mockStarts(lineCode, e, close)) {
            depth--;
            e += close.size();
        } else if (mockStarts(lineCode, e, open)) {
            depth++;
            e += open.size();
        } else e++;
    }
    return {f, start, e, depth == 0};
}

goodNumber findNumber(std::string &lineCode, int startPosition) {
    int p = mockSkip(lineCode, startPosition);
    if (p >= (int)lineCode.size() || !std::isdigit((unsigned char)lineCode[p])) return {0, startPosition, false};
    long long v = 0;
    while (p < (int)lineCode.size() && std::isdigit((unsigned char)lineCode[p])) v = v * 10 + (lineCode[p++] - '0');
    return {(int)v, p, true};
}

goodBracket findStartBracket(std::string &lineCode, int startPosition) {
    auto r = mockSymbol(lineCode, startPosition, btSyntax.normalBracketStart);
    return {r.position, r.exists};
}

goodBracket findEndBracket(std::string &lineCode, int startPosition) {
    auto r = mockSymbol(lineCode, startPosition, btSyntax.normalBracketEnd);
    return {r.position, r.exists};
}

// zavorky volani funkce (functionCallStart / functionCallEnd z lexicalSyntax.cpp)
goodBracket findFunqStrBrc(std::string &lineCode, int startPosition) {
    auto r = mockSymbol(lineCode, startPosition, btSyntax.functionCallStart);
    return {r.position, r.exists};
}

goodBracket findFunqEndBrc(std::string &lineCode, int startPosition) {
    auto r = mockSymbol(lineCode, startPosition, btSyntax.functionCallEnd);
    return {r.position, r.exists};
}

// 2znakove pred 1znakovymi; konec/zacatek volani neni operator (napr. "/" vs "/_")
goodOperator findOperator(std::string &lineCode, int startPosition) {
    int p = mockSkip(lineCode, startPosition);
    const std::pair<const std::string *, operators> ops[] = {
        {&btSyntax.equalSign, operators::equal},  {&btSyntax.eqBigSign, operators::ebig},   {&btSyntax.eqSmlSign, operators::esml},
        {&btSyntax.notEqualSign, operators::notEqual}, {&btSyntax.plusSign, operators::plus}, {&btSyntax.minusSign, operators::remove},
        {&btSyntax.timesSign, operators::multiply}, {&btSyntax.divideSign, operators::divade}, {&btSyntax.andSign, operators::andOperator},
        {&btSyntax.orSign, operators::orOperator},  {&btSyntax.bigSign, operators::bigger},   {&btSyntax.smlSign, operators::lesser},
    };
    for (auto &[sym, op] : ops) {
        if (!mockStarts(lineCode, p, *sym)) continue;
        if (mockStarts(lineCode, p, btSyntax.functionCallStart) || mockStarts(lineCode, p, btSyntax.functionCallEnd))
            return {operators::plus, startPosition, false};
        return {op, p + (int)sym->size(), true};
    }
    return {operators::plus, startPosition, false};
}

// MOCK: "!" = notOperBit, "$" = isPointer (notOperByte zatim nema v syntaxi znak)
goodSingleOperator findSingleOperator(std::string &lineCode, int startPosition) {
    int p = mockSkip(lineCode, startPosition);
    if (mockStarts(lineCode, p, btSyntax.notSign)) return {singleOperator::notOperBit, p + 1};
    if (mockStarts(lineCode, p, btSyntax.isPointerSign)) return {singleOperator::isPointer, p + 1};
    return {singleOperator::noSingleOper, startPosition};
}

goodSeparator findSeparator(std::string &lineCode, int startPosition) {
    auto r = mockSymbol(lineCode, startPosition, btSyntax.nextSign);
    return {r.position, r.exists};
}
