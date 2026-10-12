#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <regex>
#include <string_view>
#include "./memoryOperations.cpp"

#define numType "number"
#define voidType "void"


struct offset {
    int localPointer; // => chyba
    bool inReg;
    bool isRef;
};


struct typePasser {
    bool isThisRef = false;
    int curPointer = 0;
    inReg placeTo = inReg::reg1;
    bool alreadyInReg = false;
    bool curentlyInOper = false;
};


struct typee {
    std::string name;
    std::vector<typee*> params;
    std::vector<std::string> paramKeys;
    int size;
    std::vector<bool> isRef;

    typee* findParamOnKey(std::string &key, typePasser &pass) {
        for (int i = 0; i < params.size(); i++) {
            if (paramKeys[i] == key) {
                pass.isThisRef = isRef[i];
                return params[i];
            }
            if (isRef[i]) {
                pass.curPointer += 2;
                continue;
            }
            pass.curPointer += params[i]->size;
        }
        //non existent parameter
        return nullptr;
    }

    offset loadFromTyper(fullCompiledCode &code, std::vector<std::string> &keysAfter, typePasser &pass) {
        if (keysAfter.size() <= 0) {
            if (pass.alreadyInReg) {
                if (pass.curentlyInOper) {
                    code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + inRegToString(pass.placeTo));
                }
                code.addInstruction(asmSyntax.addReg + inRegToString(pass.placeTo) + std::to_string(pass.curPointer));
                return {-1, true, pass.isThisRef};
            }
            return {pass.curPointer, false, pass.isThisRef};
        }

        typee* child = findParamOnKey(keysAfter[0], pass);
        keysAfter.erase(keysAfter.begin());

        if (child == nullptr) {
            return {-2, false, false};
        }

        if (pass.alreadyInReg) {
            std::string location;
            std::string location2;
            if (pass.curentlyInOper) {
                location = asmSyntax.regStackOper;
                location2 = inRegToString(pass.placeTo);
            }
            else {
                location = inRegToString(pass.placeTo);
                location2 = asmSyntax.regStackOper;
            }
            if (pass.isThisRef && !keysAfter.empty()) {
                code.addInstruction(asmSyntax.addReg + location + std::to_string(pass.curPointer));
                pass.curPointer = 0;
                code.addInstruction(asmSyntax.movRamReg16 + location + location2);
                pass.curentlyInOper = !pass.curentlyInOper;
            }
            return child->loadFromTyper(code, keysAfter, pass);
        }

        if (pass.isThisRef && !keysAfter.empty()) {
            code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + std::to_string(pass.curPointer) + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + inRegToString(pass.placeTo));
            pass.curPointer = 0;
            pass.curentlyInOper = false;
            pass.alreadyInReg = true;
            return child->loadFromTyper(code, keysAfter, pass);
        }
        return child->loadFromTyper(code, keysAfter, pass);
    }

    void calculateSize(int potSize = -1) {
        if (name == numType) {
            size = potSize;
        }

        for (int i = 0; i < params.size(); i++) {
            if (isRef[i]) {
                size += 2;
                continue;
            }
            size += params[i]->size;
        }
    }
};


struct variable {
    std::string name;
    typee* myTypee;
    int localPointer0;
    bool isRef;

    void call(std::vector<std::string> &keysAfter, fullCompiledCode &code) {

    }


    void set() {

    }

};







