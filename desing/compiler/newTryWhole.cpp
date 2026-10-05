 #include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include "./syntax.cpp"

#define numType "number"


struct fullCompiledCode {
    std::string code = "";
    int currentLine = 0;

    void addInstruction(std::string instrText) {
        code += instrText;
        currentLine += 1;
    }
};


struct numberVar {
    int pointer;
    int sizeB;
    bool isRef;
};


struct typee {
    std::string name;
    std::vector<typee*> params;
    std::vector<std::string> paramKeys;
    int size;
    std::vector<bool> isRef;

    typee* findParamOnKey(std::string &key, int &diff, bool &wasITRef) {
        for (int i = 0; i < params.size(); i++) {
            if (paramKeys[i] == key) {
                wasITRef = isRef[i];
                return params[i];
            }
            diff += params[i]->size;
        }
        //non existent parameter
        return nullptr;
    }

    //pokud z toho vyjde wasReferenced jako true, tak nam tahle funkce garantuje ze pointer na to misto je v Reg1!!!
    numberVar paramPointer(int pointer0, bool &wasReferenced, bool aboutThis, fullCompiledCode &compilingCode, std::vector<std::string> &keysAfter) {
        if (wasReferenced) {
            //kdyz jsme ukoncili radu, tak musime tohle vratit
            if (keysAfter.size() <= 0) {
                return {0, size, aboutThis};
            }

            //kdyz je to cislo tak musime vratit to cislo o velikosti cislo, protoze cislo pak pod sebou nema dalsi
            if (name == numType) {
                return {0, size, aboutThis};
            }

            int toAdd = 0;
            bool someRandom = false;
            typee* lookedFor = findParamOnKey(keysAfter[0], toAdd, someRandom);
            keysAfter.erase(keysAfter.begin());

            if (toAdd != 0) {
                compilingCode.addInstruction(asmSyntax.setReg + asmSyntax.reg2 + " " + std::to_string(toAdd));
                compilingCode.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.reg2);
            }

            compilingCode.addInstruction(asmSyntax.movRegReg + asmSyntax.reg0 + asmSyntax.reg1);

            if (someRandom) {
                compilingCode.addInstruction(asmSyntax.movRamReg + asmSyntax.addr0 + asmSyntax.reg1);
            }

            return lookedFor->paramPointer(0, wasReferenced, someRandom, compilingCode, keysAfter);
        }

        else {
            //kdyz jsme ukoncili radu, tak musime tohle vratit
            if (keysAfter.size() <= 0) {
                return {pointer0, size, aboutThis};
            }

            //kdyz je to cislo tak musime vratit to cislo o velikosti cislo, protoze cislo pak pod sebou nema dalsi
            if (name == numType) {
                return {pointer0, size, aboutThis};
            }

            int toAdd = 0;
            bool someRandom = false;
            typee* lookedFor = findParamOnKey(keysAfter[0], toAdd, someRandom);
            keysAfter.erase(keysAfter.begin());
            wasReferenced = someRandom;

            if (someRandom) {
                compilingCode.addInstruction(asmSyntax.movRamReg + " " + std::to_string((toAdd + pointer0)) + asmSyntax.reg1);
                return lookedFor->paramPointer(0, wasReferenced, someRandom, compilingCode, keysAfter);
            }
            return lookedFor->paramPointer(toAdd + pointer0, wasReferenced, someRandom, compilingCode, keysAfter);
        }

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


struct typesHandle {
    std::deque<typee> typeList;

    void createNewType(std::string name, std::vector<typee*> &params, std::vector<std::string> &paramKeys, std::vector<bool> &refers) {
        typeList.push_back({name, params, paramKeys, 0, refers});
        typeList.back().calculateSize();
    }

    typee* findType(std::string key) {
        for (size_t i = 0; i < typeList.size(); i++) {
            if (typeList[i].name == key) {
                return &typeList[i];
            }
        }
        std::cout << "Type doesn't exist: " << key << "\n" << std::endl;
        return nullptr;
    }
};


void copyVar(fullCompiledCode &compilingCode, numberVar varTo, numberVar varFrom) {
    for (int i = 0; i < varTo.sizeB; i++) {
        compilingCode.addInstruction(asmSyntax.movRamRam + " " + std::to_string(varFrom.pointer + i) + " " + std::to_string(varTo.pointer + i));
    }
}


struct variable {
    int pointer0;
    std::string name;
    typee* typee;
    bool isRef = false;

    void set(variable &var, fullCompiledCode &compilingCode, std::vector<std::string> &keyedList) {
        //keyed list je pro NAS!!!, u varu verime ze je spravneho typu
        if (keyedList.size() == 0) {
            if (isRef) {
                if (var.isRef) {
                    copyVar(compilingCode, {pointer0, 2}, {var.pointer0, 2});
                    return;
                }
                compilingCode.addInstruction(asmSyntax.setRam16 + " " + std::to_string(pointer0) + " " + std::to_string(var.pointer0));
                return;
            }
            copyVar(compilingCode, {pointer0, typee->size}, {var.pointer0, var.typee->size});
            return;
        }

        bool willBeRef = isRef;
        numberVar result = typee->paramPointer(pointer0, willBeRef, false, compilingCode, keyedList);

        //result v R1
        if (willBeRef) {
            //tamto je reference
            if (var.isRef) {
                //zkopirujeme jenom tu referenci, ten pointer
                if (result.isRef) {
                    compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + std::to_string(var.pointer0));
                    return;
                }

                compilingCode.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + std::to_string(var.pointer0));

                for (int i = 0; i < result.sizeB; i++) {
                    compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + asmSyntax.addr1);
                    compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
                    compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
                }
                return;
            }

            //tamto neni reference
            if (result.isRef) {
                compilingCode.addInstruction(asmSyntax.setRam16 + asmSyntax.addr0 + std::to_string(var.pointer0));
                return;
            }

            //jinak kopirujeme
            for (int i = 0; i < result.sizeB; i++) {
                compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + std::to_string(var.pointer0));
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
            }
            return;
        }

        //pouze kopirujeme, neboli to nase neni reference
        if (var.isRef) {
            compilingCode.addInstruction(asmSyntax.movRegRam + asmSyntax.reg1 + std::to_string(var.pointer0));
            for (int i = 0; i < result.sizeB; i++) {
                compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(result.pointer) + asmSyntax.addr0);
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
            }
            return;
        }

        //pouze kopirovani
        copyVar(compilingCode, {result.pointer, result.sizeB}, {var.pointer0, var.typee->size});
    }

    void call(std::vector<std::string> &keyedList, fullCompiledCode &compilingCode, numberVar &varThere) {

    }
};


struct varHandle {
    std::deque<variable> varList;

    void addNewVariable(std::string &name, typee* typee, int pointer, bool isRef) {
        varList.push_back({pointer, name, typee});
    }

    variable* returnVar(std::string &name) {
        for (size_t i = 0; i < varList.size(); i++) {
            if (varList[i].name == name) {
                return &varList[i];
            }
        }
        std::cout << "Variable doesn't exist: " << name << "\n" << std::endl;
        return nullptr;
    }


    void deleteVar(std::string &name) {
        for (size_t i = 0; i < varList.size(); i++) {
            if (varList[i].name == name) {
                varList.erase(varList.begin() + i);
                return;
            }
        }
        std::cout << "Variable doesn't exist: " << name << "\n" << std::endl;
    }
};


struct timeLineManager {
    std::vector<std::string> varNames = {};

    void destroyLocals(varHandle &handle) {
        for (size_t i = 0; i < varNames.size(); i++) {
            handle.deleteVar(varNames[i]);
        }
    }

    void addVar(std::string const &name) {
        varNames.push_back(name);
    }
};


struct cacheSegment {
    int start;
    int end;
    int current;

    cacheSegment(int pointer0, int fullSize) {
        start = pointer0;
        end = pointer0 + fullSize;
        current = start;
    }

    numberVar requestFreeSub(int itsSize) {
        if (current + itsSize > end) {
            std::cout << "Ran out of one line stack" << std::endl;
            return {-1, -1};
        }
        current += itsSize;
        return {current - itsSize, itsSize};
    }

    void resetCache() {
        current = start;
    }
};


//code segment je pro promene, cacheSegment pro mezivypocty
struct codeSegmentRam {
    int start;
    int end;

    codeSegmentRam(int pointer0) {
        start = pointer0;
        end = pointer0;
    }

    void expand(int size) {
        end += size;
    }
};


struct functionSegment {
    cacheSegment myCacheSeg;
    codeSegmentRam myCodeSeg;
};


struct basicFunction {
    std::string name;
    functionSegment* myRamSeg;
    std::vector<variable> inputsInner;
    variable output;
    int jumpBackToOrigin;
    int jumpToFunction;

    bool getCalled(fullCompiledCode &compilingCode, std::vector<numberVar> &inputsOuter) {
        if (inputsOuter.size() != inputsInner.size()) {
            std::cout << "Function hasn't got the correct amount of parameters" << std::endl;
            return false;
        }

        for (int i = 0; i < inputsInner.size(); i++) {
            if (inputsInner[i].reference) {
                compilingCode.addInstruction(asmSyntax.setRam16 + " " + std::to_string(inputsInner[i].pointer0) + " " + std::to_string(inputsOuter[i].pointer));
                continue;
            }

            copyVar(compilingCode, {inputsInner[i].pointer0, inputsInner[i].typee->size}, inputsOuter[i]);
        }

        compilingCode.addInstruction(asmSyntax.setRam16 + " " + std::to_string(jumpBackToOrigin) + " " + std::to_string(compilingCode.currentLine + 2));
        compilingCode.addInstruction(asmSyntax.jumpDictate + " " + std::to_string(jumpToFunction));
        return true;
    }

    void define(std::string name, std::vector<bool> &isReferences, std::vector<std::string> varNames, std::vector<typee*> varTypes, typee* outputType,
        int outputPointer, varHandle &varsHandle, timeLineManager &timeLine, functionSegment* hisPart) {
        if (varNames.size() != varTypes.size()) {
            std::cout << "Function hasn't got the same amount of names and types" << std::endl;
            return;
        }

        this->name = name;

        output = variable(outputPointer++, name, outputType, false);

        for (size_t i = 0; i < varNames.size(); i++) {
            timeLine.addVar(varNames[i]);
            varsHandle.addNewVariable(varNames[i], varTypes[i], outputPointer++, isReferences[i]);
            inputsInner.push_back(varsHandle.varList[varsHandle.varList.size() - 1]);
        }

        myRamSeg = hisPart;
    }
};


struct functionHandle {
    std::vector<basicFunction> functionList;

    basicFunction* returnVar(std::string &name) {
        for (size_t i = 0; i < functionList.size(); i++) {
            if (functionList[i].name == name) {
                return &functionList[i];
            }
        }
        std::cout << "Function doesn't exist: " << name << "\n" << std::endl;
        return nullptr;
    }
};


struct memoryManager {
    functionHandle functionsHandle;
    varHandle varsHandle;
    typesHandle typesHandle;
    cacheSegment cacheCurrent;
    std::vector<codeSegmentRam> normalSegments; // dynamicke
    std::vector<functionSegment> staticSegments;
    std::vector<timeLineManager> timelinesList;
    bool lastWasFunction = false;

    void createSubCode(bool isFunction, int wantedLenghtFunq = 50) {
        lastWasFunction = isFunction;
        if (lastWasFunction) {
            //nejdriv cache o nejake definovane velikosti, potom codeSegment, ktery muze rust za doby existence funkce
            staticSegments.push_back({cacheSegment(staticSegments.back().myCodeSeg.end, wantedLenghtFunq),
                codeSegmentRam(staticSegments.back().myCodeSeg.end + wantedLenghtFunq)});
        }
        else {
            normalSegments.push_back({normalSegments.back().end});
        }
        timelinesList.push_back({});
    }

    void destroyCurrentTimeLine() {
        timelinesList.back().destroyLocals(varsHandle);
        if (!lastWasFunction) {
            normalSegments.pop_back();
        }
        else {
            lastWasFunction = false;
        }
        timelinesList.pop_back();
    }



};


