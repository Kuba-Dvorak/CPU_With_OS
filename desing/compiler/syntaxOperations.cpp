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

    if (placeTo.size <= (std::min(numB.size, numA.size))) {
        bool oddSize = false;
        if ((placeTo.size % 2) != 0) {
            oddSize = true;
        }

        for (int i = 0; i < placeTo.size; i++) {
            if (oddSize && ((placeTo.size - i) == 1)) {
                break;
            }
            transferAddr(numA.pointer, code);
            code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + asmSyntax.reg1);
            transferAddr(numB.pointer, code);
            code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + asmSyntax.reg2);
            code.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
            transferAddr(placeTo.pointer, code);
            code.addInstruction(asmSyntax.movRegRam16 + asmSyntax.reg5 + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
            placeTo.pointer += 2;
            numA.pointer += 2;
            numB.pointer += 2;
        }

        if (oddSize) {
            transferAddr(numA.pointer, code);
            code.addInstruction(asmSyntax.movRamReg + asmSyntax.regStackOper + asmSyntax.reg1);
            transferAddr(numB.pointer, code);
            code.addInstruction(asmSyntax.movRamReg + asmSyntax.regStackOper + asmSyntax.reg2);
            code.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
            transferAddr(placeTo.pointer, code);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
            placeTo.pointer += 1;
            numA.pointer += 1;
            numB.pointer += 1;
        }

        return;
    }

    if (placeTo.size <= (std::max(numB.size, numA.size))) {

        if (numA.size > numB.size) {
            bool oddSize = false;
            if ((numB.size % 2) != 0) {
                oddSize = true;
            }

            for (int i = 0; i < numB.size; i++) {
                if (oddSize && ((numB.size - i) == 1)) {
                    break;
                }
                transferAddr(numA.pointer, code);
                code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + asmSyntax.reg1);
                transferAddr(numB.pointer, code);
                code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.regStackOper + asmSyntax.reg2);
                code.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
                transferAddr(placeTo.pointer, code);
                code.addInstruction(asmSyntax.movRegRam16 + asmSyntax.reg5 + asmSyntax.regStackOper);
                code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
                placeTo.pointer += 2;
                numA.pointer += 2;
                numB.pointer += 2;
            }

            if (oddSize) {
                transferAddr(numA.pointer, code);
                code.addInstruction(asmSyntax.movRamReg + asmSyntax.regStackOper + asmSyntax.reg1);
                transferAddr(numB.pointer, code);
                code.addInstruction(asmSyntax.movRamReg + asmSyntax.regStackOper + asmSyntax.reg2);
                code.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
                transferAddr(placeTo.pointer, code);
                code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
                code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
                placeTo.pointer += 1;
                numA.pointer += 1;
                numB.pointer += 1;
            }
            //after that just moving

            return;
        }

        //proste obracene
        return;
    }

    if (placeTo.size > (std::max(numB.size, numA.size))) {
        if (numA.size > numB.size) {

            return;
        }

        return;
    }
}


//nasobime po 1 bajtu => nasobicka udela z 2 8 bit cise 1 16bitove a vic registry nepoberou
void multiplyPlaces(place numA, place numB, place placeTo) {

}


