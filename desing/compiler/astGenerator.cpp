#include "./typeConstructor.cpp"
#include <vector>
#include <string>


struct errorCode {
    bool exists;
    int line;
    int lineChar;
    std::string errorMes;
};


struct compiler {
    std::string originCode;
    fullCompiledCode compiledCode;
    functionHandle funqHandler;
    typesHandle typeHandler;
    //v jake funkci zrovna definujeme
    std::vector<basicFunction*> currentFunctionList;
    std::string numberKey = "number";

    void compileSegment(goodLine &oneLine, astNodeNames &lineType) {
        basicFunction* currentFunq = currentFunctionList.back();
        if (lineType == astNodeNames::segmentEnder) {
            //kdyz je poslednis scope, tak je konec definice funkce
            if (currentFunq->mySegmentManager.isLastScope()) {
                currentFunq->getReturnCMD({-1, 5, false}, compiledCode);
                currentFunq->endDefinition(compiledCode);
                currentFunctionList.pop_back();
                return;
            }
            currentFunq->mySegmentManager.stopCurScope(currentFunq->myVarHandlerer, compiledCode);
            return;
        }

        else {
            if (lineType == astNodeNames::segmentAvg) {
                currentFunq->mySegmentManager.createNewScope(false, false, compiledCode, {});
            }
            if (lineType == astNodeNames::segmentConditional) {
                //jenom kondicional ma podminku
                goodFoundSegment segmentPart = findSegmenter(oneLine.oneLine);
                currentFunq->mySegmentManager.createNewScope(true, false, compiledCode,
                    createParsedSegment(typeHandler.findType(numberKey, 1), currentFunq->mySegmentManager,
                        segmentPart.oneLinePos, oneLine.oneLine, compiledCode, currentFunq->myVarHandlerer, funqHandler));
            }
            if (lineType == astNodeNames::segmentCyclic) {
                currentFunq->mySegmentManager.createNewScope(false, true, compiledCode, {});
            }
        }
    }

    void compileDeclaration(goodLine &oneLine, astNodeNames &lineType, int &lineNum) {
        if (lineType == astNodeNames::declareType) {
            createNewType(typeHandler, originCode, loadTypeDefName(oneLine.oneLine), lineNum);
        }
        if (lineType == astNodeNames::declareVar) {
            goodVarDecl loadedVar = loadVarDef(oneLine.oneLine);
            typee* thingType = typeHandler.findType(loadedVar.varTypeName, loadedVar.posibleSize);
            int sizeVar = addressSize;
            if (!loadedVar.isRef) {
                sizeVar = thingType->size;
            }
            currentFunctionList.back()->myVarHandlerer.addNewVariable(loadedVar.varName, thingType,
                currentFunctionList.back()->mySegmentManager.reqFreeForVar(sizeVar, loadedVar.varName), loadedVar.isRef);
        }
        if (lineType == astNodeNames::declareFunction) {
            goodLoadedFunctionDefinition funqInfo = loadDefFunq(oneLine.oneLine);
            funqHandler.functionList.push_back({});
            funqHandler.functionList.back().define(funqInfo.funqName, funqInfo.inputRefs, funqInfo.inputNames,
                funqInfo.inputTypes, funqInfo.outputType, funqInfo.inputSizes, funqInfo.outputSize, funqInfo.outputRef,  typeHandler, compiledCode);
            currentFunctionList.push_back(&funqHandler.functionList.back());
        }
    }

    void compileSetting(goodLine &oneLine) {
        goodVarSet varSet = loadVarSetter(oneLine.oneLine);
        goodFoundSegment segmentPart = findSegmenter(oneLine.oneLine);
        variable* oneVar = currentFunctionList.back()->myVarHandlerer.returnVar(varSet.varName);
        place loadedSegment = createParsedSegment(oneVar->myTypee, currentFunctionList.back()->mySegmentManager,
                        segmentPart.oneLinePos, oneLine.oneLine, compiledCode, currentFunctionList.back()->myVarHandlerer, funqHandler);
        oneVar->set(varSet.keys, compiledCode, loadedSegment, varSet.changeOriginal);
    }

    void compileReturn(goodLine &oneLine) {
        basicFunction* currentFunq = currentFunctionList.back();
        goodFoundSegment segmentPart = findSegmenter(oneLine.oneLine);
        currentFunq->getReturnCMD(createParsedSegment(currentFunq->outputType->myTypee, currentFunq->mySegmentManager,
                segmentPart.oneLinePos, oneLine.oneLine, compiledCode, currentFunq->myVarHandlerer, funqHandler), compiledCode);
    }

    void compileContinuation(astNodeNames &lineType) {
        if (lineType == astNodeNames::breaker) {
            currentFunctionList.back()->mySegmentManager.callBreak(compiledCode);
        }

        if (lineType == astNodeNames::continuer) {
            currentFunctionList.back()->mySegmentManager.callContinue(compiledCode);
        }
    }

    void compilePureCall(goodLine &oneLine) {
        goodFoundSegment segmentPart = findSegmenter(oneLine.oneLine);
        createParsedSegment(typeHandler.findType(numberKey, 1), currentFunctionList.back()->mySegmentManager,
                segmentPart.oneLinePos, oneLine.oneLine, compiledCode, currentFunctionList.back()->myVarHandlerer, funqHandler);
    }

    errorCode compile() {
        //nejdriv vytvorime zakladni funkce main, ktera bude vse obklopovat nic nevracet, a nebude mit zadne vstupni parametry pro ted
        funqHandler.functionList.push_back({});
        currentFunctionList.push_back(&funqHandler.functionList.back());
        currentFunctionList.back()->mySegmentManager.createNewScope(false, false, compiledCode, {});
        int lineNum = 0;
        while (true) {
            goodLine oneLine = loadNextLine(originCode, lineNum);
            currentFunctionList.back()->mySegmentManager.endLine();
            //chybejici strednik
            if (oneLine.problem) {
                return {true, lineNum, -1, "Na konci radku ti chybi ukoncovac radku "};
            }

            //konec programu
            if (!oneLine.exists) {
                break;
            }

            astNodeNames lineType = decideLineType(oneLine.oneLine);

            if (lineType == astNodeNames::segmentAvg || lineType == astNodeNames::segmentCyclic ||
                lineType == astNodeNames::segmentConditional || lineType == astNodeNames::segmentEnder) {
                compileSegment(oneLine, lineType);
            }

            if (lineType == astNodeNames::declareVar || lineType == astNodeNames::declareType || lineType == astNodeNames::declareFunction) {
                compileDeclaration(oneLine, lineType, lineNum);
            }

            if (lineType == astNodeNames::setVar) {
                compileSetting(oneLine);
            }

            if (lineType == astNodeNames::returner) {
                compileReturn(oneLine);
            }

            if (lineType == astNodeNames::continuer || lineType == astNodeNames::breaker) {
                compileContinuation(lineType);
            }

            if (lineType == astNodeNames::callFunq) {
                compilePureCall(oneLine);
            }

            lineNum += 1;
        }
        return {false};
    }
};
