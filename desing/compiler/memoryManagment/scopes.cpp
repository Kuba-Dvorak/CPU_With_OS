#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include "./variables.cpp"



struct codeScope {
    int curPos = 0;
    std::vector<std::string> scopeVars;

    void increase(int bySize) {
        curPos += bySize;
    }

    void destroy(varHandle &handle) {
        for (size_t i = 0; i < scopeVars.size(); i++) {
            handle.deleteVar(scopeVars[i]);
        }
    }
};


struct cache {
    int curSize = 0;

    int reqFreePointer(int forSize) {
        curSize += forSize;
        return curSize - forSize;
    }

    void reset() {
        curSize = 0;
    }
};


struct segmentManager {
    std::vector<codeScope> curScopes;
    cache myCache;

    void callDiffFunq(fullCompiledCode &code) {
        code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + std::to_string(curScopes.back().curPos + myCache.curSize) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + asmSyntax.regStackPointer);
    }

    //volat hned po ukonceni funkce !!!
    void endFunq(fullCompiledCode &code) {
        code.addInstruction(asmSyntax.rmReg + asmSyntax.regStackPointer + std::to_string(curScopes.back().curPos + myCache.curSize) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + asmSyntax.regStackPointer);
    }

    int reqFreeSub(int forSize) {
        return curScopes.back().curPos + myCache.reqFreePointer(forSize);
    }

    void endLine() {
        myCache.reset();
    }

    void reqFreeForVar(int forSize, std::string &name) {
        curScopes.back().increase(forSize);
        curScopes.back().scopeVars.push_back(name);
    }

    void createNewScope() {
        if (curScopes.empty()) {
            curScopes.push_back({0, {}});
            return;
        }
        curScopes.push_back({curScopes.back().curPos, {}});
        myCache.reset();
    }

    void stopCurScope(varHandle &handle) {
        curScopes.back().destroy(handle);
        curScopes.pop_back();
    }
};




