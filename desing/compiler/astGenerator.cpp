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

    void compileSegment(goodLine &oneLine, astNodeNames &lineType) {
        basicFunction* currentFunq = currentFunctionList.back();
        if (lineType == astNodeNames::segmentEnder) {
            //kdyz je poslednis scope, tak je konec definice funkce
            if (currentFunq->mySegmentManager.isLastScope()) {
                currentFunq->endDefinition(compiledCode);
                currentFunctionList.erase(currentFunctionList.end());
                return;
            }
            currentFunq->mySegmentManager.stopCurScope(currentFunq->myVarHandlerer, compiledCode);
            return;
        }

        else {
            if (lineType == astNodeNames::segmentAvg) {
                currentFunq->mySegmentManager.createNewScope(false, false, compiledCode);
            }
            if (lineType == astNodeNames::segmentConditional) {
                currentFunq->mySegmentManager.createNewScope(true, false, compiledCode);
            }
            if (lineType == astNodeNames::segmentCyclic) {
                currentFunq->mySegmentManager.createNewScope(false, true, compiledCode);
            }
        }
    }

    void compileDeclaration(goodLine &oneLine, astNodeNames &lineType) {

    }

    void compileSetting(goodLine &oneLine) {

    }

    void compileReturn(goodLine &oneLine) {

    }

    void compileContinuation(goodLine &oneLine, astNodeNames &lineType) {

    }

    void compilePureCall(goodLine &oneLine) {

    }

    errorCode compile() {
        //nejdriv vytvorime zakladni funkce main, ktera bude vse obklopovat nic nevracet, a nebude mit zadne vstupni parametry pro ted
        funqHandler.functionList.push_back({});
        int lineNum = 0;
        while (true) {
            goodLine oneLine = loadNextLine(originCode, lineNum);

            //chybejici strednik
            if (oneLine.problem) {
                return {true, lineNum, -1, "Na konci radku ti chybi ukoncovac radku"};
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
                compileDeclaration(oneLine, lineType);
            }

            if (lineType == astNodeNames::setVar) {
                compileSetting(oneLine);
            }

            if (lineType == astNodeNames::returner) {
                compileReturn(oneLine);
            }

            if (lineType == astNodeNames::continuer || lineType == astNodeNames::breaker) {
                compileContinuation(oneLine, lineType);
            }

            if (lineType == astNodeNames::callFunq) {
                compilePureCall(oneLine);
            }

            lineNum += 1;
        }
        return {false};
    }
};
