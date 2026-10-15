#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>
#include <string_view>
#include "./variables.cpp"



struct codeScope {
    int curPos = 0;
    std::vector<std::string> scopeVars = {};

    int increase(int bySize) {
        curPos += bySize;
        return curPos - bySize;
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


struct passerPair {
    variable* var;
    std::vector<std::string> keys;
};


struct segmentManager {
    std::vector<codeScope> curScopes;
    cache myCache;
    int lastFunqOffset = 0;

    //in vars jsou ty co tam posilame do toho, ty z funkce jsou ty funqVars
    void callDiffFunq(fullCompiledCode &code, variable* output, std::vector<passerPair> &inVars,
        std::vector<variable*> const &funqVars, int jumpSize) {
        if (funqVars.size() != inVars.size()) {
            //chyba, nespravny pocet argumentu na vstupu funkce
            return;
        }
        int absSize = output->myTypee->size + jumpSize;
        myCache.reqFreePointer(output->myTypee->size + jumpSize);

        for (int i = 0; i < inVars.size(); i++) {
            int itsSize = funqVars[i]->getSize();
            place location = {itsSize, myCache.reqFreePointer(itsSize), funqVars[i]->isRef, false, inReg::reg2};
            absSize += itsSize;
            inVars[i].var->call(inVars[i].keys, code, location);
        }

        code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer
            + std::to_string(curScopes.back().curPos + myCache.curSize - absSize) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + asmSyntax.regStackPointer);
        lastFunqOffset = absSize;
    }

    //volat hned po ukonceni funkce !!!
    void endFunq(fullCompiledCode &code, variable* output, std::vector<variable*> const &inVars, int jumpSize) {
         //takhle kdyz pak nacteme dalsi freeSub, tak v nem bude ten output (pokud bude mit velikost outputSize)
         ////a ty dalsi potom muzeme prepsat takze to je ok
        code.addInstruction(asmSyntax.rmReg + asmSyntax.regStackPointer
            + std::to_string(curScopes.back().curPos + myCache.curSize - lastFunqOffset) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + asmSyntax.regStackPointer);
    }

    int reqFreeSub(int forSize) {
        return curScopes.back().curPos + myCache.reqFreePointer(forSize);
    }

    void endLine() {
        myCache.reset();
    }

    int reqFreeForVar(int forSize, std::string name) {
        curScopes.back().scopeVars.push_back(name);
        return curScopes.back().increase(forSize);
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




