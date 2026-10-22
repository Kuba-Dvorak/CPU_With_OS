#include <cmath>
#include <vector>
#include <string>
#include <variant>
#include <array>
#include "./lexicalAnalyzer.cpp"


void transferAddr(int locPointer, fullCompiledCode &code) {
    code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + std::to_string(locPointer) + asmSyntax.regStackOper);
}


//mistoA a mistoB uz nejsou v registru (definitivne)
void addPlaces(place numA, place numB, place placeTo, fullCompiledCode &code) {
    //musime to brat pojednom (pro optimalizaci po 2 bajtech a pak scitat
    std::string movCur = asmSyntax.movRamReg16;
    std::string movCurBack = asmSyntax.movRegRam16;

    int increaser = 2;
    bool movingMode = false;
    int movingPointer = 0;

    while (true) {

        //sudy nebo lichy
        if (placeTo.size == 1 || numA.size == 1 || numB.size == 1) {
            increaser = 1;
            std::string movCur = asmSyntax.movRamReg;
            std::string movCurBack = asmSyntax.movRegRam;
        }

        else {
            std::string movCur = asmSyntax.movRamReg16;
            std::string movCurBack = asmSyntax.movRegRam16;
            int increaser = 2;
        }

        if (numA.size <= 0 && numB.size <= 0) {
            break;
        }

        if (placeTo.size <= 0) {
            return;
        }


        if (numA.size <= 0) {
            movingMode = true;
            movingPointer = numA.pointer;
        }

        if (numB.size <= 0) {
            movingMode = true;
            movingPointer = numB.pointer;
        }


        if (movingMode) {
            transferAddr(movingPointer, code);
            code.addInstruction(movCur + asmSyntax.regStackOper + asmSyntax.reg1);
            code.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.regNone + asmSyntax.reg5);
            transferAddr(placeTo.pointer, code);
            code.addInstruction(movCurBack + asmSyntax.reg5 + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
        }

        else {
            transferAddr(numA.pointer, code);
            code.addInstruction(movCur + asmSyntax.regStackOper + asmSyntax.reg1);
            transferAddr(numB.pointer, code);
            code.addInstruction(movCur + asmSyntax.regStackOper + asmSyntax.reg2);
            code.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
            transferAddr(placeTo.pointer, code);
            code.addInstruction(movCurBack + asmSyntax.reg5 + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
        }

        placeTo.pointer += increaser;
        numA.pointer += increaser;
        numB.pointer += increaser;
        placeTo.size -= increaser;
        numA.size -= increaser;
        numB.size -= increaser;
    }

    //dat increment
    transferAddr(placeTo.pointer, code);
    code.addInstruction(movCurBack + asmSyntax.latchALUCarryI + asmSyntax.regStackOper);
}


//nasobime po 1 bajtu => nasobicka udela z 2 8 bit cise 1 16bitove a vic registry nepoberou
void multiplyPlaces(place numA, place numB, place placeTo) {

}


