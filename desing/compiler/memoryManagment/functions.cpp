#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include "./scopes.cpp"


struct basicFunction {
    std::string name;
    segmentManager mySegmentManager = {};
    std::vector<variable*> inputsInner;
    variable* outputType; // od pozice 0
    std::vector<std::string> emptyKeyList = {};
    int jumpToFunction;

    bool define(std::string name, std::vector<bool> &isReferences, std::vector<std::string> &varNames, std::vector<std::string> &varTypesNames,
        varHandle &varsHandle, std::string &outPutTypeName, typesHandle &handle, fullCompiledCode &code) {
        if ((varNames.size() != varTypesNames.size()) || (varNames.size() != isReferences.size())) {
            std::cout << "Function hasn't got the same amount of names and types" << std::endl;
            return false;
        }

        this->name = name;
        mySegmentManager = {{}, {}};
        std::string returName = returnNamer + "Var";
        varsHandle.addNewVariable(returName,  handle.findType(outPutTypeName),
            mySegmentManager.reqFreeForVar(handle.findType(outPutTypeName)->size, returnNamer), false);
        outputType = varsHandle.returnVar(returName);
        jumpToFunction = code.currentLine;
        //misto kde je navrat na origin
        mySegmentManager.reqFreeForVar(2, "jumpBacker");


        for (int i = 0; i < varNames.size(); i++) {
            typee* theType = handle.findType(varTypesNames[i]);
            if (isReferences[i]) {
                varsHandle.addNewVariable(varNames[i],  theType,
            mySegmentManager.reqFreeForVar(2, returnNamer), isReferences[i]);
                continue;
            }
            varsHandle.addNewVariable(varNames[i],  theType,
            mySegmentManager.reqFreeForVar(theType->size, returnNamer), isReferences[i]);
        }

        return true;
    }

    //vola se AZ PO FUNKCI ZE SEGMENTU, TOTO UZ JE V LOKALNI PROMENE SETUP
    bool getCalled(fullCompiledCode &code) {
        if (setRamLocal(code.currentLine + 3, outputType->myTypee->size, 2, code)) {
            code.addInstruction(asmSyntax.setReg + asmSyntax.reg5 + std::to_string(jumpToFunction));
            code.addInstruction(asmSyntax.jumpFromReg + asmSyntax.reg5);
            return true;
        }
        return false;
    }

    //pro prvotni kompilaci funkce, pisu sem kvuli prehlednosti, zavola se do psani kodu na misto kde bude
    //return ....
    void getReturnCMD(place thingToReturn, fullCompiledCode &code) {
        std::vector<std::string> nothingKeys = {};
        outputType->set(nothingKeys, code, thingToReturn);
        code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + std::to_string(outputType->myTypee->size) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + asmSyntax.reg5);
        code.addInstruction(asmSyntax.jumpFromReg + asmSyntax.reg5);
    }
};
