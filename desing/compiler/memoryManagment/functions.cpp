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
    std::deque<variable*> inputsInner = {};
    variable* outputType; // od pozice 0
    std::vector<std::string> emptyKeyList = {};
    int jumpToFunction;
    varHandle myVarHandlerer;
    int changeBackerInstr;

    bool define(std::string name, std::vector<bool> &isReferences, std::vector<std::string> &varNames, std::vector<std::string> &varTypesNames
        , std::string &outPutTypeName, bool outputIsRef, typesHandle &handle, fullCompiledCode &code) {
        if ((varNames.size() != varTypesNames.size()) || (varNames.size() != isReferences.size())) {
            std::cout << "Function hasn't got the same amount of names and types" << std::endl;
            return false;
        }

        this->name = name;
        mySegmentManager = {{}, {}};
        changeBackerInstr = code.currentLine;
        code.addInstruction(asmSyntax.jumpDictate);
        std::string returName = returnNamer + "Var";
        myVarHandlerer.addNewVariable(returName,  handle.findType(outPutTypeName),
            mySegmentManager.reqFreeForVar(handle.findType(outPutTypeName)->size, returName), outputIsRef);
        outputType = &myVarHandlerer.varList.back();
        jumpToFunction = code.currentLine;
        //misto kde je navrat na origin
        mySegmentManager.reqFreeForVar(2, "jumpBacker");


        for (int i = 0; i < varNames.size(); i++) {
            typee* theType = handle.findType(varTypesNames[i]);
            if (isReferences[i]) {
                myVarHandlerer.addNewVariable(varNames[i],  theType,
            mySegmentManager.reqFreeForVar(2, varNames[i]), isReferences[i]);
                inputsInner.push_back(&myVarHandlerer.varList.back());
                continue;
            }
            myVarHandlerer.addNewVariable(varNames[i],  theType,
            mySegmentManager.reqFreeForVar(theType->size, varNames[i]), isReferences[i]);
            inputsInner.push_back(&myVarHandlerer.varList.back());
        }

        return true;
    }

    //vola se AZ PO FUNKCI ZE SEGMENTU, TOTO UZ JE V LOKALNI PROMENE SETUP
    bool getCalled(fullCompiledCode &code) {
        //addressSize * 2 + 3
        if (setRamLocal(code.currentLine + (addressSize) * 2 + 2, outputType->getSize(), 2, code)) {
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
        //outputType muze byt referenci, ale pokud tomu tak je, tak funkce returnuje ten pointer a nechceme zmenit to na co ukazuje
        if (thingToReturn.size != -1) {
            outputType->set(nothingKeys, code, thingToReturn, false);
        }
        code.addInstruction(asmSyntax.addRegNoCarry + asmSyntax.regStackPointer + std::to_string(outputType->getSize()) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + asmSyntax.reg5);
        code.addInstruction(asmSyntax.jumpFromReg + asmSyntax.reg5);
    }

    int getInputsSize() {
        int absSize = 0;
        for (int i = 0; i < inputsInner.size(); i++) {
            absSize += inputsInner[i]->getSize();
        }
        return absSize;
    }

    //jumpBack aby se pri definici rovnou nevolal
    void endDefinition(fullCompiledCode &code) {
        code.changeBackFunq(asmSyntax.jumpDictate + std::to_string(code.currentLine), changeBackerInstr);
    }
};


struct functionHandle {
    std::deque<basicFunction> functionList;

    basicFunction* findFunction(std::string &name) {
        for (size_t i = 0; i < functionList.size(); i++) {
            if (functionList[i].name == name) {
                return &functionList[i];
            }
        }
        std::cout << "Type doesn't exist: " << name << "\n" << std::endl;
        return nullptr;
    }
};


