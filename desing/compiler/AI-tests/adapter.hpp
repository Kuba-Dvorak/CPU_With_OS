#pragma once
// Jedine misto, kde se testy dotykaji kodu kompilatoru.
// Kdyz pri prepisu zmenis jmena nebo parametry, uprav jen tenhle soubor.
// Include jde pres -I.. (mutate.py podstrci -I na upravenou kopii).
#include <string>
#include <utility>
#include <vector>
#include "memoryManagment/functions.cpp"

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
};

inline void defineNumber(World &w, int size) {
    std::string n = numType;
    w.types.startCreatingNewType(n, size);
}

inline void defineStruct(World &w, std::string name, const std::vector<Field> &fields) {
    w.types.startCreatingNewType(name);
    for (auto f : fields) w.types.createPartNewType(f.type, f.key, f.isRef);
    w.types.stopCreatingNewType();
}

inline int typeSize(World &w, std::string name) { return w.types.findType(name)->size; }

inline variable *addVar(World &w, std::string name, std::string type, int localOffset, bool isRef) {
    w.vars.addNewVariable(name, w.types.findType(type), localOffset, isRef);
    return w.vars.returnVar(name);
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

inline void openScope(Frame &f) { f.createNewScope(); }
inline int allocVar(Frame &f, int size, std::string name) { return f.reqFreeForVar(size, name); }
inline int allocTemp(Frame &f, int size) { return f.reqFreeSub(size); }
inline void closeScope(Frame &f, World &w) { f.stopCurScope(w.vars); }
inline void endLine(Frame &f) { f.endLine(); }
inline variable *findVar(World &w, std::string name) { return w.vars.returnVar(name); }

struct Function {
    basicFunction fn;
    std::vector<std::string> paramNames;
};

inline void defineFunction(Function &f, std::string name, std::vector<std::string> paramNames,
                           std::vector<std::string> paramTypes, std::vector<bool> paramRefs, std::string outType,
                           World &w, fullCompiledCode &code) {
    f.paramNames = paramNames;
    std::vector<bool> refs = paramRefs;
    f.fn.define(name, refs, paramNames, paramTypes, w.vars, outType, w.types, code);
}

inline Frame &bodyFrame(Function &f) { return f.fn.mySegmentManager; }
inline std::vector<variable *> &inputs(Function &f) { return f.fn.inputsInner; }
inline int startLine(Function &f) { return f.fn.jumpToFunction; }

// offset parametru v ramci funkce (vuci SP uvnitr funkce)
inline variable *param(World &w, std::string name) { return w.vars.returnVar(name); }
inline int paramOffset(World &w, std::string name) { return param(w, name)->localPointer0; }

// cele volani z ramce caller; vrati offset vystupu funkce v ramci caller
inline int callFunction(Frame &caller, Function &f, std::vector<std::pair<variable *, std::vector<std::string>>> args,
                        World &w, fullCompiledCode &code) {
    int outOffset = caller.reqFreeSub(0);
    std::vector<passerPair> in;
    for (auto &a : args) in.push_back({a.first, a.second});
    // parametry podle jmena: nezavisle na inputsInner, aby test nekopiroval to, co testuje
    std::vector<variable *> params;
    for (auto &n : f.paramNames) params.push_back(w.vars.returnVar(n));
    caller.callDiffFunq(code, f.fn.outputType, in, params, addressSize);
    f.fn.getCalled(code);
    caller.endFunq(code, f.fn.outputType, params, addressSize);
    return outOffset;
}

inline void returnFrom(Function &f, const Place &what, fullCompiledCode &code) { f.fn.getReturnCMD(toPlace(what), code); }

} // namespace adapter
