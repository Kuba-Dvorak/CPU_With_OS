#pragma once
// Jedine misto, kde se testy dotykaji kodu kompilatoru.
// Kdyz pri prepisu zmenis jmena nebo parametry, uprav jen tenhle soubor.
// Include jde pres -Ibuild/stage (kopie stromu s nahradnim lexerem, viz run.sh; mutate.py podstrci svou kopii).
#include <string>
#include <utility>
#include <vector>
#include "syntaxAnalyzer.cpp"

namespace adapter {

// ---------------------------------------------------------------- copyFromTo

struct Place {
    int size;
    int pointer;  // offset vuci SP, -1 = nedefinovano / je v registru
    bool isRef;   // ve slotu je pointer
    bool inReg;   // adresa slotu je v registru
    int reg;      // 1 nebo 2
};

inline ::place toPlace(const Place &p) {
    return {p.size, p.pointer, p.isRef, p.inReg, p.reg == 1 ? inReg::reg1 : inReg::reg2};
}

// pointing = pointingMode: hodnota -> pointer zapise do cile pointeru misto prepsani pointeru
inline bool copy(const Place &from, const Place &to, fullCompiledCode &code, bool pointing = false) {
    return copyFromTo(toPlace(from), toPlace(to), code, pointing);
}

inline bool setLocal(int number, int localPos, int size, fullCompiledCode &code) {
    return setRamLocal(number, localPos, size, code);
}

// ---------------------------------------------------------------- typy a promenne

struct Field {
    std::string type, key;
    bool isRef;
};

struct World {
    typesHandle types;
    varHandle vars;
    functionHandle funcs;
};

// cisla si findType vytvari samo podle velikosti (posSize); testy pouzivaji vsude cisla o NUMBER_SIZE B
inline int NUMBER_SIZE = 2;
inline void defineNumber(World &w, int size) { NUMBER_SIZE = size; }

inline void defineStruct(World &w, std::string name, const std::vector<Field> &fields) {
    w.types.startCreatingNewType(name);
    for (auto f : fields) w.types.createPartNewType(f.type, f.key, f.isRef, NUMBER_SIZE);
    w.types.stopCreatingNewType();
}

inline int typeSize(World &w, std::string name) { return w.types.findType(name, NUMBER_SIZE)->size; }

inline variable *addVar(World &w, std::string name, std::string type, int localOffset, bool isRef, varHandle *into = nullptr) {
    varHandle &h = into ? *into : w.vars;
    h.addNewVariable(name, w.types.findType(type, NUMBER_SIZE), localOffset, isRef);
    return h.returnVar(name);
}

// zkopiruje promennou (s cestou keys) do whereTo
inline void readVar(variable *v, std::vector<std::string> keys, const Place &whereTo, fullCompiledCode &code, bool pointing = false) {
    ::place p = toPlace(whereTo);
    v->call(keys, code, p, pointing);
}

// zkopiruje whereFrom do promenne (s cestou keys)
inline void writeVar(variable *v, std::vector<std::string> keys, const Place &whereFrom, fullCompiledCode &code, bool pointing = false) {
    ::place p = toPlace(whereFrom);
    v->set(keys, code, p, pointing);
}

// ---------------------------------------------------------------- ramce a funkce

using Frame = segmentManager;

// obycejny scope (ne if / cyklus) zadny kod negeneruje
inline void openScope(Frame &f) {
    fullCompiledCode none;
    f.createNewScope(false, false, none, {});
}
inline int allocVar(Frame &f, int size, std::string name) { return f.reqFreeForVar(size, name); }
inline int allocTemp(Frame &f, int size) { return f.reqFreeSub(size); }
// promenne ramce: telo funkce ma vlastni varHandle (basicFunction::myVarHandlerer), hlavni program w.vars
inline varHandle &varsOf(Frame &f, World &w) {
    for (auto &fn : w.funcs.functionList)
        if (&fn.mySegmentManager == &f) return fn.myVarHandlerer;
    return w.vars;
}
inline void closeScope(Frame &f, World &w) {
    fullCompiledCode none;
    f.stopCurScope(varsOf(f, w), none);
}
inline void endLine(Frame &f) { f.endLine(); }
inline variable *findVar(World &w, std::string name) { return w.vars.returnVar(name); }

struct Function {
    basicFunction *fn = nullptr;  // lezi ve World::funcs, aby ji nasel "call jmeno /_ ... _/"
    std::vector<std::string> paramNames;
    std::vector<bool> paramRefs;
};

inline void defineFunction(Function &f, std::string name, std::vector<std::string> paramNames,
                           std::vector<std::string> paramTypes, std::vector<bool> paramRefs, std::string outType,
                           World &w, fullCompiledCode &code) {
    f.paramNames = paramNames;
    f.paramRefs = paramRefs;
    w.funcs.functionList.emplace_back();
    f.fn = &w.funcs.functionList.back();
    // velikosti cisel jako driv (findType bez velikosti = 2 B)
    std::vector<int> paramSizes(paramTypes.size(), 2);
    f.fn->define(name, paramRefs, paramNames, paramTypes, outType, paramSizes, 2, false, w.types, code);
}

inline Frame &bodyFrame(Function &f) { return f.fn->mySegmentManager; }
inline std::deque<variable *> &inputs(Function &f) { return f.fn->inputsInner; }
inline int startLine(Function &f) { return f.fn->jumpToFunction; }

// offset parametru v ramci funkce (vuci SP uvnitr funkce)
// (jmena parametru jsou v testech unikatni, takze staci projit varHandle vsech funkci)
inline variable *param(World &w, std::string name) {
    for (auto &fn : w.funcs.functionList)
        for (auto &v : fn.myVarHandlerer.varList)
            if (v.name == name) return &v;
    return w.vars.returnVar(name);
}
inline int paramOffset(World &w, std::string name) { return param(w, name)->localPointer0; }

// text argumentu: jmeno[.klic...], u referencniho parametru s "$" (preda se adresa, ne kopie)
inline std::string argText(variable *v, const std::vector<std::string> &keys, bool asPointer) {
    std::string t = (asPointer ? btSyntax.isPointerSign : "") + v->name;
    for (auto &k : keys) t += btSyntax.keyingSymbol + k;
    return t;
}

// cele volani z ramce caller pres skutecny basicNode::evaluateFn:
// sestavi text "call f /_ a, $q, pp.y _/" a nechá ho zpracovat (prepareFnInput -> createParsedSegment -> parseBT).
// Vrati offset vystupu funkce v ramci caller.
inline std::string lastCallText;
inline int callFunction(Frame &caller, Function &f, std::vector<std::pair<variable *, std::vector<std::string>>> args,
                        World &w, fullCompiledCode &code) {
    std::string line = lexSyntax.callFunq + " " + f.fn->name + " " + btSyntax.functionCallStart + " ";
    for (size_t i = 0; i < args.size(); i++)
        line += (i ? btSyntax.nextSign + " " : "") + argText(args[i].first, args[i].second, i < f.paramRefs.size() && f.paramRefs[i]);
    line += " " + btSyntax.functionCallEnd;
    lastCallText = line;
    basicNode node;
    node.myArType = nodeTypes::function;
    node.funq = f.fn;
    node.singleOper = singleOperator::noSingleOper;
    node.nodeID = 0;
    node.textPosition = 0;
    node.textStartPos = (int)(line.find(btSyntax.functionCallStart) + btSyntax.functionCallStart.size());
    place out = node.evaluateFn(code, caller, line, varsOf(caller, w), w.funcs);
    return out.pointer;
}

inline void returnFrom(Function &f, const Place &what, fullCompiledCode &code) { f.fn->getReturnCMD(toPlace(what), code); }

// ---------------------------------------------------------------- vyrazy (syntaxAnalyzer)

// cely vyraz pres createParsedSegment (tak, jak ho vola kompilator); vrati place vysledku
inline place evalExpr(Frame &f, std::string line, World &w, fullCompiledCode &code, std::string typeName = "number") {
    int pos = 0;
    return createParsedSegment(w.types.findType(typeName, NUMBER_SIZE), f, pos, line, code, varsOf(f, w), w.funcs);
}

// jen parseBT s vlastnim seznamem uzlu, aby sel strom po sestaveni prohlednout (kod se vygeneruje taky)
inline place parseTree(Frame &f, std::string line, World &w, fullCompiledCode &code, std::deque<basicNode> &nodes) {
    int pos = 0, nextId = 0;
    return parseBT(pos, line, code, f, nodes, nextId, varsOf(f, w), w.funcs);
}

// jeden operator nad dvema hotovymi place (basicNode::evaluateOperator)
inline place evalOperator(operators op, Frame &f, const Place &a, const Place &b, fullCompiledCode &code) {
    basicNode node;
    node.myArType = nodeTypes::aritmo;
    node.oper = op;
    node.singleOper = singleOperator::noSingleOper;
    ::place pa = toPlace(a), pb = toPlace(b);
    return node.evaluateOperator(code, f, pa, pb);
}

inline int operatorLevel(operators op) { return operatorToLevel(op); }

} // namespace adapter
