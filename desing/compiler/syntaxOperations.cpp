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


void comparePlaces(place numA, place numB, place placeTo, fullCompiledCode &code, operators oper, segmentManager &scope) {
    place whereIsSub = {std::max(numA.size, numB.size) + 1, scope.reqFreeSub(std::max(numA.size, numB.size) + 1), false, false, inReg::reg1};
    if (oper == operators::lesser || oper == operators::ebig) {
        substitudePlaces(numA, numB, whereIsSub, code);
        loadAddr(whereIsSub.pointer + whereIsSub.size - 1, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.lookAtSign + asmSyntax.reg1);
        if (oper == operators::ebig) {
            code.addInstruction(asmSyntax.negateBit + asmSyntax.latchCompare + asmSyntax.reg2);
            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + asmSyntax.regStackOper);
            return;
        }
        transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.latchCompare + asmSyntax.regStackOper);
        return;
    }

    if (oper == operators::bigger || oper == operators::esml) {
        substitudePlaces(numB, numA, whereIsSub, code);
        loadAddr(whereIsSub.pointer + whereIsSub.size - 1, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.lookAtSign + asmSyntax.reg1);
        if (oper == operators::esml) {
            code.addInstruction(asmSyntax.negateBit + asmSyntax.latchCompare + asmSyntax.reg2);
            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + asmSyntax.regStackOper);
            return;
        }
        transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.latchCompare + asmSyntax.regStackOper);
        return;
    }

    if (oper == operators::equal || oper == operators::notEqual) {
        //tady musime projet cele cislo a divat se na 0 a potom to andovat
        substitudePlaces(numA, numB, whereIsSub, code);
        transferAddr(whereIsSub.pointer, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.setReg + asmSyntax.reg2 + "1");

        bool isInReg2 = true;
        for (int i = 0; i < whereIsSub.size; i++) {
            code.addInstruction(asmSyntax.movRamReg + asmSyntax.reg1 + asmSyntax.reg0);
            code.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1 + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegReg16 + asmSyntax.regStackOper + asmSyntax.reg1);
            code.addInstruction(asmSyntax.isEqual + asmSyntax.reg0);
            if (isInReg2) {
                code.addInstruction(asmSyntax.and8 + asmSyntax.reg2 + asmSyntax.latchCompare + asmSyntax.reg6);
                isInReg2 = false;
            }

            else {
                code.addInstruction(asmSyntax.and8 + asmSyntax.reg6 + asmSyntax.latchCompare + asmSyntax.reg2);
                isInReg2 = true;
            }
        }

        if (oper == operators::notEqual) {
            if (isInReg2) {
                code.addInstruction(asmSyntax.negateBit + asmSyntax.reg2 + asmSyntax.reg6);
                isInReg2 = false;
            }

            else {
                code.addInstruction(asmSyntax.negateBit + asmSyntax.reg6 + asmSyntax.reg2);
                isInReg2 = true;
            }
        }

        if (isInReg2) {
            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + asmSyntax.regStackOper);
        }

        else {
            transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg6 + asmSyntax.regStackOper);
        }

    }
}


void oneDeshift(place plac, fullCompiledCode &code, int toWhichPoint) {
    code.addInstruction(asmSyntax.setReg + asmSyntax.latchShiftLowest + "0");
    for (int i = plac.size - 1; i > toWhichPoint; i--) {
        loadAddr(plac.pointer + i, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.putToReg + asmSyntax.reg2);
        code.addInstruction(asmSyntax.shiftLowBy1 + asmSyntax.reg1 + asmSyntax.reg2);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + asmSyntax.reg0);
    }
}


void saveSing(place &num, place &placeTo, fullCompiledCode &code) {
    loadAddr(num.pointer + num.size - 1, code, asmSyntax.reg1);
    code.addInstruction(asmSyntax.lookAtSign + asmSyntax.reg1);
    transferAddr(placeTo.pointer, code, asmSyntax.reg2);
    code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg1 + asmSyntax.reg2);
}


//musi nasledovat hned po saveSing
void makePos(place &num, fullCompiledCode &code) {
    code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchCompare + asmSyntax.skippingLatch);
    int currentInstr = code.currentLine;
    code.addInstruction(asmSyntax.jumpDictate);
    code.addInstruction(asmSyntax.setReg + asmSyntax.latchALUCarryI + "0");
    code.addInstruction(asmSyntax.setReg + asmSyntax.reg2 + "255");
    for (int i = 0; i < num.size; i++) {
        loadAddr(num.pointer + i, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.addReg8 + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg6);
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
        code.addInstruction(asmSyntax.negate8 + asmSyntax.reg6 + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.regStackOper + asmSyntax.reg0);
    }
    code.changeBackFunq(asmSyntax.jumpDictate + std::to_string(code.currentLine), currentInstr);
}


//celociselne deleni A a B, 0/0 = same 1
void dividePlaces(place numA, place numB, place placeTo, fullCompiledCode &code, segmentManager &scope) {
    cleanPlace(placeTo, code);
    place numASignOr = {1, scope.reqFreeSub(1), false, false, inReg::reg1};
    saveSing(numA, numASignOr, code);
    //transformace na kladne cislo, nejdriv si ulozime nynejsi instrukci, abychom to pak kdyztak preskocili a potom upravili:
    makePos(numA, code);


    place numBSignOr = {1, scope.reqFreeSub(1), false, false, inReg::reg1};
    saveSing(numB, numBSignOr, code);
    //transformace na kladne cislo, nejdriv si ulozime nynejsi instrukci, abychom to pak kdyztak preskocili a potom upravili:
    makePos(numB, code);

    //ted prichazi na praci realne deleni
    //v tomhle miste budeme mit nase cislo a budeme ho postupne zmensovat
    place deshiftingPlace = {numB.size + numA.size, scope.reqFreeSub(numB.size + numA.size), false, false, inReg::reg1};
    cleanPlace(deshiftingPlace, code);
    copyFromTo(numB, {numB.size, deshiftingPlace.pointer + numA.size, false, false, inReg::reg1}, code, false);
    place testingPlace = {numB.size + numA.size, scope.reqFreeSub(numB.size + numA.size), false, false, inReg::reg1};
    cleanPlace(testingPlace, code);
    copyFromTo(numA, testingPlace, code, true);

    for (int i = numA.size - 1; i >= 0; i--) {
        code.addInstruction(asmSyntax.setReg + asmSyntax.reg6 + "0");
        code.addInstruction(asmSyntax.setReg + asmSyntax.reg7 + "128");

        int startPos = code.currentLine;
        oneDeshift(deshiftingPlace, code, i - 1);
        substitudePlaces(testingPlace, deshiftingPlace, testingPlace, code);
        loadAddr(testingPlace.pointer + testingPlace.size - 1, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.lookAtSign + asmSyntax.reg1);
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchCompare + asmSyntax.skippingLatch);

        int skippingInstr = code.currentLine;
        //kdyz je to zaporne tak vratime testingPlace na hodnotu numA
        code.addInstruction(asmSyntax.jumpDictate);
        cleanPlace(testingPlace, code);
        copyFromTo(numA, testingPlace, code, true);
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.reg7 + asmSyntax.reg2);
        code.addInstruction(asmSyntax.shiftLowBy1 + asmSyntax.reg2 + asmSyntax.reg7);
        int skippingInstr2 = code.currentLine;
        code.addInstruction(asmSyntax.jumpDictate);

        //jinak kopirujeme prave z testingPlace zpatky do numA
        code.changeBackFunq(asmSyntax.jumpDictate + std::to_string(code.currentLine), skippingInstr);
        copyFromTo(testingPlace, numA, code, false);
        code.addInstruction(asmSyntax.or8 + asmSyntax.reg6 + asmSyntax.reg7 + asmSyntax.reg0);
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.reg0 + asmSyntax.reg6);

        code.addInstruction(asmSyntax.movRegReg + asmSyntax.reg7 + asmSyntax.reg2);
        code.addInstruction(asmSyntax.shiftLowBy1 + asmSyntax.reg2 + asmSyntax.reg7);
        code.changeBackFunq(asmSyntax.jumpDictate + std::to_string(code.currentLine), skippingInstr2);
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.reg2 + asmSyntax.skippingLatch);
        code.addInstruction(asmSyntax.jumpDictate + std::to_string(startPos));

        if (i < placeTo.size) {
            transferAddr(placeTo.pointer + i, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg6 + asmSyntax.regStackOper);
        }

        deshiftingPlace.size -= 1;
    }

    //znamenko potom je vysledek XORu zn1 a zn2
    loadAddr(numASignOr.pointer, code, asmSyntax.reg1);
    loadAddr(numBSignOr.pointer, code, asmSyntax.reg2);
    code.addInstruction(asmSyntax.xorReg + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg6);
    code.addInstruction(asmSyntax.lookAtSign + asmSyntax.reg6);
    code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchCompare + asmSyntax.skippingLatch);
    //pokud je to 0, tak se to neskipne => ted musime udelat cislo zaporne
    int changeInstr = code.currentLine;
    code.addInstruction(asmSyntax.jumpDictate);
    code.addInstruction(asmSyntax.setReg + asmSyntax.latchALUCarryI + "1");
    for (int i = 0; i < placeTo.size; i++) {
        loadAddr(placeTo.pointer + i, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.negate8 + asmSyntax.reg1 + asmSyntax.reg2);
        code.addInstruction(asmSyntax.addReg8 + asmSyntax.reg2 + asmSyntax.regNone + asmSyntax.reg1);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg1 + asmSyntax.reg0);
        code.addInstruction(asmSyntax.movRegReg + asmSyntax.latchALUCarryO + asmSyntax.latchALUCarryI);
    }
    code.changeBackFunq(asmSyntax.jumpDictate + std::to_string(code.currentLine), changeInstr);
}


std::string logOperToString(logOperators oper) {
    if (oper == logOperators::andOp) {
        return asmSyntax.and8;
    }
    if (oper == logOperators::notByteOp) {
        return asmSyntax.negate8;
    }
    if (oper == logOperators::xorOp) {
        return asmSyntax.xorReg;
    }
    if (oper == logOperators::orOp) {
        return asmSyntax.or8;
    }
}


//pro negaci staci vyplnit numA
void logicalOperation(place numA, place numB, place placeTo, fullCompiledCode &code, logOperators opers) {
    std::string operation = logOperToString(opers);
    cleanPlace(placeTo, code);
    if (opers == logOperators::notBitOp) {
        loadAddr(numA.pointer, code, asmSyntax.reg1);
        code.addInstruction(asmSyntax.setReg + asmSyntax.reg2 + "0");
        code.addInstruction(asmSyntax.negateBit + asmSyntax.reg1 + asmSyntax.reg2);
        transferAddr(placeTo.pointer, code, asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + asmSyntax.regStackOper);
    }

    else if (opers == logOperators::notByteOp) {
        int iterator = 0;
        while (true) {
            if (iterator >= numA.size) {
                break;
            }

            if (iterator >= placeTo.size) {
                break;
            }
            loadAddr(numA.pointer + iterator, code, asmSyntax.reg1);
            code.addInstruction(operation + asmSyntax.reg1 + asmSyntax.reg2);
            transferAddr(placeTo.pointer + iterator, code, asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + asmSyntax.regStackOper);
            iterator += 1;
        }
    }

    else {
        int iterator = 0;
        bool justA = false;
        bool justB = false;
        code.addInstruction(asmSyntax.setReg + asmSyntax.reg6 + "0");
        while (true) {
            if (iterator >= numA.size) {
                if (opers == logOperators::andOp) {
                    break;
                }
                justB = true;
            }

            if (iterator >= numB.size) {
                if (opers == logOperators::andOp) {
                    break;
                }
                justA = true;
            }

            if (justA && justB) {
                break;
            }

            if (iterator >= placeTo.size) {
                break;
            }

            if (justA) {
                loadAddr(numA.pointer + iterator, code, asmSyntax.reg1);
                code.addInstruction(operation + asmSyntax.reg1 + asmSyntax.reg6 + asmSyntax.reg5);
                transferAddr(placeTo.pointer + iterator, code, asmSyntax.regStackOper);
                code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
            }

            else if (justB) {
                loadAddr(numB.pointer + iterator, code, asmSyntax.reg1);
                code.addInstruction(operation + asmSyntax.reg1 + asmSyntax.reg6 + asmSyntax.reg5);
                transferAddr(placeTo.pointer + iterator, code, asmSyntax.regStackOper);
                code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
            }

            else {
                loadAddr(numA.pointer + iterator, code, asmSyntax.reg1);
                loadAddr(numB.pointer + iterator, code, asmSyntax.reg2);
                code.addInstruction(operation + asmSyntax.reg1 + asmSyntax.reg2 + asmSyntax.reg5);
                transferAddr(placeTo.pointer + iterator, code, asmSyntax.regStackOper);
                code.addInstruction(asmSyntax.movRegRam + asmSyntax.reg5 + asmSyntax.regStackOper);
            }


            iterator += 1;
        }
    }
}


