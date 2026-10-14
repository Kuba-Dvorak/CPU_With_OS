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
    std::vector<variable> inputsInner;
    variable outputType; // od pozice 0
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
        std::string returName = returnNamer;
        outputType = variable(returName, handle.findType(outPutTypeName), mySegmentManager.reqFreeForVar(handle.findType(outPutTypeName)->size, returnNamer), false);
        jumpToFunction = code.currentLine;
        //misto kde je navrat na origin
        mySegmentManager.reqFreeForVar(2, "jumpBacker");

        myRamSeg->myCodeSeg.back().expand(outPutType->size);

        for (size_t i = 0; i < varNames.size(); i++) {
            timeLine.addVar(varNames[i]);
            varsHandle.addNewVariable(varNames[i], STATIC_RAM, varTypes[i], myRamSeg->myCodeSeg.back().end, isReferences[i]);
            inputsInner.push_back(varsHandle.varList[varsHandle.varList.size() - 1]);
            if (isReferences[i]) {
                myRamSeg->myCodeSeg.back().expand(2);
                continue;
            }
            myRamSeg->myCodeSeg.back().expand(varTypes[i]->size);
        }

        return true;
    }

    bool getCalled(fullCompiledCode &code) {
        if (setRamLocal(code.currentLine, outputType.myTypee->size, 2, code)) {

            return true;
        }
        return false;
    }

    void callReturn() {

    }
};
