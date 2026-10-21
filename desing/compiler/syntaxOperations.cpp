#include <cmath>
#include <vector>
#include <string>
#include <variant>
#include "./lexicalAnalyzer.cpp"


//mistoA a mistoB uz nejsou v registru (definitivne)
void addPlaces(place numA, place numB, place placeTo, fullCompiledCode &code) {
    //musime to brat pojednom (pro optimalizaci po 2 bajtech a pak scitat
    std::string mover = asmSyntax.movRamReg16;
    std::string moverBack = asmSyntax.movRegRam16;
    int maxI = std::min(std::min(placeTo.size, numA.size), numB.size);
    for (int i = 0; i < maxI; i += 2) {
        if (((maxI - i) == 1) && (maxI % 2 == 0)) {
            mover = asmSyntax.movRamReg;
            moverBack = asmSyntax.movRegRam;
        }
        code.addInstruction(mover + std::to_string(numA.pointer) + asmSyntax.reg1);
        code.addInstruction(mover + std::to_string(numB.pointer) + asmSyntax.reg2);
        code.addInstruction(asmSyntax.addReg + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.regStackOper);
        code.addInstruction(moverBack + asmSyntax.regStackOper + std::to_string(placeTo.pointer));
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
        placeTo.pointer += 2;
        numA.pointer += 2;
        numB.pointer += 2;
    }

    if (placeTo.size > maxI) {
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.latchALUCarryI + std::to_string(placeTo.pointer));
    }
}


void multiplyPlaces(place numA, place numB, place placeTo) {

}


