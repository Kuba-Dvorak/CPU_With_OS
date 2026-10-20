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
    bool conditioned = false;
    bool cycled = false;
    int startingJumpIndex = 0;
    std::vector<std::string> scopeVars = {};
    std::vector<int> breakPoses;

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
    variable* var = nullptr;
    std::vector<std::string> keys;
};


struct segmentManager {
    std::vector<codeScope> curScopes;
    cache myCache;
    std::vector<int> lastFunqOffset = {};

    //posilame pouze jumpBy a verime binarnimu stromu ze rezervoval spravne a spravne vyplnil veci
    //nasledovne musime nacist reqFreeSub o velikosti toho outputu
    void callDiffFunq(fullCompiledCode &code, int jumpBy) {
        code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer
            + std::to_string(reqFreeSub(0) - jumpBy) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + asmSyntax.regStackPointer);
        lastFunqOffset.push_back(jumpBy);
    }

    //volat hned po ukonceni funkce !!!
    void endFunq(fullCompiledCode &code) {
         //takhle kdyz pak nacteme dalsi freeSub, tak v nem bude ten output (pokud bude mit velikost outputSize)
         //a ty dalsi potom muzeme prepsat takze to je ok
        code.addInstruction(asmSyntax.rmReg + asmSyntax.regStackPointer
            + std::to_string(curScopes.back().curPos + myCache.curSize - lastFunqOffset.back()) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + asmSyntax.regStackPointer);
        myCache.curSize -= lastFunqOffset.back();
        lastFunqOffset.pop_back();
    }

    int reqFreeSub(int forSize) {
        return curScopes.back().curPos + myCache.reqFreePointer(forSize);
    }

    void endLine() {
        myCache.reset();
    }

    int reqFreeForVar(int forSize, std::string name) {
        if (curScopes.empty()) {
            curScopes.push_back({0, {}});
        }
        curScopes.back().scopeVars.push_back(name);
        return curScopes.back().increase(forSize);
    }

    bool callBreak(fullCompiledCode &code) {
        int last = -1;
        for (int i = curScopes.size() - 1; i >= 0; i--) {
            if (curScopes[i].cycled) {
                last = i;
                break;
            }
        }

        if (last == -1) {
            return false;
        }

        curScopes[last].breakPoses.push_back(code.currentLine);
        code.addInstruction(asmSyntax.jumpDictate); //musi byt predelano
        return true;
    }

    bool callContinue(fullCompiledCode &code) {
        int last = -1;
        for (int i = curScopes.size() - 1; i >= 0; i--) {
            if (curScopes[i].cycled) {
                last = i;
                break;
            }
        }

        if (last == -1) {
            return false;
        }

        code.addInstruction(asmSyntax.jumpDictate + std::to_string(curScopes[last].startingJumpIndex));
        return true;
    }

    void createNewScope(bool isConditioned, bool isCycled, fullCompiledCode &code) {
        if (curScopes.empty()) {
            curScopes.push_back({0, isConditioned, isCycled,code.currentLine, {}, {}});
        }
        else {
            curScopes.push_back({curScopes.back().curPos, isConditioned, isCycled,code.currentLine, {}, {}});
        }

        myCache.reset();

        if (isConditioned) {
            curScopes.back().startingJumpIndex = code.currentLine;
            place resultLocation = {}; //tady bude muset byt result z BT ktery je definovat u te kondice
            //taky protoze je to od BT a je to od cache, tak to je v RAM (v cachi)
            code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer
                + std::to_string(resultLocation.pointer) + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRamReg + asmSyntax.regStackOper + asmSyntax.latchCompare);
            code.addInstruction(asmSyntax.setReg + asmSyntax.skippingLatch + std::to_string(1));
            curScopes.back().breakPoses.push_back(code.currentLine);
            code.addInstruction(asmSyntax.jumpDictate); //tohle je na stopCurScope doplneno
        }
    }

    void stopCurScope(varHandle &handle, fullCompiledCode &code) {
        if (curScopes.back().cycled) {
            code.addInstruction(asmSyntax.jumpDictate + std::to_string(curScopes.back().startingJumpIndex));
        }
        if (curScopes.back().conditioned || curScopes.back().cycled) {
            for (int i = 0; i < curScopes.back().breakPoses.size(); i++) {
                code.changeBackFunq(asmSyntax.jumpDictate + std::to_string(code.currentLine), curScopes.back().breakPoses[i]);
            }
        }
        curScopes.back().destroy(handle);
        curScopes.pop_back();
    }
};




