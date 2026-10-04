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
    bool reference;
};


struct typee {
    std::string name;
    std::vector<typee*> params;
    std::vector<std::string> paramKeys;
    int size;

    typee* findParamOnKey(std::string &key, int &pointerLocal) {
        for (int i = 0; i < params.size(); i++) {
            if (paramKeys[i] == key) {
                return params[i];
            }
            pointerLocal += params[i]->size;
        }
        //non existent parameter
        return nullptr;
    }

    numberVar paramPointer(int pointer0, std::vector<std::string> &keysAfter) {
        if (name == numType) {
            return {pointer0, size, false};
        }

        if (keysAfter.size() > 0) {
            typee* nextType = findParamOnKey(keysAfter[0], pointer0);
            if (nextType == nullptr) {
                return {-1, -1, false};
            }
            keysAfter.erase(keysAfter.begin());
            return nextType->paramPointer(pointer0, keysAfter);
        }
        return {pointer0, size, false};
    }

    void calculateSize(int potSize = -1) {
        if (name == numType) {
            size = potSize;
        }
        for (int i = 0; i < params.size(); i++) {
            size += params[i]->size;
        }
    }
};


struct typesHandle {
    std::deque<typee> typeList;

    void createNewType(std::string name, std::vector<typee*> &params, std::vector<std::string> &paramKeys) {
        typeList.push_back({name, params, paramKeys, 0});
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
    for (int i = 0; i < std::min(varTo.sizeB, varFrom.sizeB); i++) {
        compilingCode.addInstruction(asmSyntax.movRamRam + " " + std::to_string(varFrom.pointer + i) + " " + std::to_string(varTo.pointer + i));
    }
}


struct variable {
    int pointer0;
    std::string name;
    typee* typee;
    std::vector<variable> inVars;
    variable* reference = nullptr;

    variable* findOnKey(std::string &key) {
        for (int i = 0; i < inVars.size(); i++) {
            if (inVars[i].name == key) {
                return &inVars[i];
            }
        }
    }

    void set(variable &var, fullCompiledCode &compilingCode, std::vector<std::string> &keyedList) {
        variable* finder = this;
        for (int i = 0; i < keyedList.size(); i++) {
            if (finder->reference != nullptr) {
                finder = reference->findOnKey(keyedList[i]);
                continue;
            }
            finder = finder->findOnKey(keyedList[i]);
        }

        if (finder->reference != nullptr) {
            if (var.reference != nullptr) {
                var.reference = finder->reference;
                return;
            }
            finder = reference;
        }

        for (int i = 0; i < std::min(var.typee->size, finder->typee->size); i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + " " + std::to_string(finder->pointer0 + i) + " " + std::to_string(var.pointer0 + i));
        }

    }

    void call(std::vector<std::string> &keyedList, fullCompiledCode &compilingCode, numberVar &varThere) {
        if (varThere.reference && reference) {
            if (varThere.sizeB != 2) {
                std::cout << "The var there is not a correct pointer" << "\n" << std::endl;
                return;
            }
            std::cout << "Reference on a reference generated" << "\n" << std::endl;
            copyVar(compilingCode, varThere, {pointer0, 2, true});
            return;
        }

        numberVar var2 = typee->paramPointer(pointer0, keyedList);

        if (varThere.reference) {
            if (varThere.sizeB != 2) {
                std::cout << "The var there is not a correct pointer" << "\n" << std::endl;
                return;
            }
            compilingCode.addInstruction(asmSyntax.setRam16 + " " + std::to_string(var2.pointer) + " " + std::to_string(var2.pointer));
            return;
        }

        if (varThere.sizeB != var2.sizeB) {
            std::cout << "The vars dont have same sizes, not calling" << "\n" << std::endl;
            return;
        }

        copyVar(compilingCode, varThere, var2);
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
            return {-1, -1, false};
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


