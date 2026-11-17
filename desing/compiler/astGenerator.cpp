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

    void compileSegment() {

    }

    void compileDeclaration() {

    }

    void compileSetting() {

    }

    void compileReturn() {

    }

    void compileContinuation() {

    }

    errorCode compile() {
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

            if (lineType == astNodeNames::segmentAvg || lineType == astNodeNames::segmentCyclic || lineType == astNodeNames::segmentConditional || lineType == astNodeNames::segmentEnder) {
                compileSegment();
            }


            lineNum += 1;
        }
        return {false};
    }
};