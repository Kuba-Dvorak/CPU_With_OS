#include <cmath>
#include <vector>
#include <string>
#include <variant>
#include <array>
#include "./lexicalAnalyzer.cpp"


void transferAddr(int locPointer, fullCompiledCode &code, std::string toWhere) {
    code.addInstruction(asmSyntax.addRegNoCarry + asmSyntax.regStackPointer + std::to_string(locPointer) + toWhere);
}


void loadAddr(int locPointer, fullCompiledCode &code, std::string toWhere) {
    code.addInstruction(asmSyntax.addRegNoCarry + asmSyntax.regStackPointer + std::to_string(locPointer) + asmSyntax.reg0);
    code.addInstruction(asmSyntax.movRamReg + asmSyntax.reg0 + toWhere);
}


void loadAddr16(int locPointer, fullCompiledCode &code, std::string toWhere) {
    code.addInstruction(asmSyntax.addRegNoCarry + asmSyntax.regStackPointer + std::to_string(locPointer) + asmSyntax.reg0);
    code.addInstruction(asmSyntax.movRamReg16 + asmSyntax.reg0 + toWhere);
}


void cleanPlace(place cleaningRealm, fullCompiledCode &code) {
    for (int i = 0; i < cleaningRealm.size; i++) {
        setRamLocal(0, cleaningRealm.pointer + i, 1, code);
    }
}


void goThroughCarrying(int fromPos, int sizer, fullCompiledCode &code) {
    for (int i = 0; i < sizer; i++) {
        code.addInstruction(asmSyntax.addRegNoCarry + asmSyntax.regStackPointer + std::to_string(fromPos + i) + asmSyntax.reg0);
        code.addInstruction(asmSyntax.movRamReg + asmSyntax.reg0 + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.addReg8 + asmSyntax.regStackOper + asmSyntax.regNone + asmSyntax.reg6);
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg6 + asmSyntax.reg0);
    }
}


//mistoA a mistoB uz nejsou v registru (definitivne)
//vsechny cisla jsou signed
void addPlaces(place numA, place numB, place placeTo, fullCompiledCode &code) {
    code.addInstruction(asmSyntax.setReg + asmSyntax.signReg1 + "0");
    code.addInstruction(asmSyntax.setReg + asmSyntax.signReg2 + "0");
    code.addInstruction(asmSyntax.setReg + asmSyntax.latchALUCarryI + "0");
    bool numAFirst = false;
    bool numBFirst = false;

    while (true) {
        if (placeTo.size <= 0) {
            return;
        }

        if (numA.size > 0 && numB.size > 0) {
            loadAddr(numA.pointer, code, asmSyntax.reg1);
            loadAddr(numB.pointer, code, asmSyntax.reg2);

            code.addInstruction(asmSyntax.addReg8 + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);

            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);

            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }

        else if (numA.size > 0) {
            loadAddr(numA.pointer, code, asmSyntax.reg1);
            code.addInstruction(asmSyntax.addReg8 + asmSyntax.reg1 + asmSyntax.signReg2 + asmSyntax.reg5);

            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);

            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }

        else if (numB.size > 0) {
            loadAddr(numB.pointer, code, asmSyntax.reg2);
            code.addInstruction(asmSyntax.addReg8 + asmSyntax.signReg1 + asmSyntax.reg2 + asmSyntax.reg5);

            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);

            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }

        if (numA.size <= 0 && numB.size <= 0) {
            code.addInstruction(asmSyntax.addReg8 + asmSyntax.signReg1 + asmSyntax.signReg2 + asmSyntax.reg5);
            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }


        code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);


        numA.size -= 1;
        numB.size -= 1;
        placeTo.size -= 1;
        numA.pointer += 1;
        numB.pointer += 1;
        placeTo.pointer += 1;

        if (numA.size <= 0 && !numAFirst) {
            code.addInstruction(asmSyntax.setBySign + asmSyntax.reg1 + asmSyntax.signReg1);
            numAFirst = true;
        }

        if (numB.size <= 0 && !numBFirst) {
            code.addInstruction(asmSyntax.setBySign + asmSyntax.reg2 + asmSyntax.signReg2);
            numBFirst = true;
        }
    }
}


void substitudePlaces(place numA, place numB, place placeTo, fullCompiledCode &code) {
    code.addInstruction(asmSyntax.setReg + asmSyntax.signReg1 + "0");
    code.addInstruction(asmSyntax.setReg + asmSyntax.signReg2 + "0");
    code.addInstruction(asmSyntax.setReg + asmSyntax.latchALUCarryI + "1");
    bool numAFirst = false;
    bool numBFirst = false;

    while (true) {
        if (placeTo.size <= 0) {
            return;
        }

        if (numA.size > 0 && numB.size > 0) {
            loadAddr(numA.pointer, code, asmSyntax.reg1);
            loadAddr(numB.pointer, code, asmSyntax.reg2);

            code.addInstruction(asmSyntax.negate8 + asmSyntax.reg2 + asmSyntax.reg0);
            code.addInstruction(asmSyntax.movRegReg + asmSyntax.reg0 + asmSyntax.reg2);
            code.addInstruction(asmSyntax.addReg8 + asmSyntax.reg1 + asmSyntax.reg0 + asmSyntax.reg5);

            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);

            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }

        else if (numA.size > 0) {
            loadAddr(numA.pointer, code, asmSyntax.reg1);
            code.addInstruction(asmSyntax.addReg8 + asmSyntax.reg1 + asmSyntax.signReg2 + asmSyntax.reg5);

            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);

            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }

        else if (numB.size > 0) {
            loadAddr(numB.pointer, code, asmSyntax.reg2);
            code.addInstruction(asmSyntax.negate8 + asmSyntax.reg2 + asmSyntax.reg0);
            code.addInstruction(asmSyntax.movRegReg + asmSyntax.reg0 + asmSyntax.reg2);
            code.addInstruction(asmSyntax.addReg8 + asmSyntax.signReg1 + asmSyntax.reg0 + asmSyntax.reg5);

            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);

            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }

        if (numA.size <= 0 && numB.size <= 0) {
            code.addInstruction(asmSyntax.addReg8 + asmSyntax.signReg1 + asmSyntax.signReg2 + asmSyntax.reg5);
            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        }


        code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);


        numA.size -= 1;
        numB.size -= 1;
        placeTo.size -= 1;
        numA.pointer += 1;
        numB.pointer += 1;
        placeTo.pointer += 1;

        if (numA.size <= 0 && !numAFirst) {
            code.addInstruction(asmSyntax.setBySign + asmSyntax.reg1 + asmSyntax.signReg1);
            numAFirst = true;
        }

        if (numB.size <= 0 && !numBFirst) {
            code.addInstruction(asmSyntax.setBySign + asmSyntax.reg2 + asmSyntax.signReg2);
            numBFirst = true;
        }
    }
}


//velikost numA a numB se musi dopadovat na velikost placeTo
void multiplyPlaces(place numA, place numB, place placeTo, fullCompiledCode &code) {
    int curCountingA = 0;
    int curCountingB = 0;
    bool takeSignA = false;
    bool takeSignB = false;
    cleanPlace(placeTo, code);

    if (placeTo.size == 1) {
        loadAddr(numA.pointer, code, asmSyntax.reg1);
        loadAddr(numB.pointer, code, asmSyntax.reg2);
        transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.multiply8 + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
        return;
    }

    while (true) {
        curCountingA = 0;
        takeSignA = false;
        while (true) {
            if (takeSignB && takeSignA) {
                code.addInstruction(asmSyntax.multiply8 + asmSyntax.signReg1 + asmSyntax.signReg2 + asmSyntax.reg5);
            }

            else if (takeSignA) {
                loadAddr(numB.pointer + curCountingB, code, asmSyntax.reg2);
                code.addInstruction(asmSyntax.multiply8 + asmSyntax.signReg1 + asmSyntax.reg2 + asmSyntax.reg5);
            }

            else if (takeSignB) {
                loadAddr(numA.pointer + curCountingA, code, asmSyntax.reg1);
                code.addInstruction(asmSyntax.multiply8 + asmSyntax.reg1 + asmSyntax.signReg2 + asmSyntax.reg5);
                if (curCountingA + 1 >= numA.size) {
                    takeSignA = true;
                    code.addInstruction(asmSyntax.setBySign + asmSyntax.reg1 + asmSyntax.signReg1);
                }
            }

            if (!takeSignB && !takeSignA) {
                loadAddr(numA.pointer + curCountingA, code, asmSyntax.reg1);
                loadAddr(numB.pointer + curCountingB, code, asmSyntax.reg2);
                code.addInstruction(asmSyntax.multiply8 + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
                if (curCountingA + 1 >= numA.size) {
                    takeSignA = true;
                    code.addInstruction(asmSyntax.setBySign + asmSyntax.reg1 + asmSyntax.signReg1);
                }
            }


            if (!(curCountingA + curCountingB >= placeTo.size - 1)) {
                loadAddr16(placeTo.pointer + curCountingA + curCountingB, code, asmSyntax.reg6);
                code.addInstruction(asmSyntax.setReg + asmSyntax.latchALUCarryI + "0");
                code.addInstruction(asmSyntax.addReg16 + asmSyntax.reg6 + asmSyntax.reg5 + asmSyntax.reg1);
                code.addInstruction(asmSyntax.movRegRam16 + asmSyntax.reg1 + asmSyntax.reg0);
                code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
                goThroughCarrying(placeTo.pointer + curCountingA + curCountingB + 2, placeTo.size - 2 - curCountingB - curCountingA, code);
            }

            else {
                loadAddr(placeTo.pointer + curCountingA + curCountingB, code, asmSyntax.reg6);
                code.addInstruction(asmSyntax.setReg + asmSyntax.latchALUCarryI + "0");
                code.addInstruction(asmSyntax.addReg8 + asmSyntax.reg6 + asmSyntax.reg5 + asmSyntax.reg1);
                code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg1 + asmSyntax.reg0);
            }

            curCountingA += 1;

            if (curCountingA + curCountingB >= placeTo.size) {
                break;
            }
        }


        curCountingB += 1;

        if (curCountingB >= placeTo.size) {
            return;
        }

        if (curCountingB >= numB.size && !takeSignB) {
            takeSignB = true;
            code.addInstruction(asmSyntax.setBySign + asmSyntax.reg2 + asmSyntax.signReg2);
        }
    }
}


void dividePlaces(place numA, place numB, place placeTo, fullCompiledCode &code) {

}

