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
    int lastSize;
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
            int returnSize = size;
            if (pass.isThisRef) {
                returnSize = 2;
            }
            if (pass.alreadyInReg) {
                if (pass.curentlyInOper) {
                    code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + inRegToString(pass.placeTo));
                }
                code.addInstruction(asmSyntax.addReg + inRegToString(pass.placeTo) + std::to_string(pass.curPointer));
                return {-1, true, pass.isThisRef, returnSize};
            }
            return {pass.curPointer, false, pass.isThisRef, returnSize};
        }

        typee* child = findParamOnKey(keysAfter[0], pass);
        keysAfter.erase(keysAfter.begin());

        if (child == nullptr) {
            return {-2, false, false, -1};
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

    variable(std::string &name, typee* theType, int pointer0, bool isRef) {
        this->name = name;
        myTypee = theType;
        localPointer0 = pointer0;
        this->isRef = isRef;
    }

    void call(std::vector<std::string> &keysAfter, fullCompiledCode &code, place &whereTo) {
        typePasser passerr = {isRef, localPointer0, inRegOposite(whereTo.regLoc)};
        if (isRef && !keysAfter.empty()) {
            code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + std::to_string(localPointer0) + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + inRegToString(inRegOposite(whereTo.regLoc)));
            passerr.curPointer = 0;
            passerr.alreadyInReg = true;
        }
        offset myOffset = myTypee->loadFromTyper(code, keysAfter, passerr);
        if (myOffset.localPointer == -2) {
            //error, keysAfter jsou spatne
            return;
        }
        copyFromTo({myOffset.lastSize, myOffset.localPointer, myOffset.isRef, myOffset.inReg, inRegOposite(whereTo.regLoc)}, whereTo, code);
    }


    void set(std::vector<std::string> &keysAfter, fullCompiledCode &code, place &whereFrom) {
        typePasser passerr = {isRef, localPointer0, inRegOposite(whereFrom.regLoc)};
        if (isRef && !keysAfter.empty()) {
            code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + std::to_string(localPointer0) + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + inRegToString(inRegOposite(whereFrom.regLoc)));
            passerr.curPointer = 0;
            passerr.alreadyInReg = true;
        }
        offset myOffset = myTypee->loadFromTyper(code, keysAfter, passerr);
        if (myOffset.localPointer == -2) {
            //error, keysAfter jsou spatne
            return;
        }
        copyFromTo(whereFrom, {myOffset.lastSize, myOffset.localPointer, myOffset.isRef, myOffset.inReg, inRegOposite(whereFrom.regLoc)}, code);
    }

};


struct typesHandle {
    std::deque<typee> typeList;

    void startCreatingNewType(std::string &name, int potSize = 0) {
        typeList.push_back({name});
        if (name == numType) {
            typeList.back().calculateSize(potSize);
        }
    }

    void createPartNewType(std::string &param, std::string &key, bool isRef) {
        typeList.back().params.push_back(findType(param));
        typeList.back().isRef.push_back(isRef);
        typeList.back().paramKeys.push_back(key);
    }

    void stopCreatingNewType() {
        typeList.back().calculateSize();
    }

    typee* findType(std::string &key) {
        for (size_t i = 0; i < typeList.size(); i++) {
            if (typeList[i].name == key) {
                return &typeList[i];
            }
        }
        std::cout << "Type doesn't exist: " << key << "\n" << std::endl;
        return nullptr;
    }
};


struct varHandle {
    std::deque<variable> varList;

    void addNewVariable(std::string &name, typee* typee, int pointer, bool isRef) {
        varList.push_back({name, typee, pointer, isRef});
    }

    variable* returnVar(std::string &name) {
        //jdeme odzadu => vzdy vezmeme to v nejnizsim levelu => promene se stejnym jmenem
        for (int i = varList.size() - 1; i >= 0; i--) {
            if (varList[i].name == name) {
                return &varList[i];
            }
        }
        std::cout << "Variable doesn't exist: " << name << "\n" << std::endl;
        return nullptr;
    }


    void deleteVar(std::string &name) {
        for (int i = varList.size() - 1; i >= 0; i--) {
            if (varList[i].name == name) {
                varList.erase(varList.begin() + i);
                return;
            }
        }
        std::cout << "Variable doesn't exist: " << name << "\n" << std::endl;
    }
};