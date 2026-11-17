#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include "../syntax.cpp"
#include "./codeGenerator.cpp"

//pro tenhle soubor poradi: move a set:
//mov: odkud, kam,
//a u set: kam, co


enum class varVarCopyTable {
    pointerToPointer = 1,
    pointerToValue = 2,
    valueToPointer = 3,
    valueToValue = 4
};

enum class inReg {
    reg1 = 1,
    reg2 = 2
};


inReg inRegOposite(inReg arg) {
    if (arg == inReg::reg1) {
        return inReg::reg2;
    }
    return inReg::reg1;
}


std::string inRegToString(inReg inpt) {
    if (inpt == inReg::reg1) {
        return asmSyntax.reg1;
    }
    return asmSyntax.reg2;
}


struct place {
    int size;
    int pointer = -1;
    bool isRef; // mluvi o obsahu
    bool isInReg;
    inReg regLoc;

    void turnToGlobal(inReg desiredReg, fullCompiledCode &code) {
        if (isInReg) {
            code.addInstruction(asmSyntax.movRegReg16 + inRegToString(regLoc) + inRegToString(desiredReg));
            return;
        }

        code.addInstruction(asmSyntax.setReg + asmSyntax.regStackOper + std::to_string(pointer));
        code.addInstruction(asmSyntax.addRegNoCarry + asmSyntax.regStackPointer + asmSyntax.regStackOper + inRegToString(desiredReg));
    }
};


//hloupa funkce kterou ovlada copyFromTo, kopiruje podle adres a to veci z reg1 do reg2
//bere "tabulku"
void copyByTable(varVarCopyTable inpt, int toSize, fullCompiledCode &code, inReg regFrom, inReg regTo, bool changeOriginal, int fromSize) {
    if (inpt == varVarCopyTable::pointerToPointer) {
        code.addInstruction(asmSyntax.movRamRam16 + inRegToString(regFrom) + inRegToString(regTo));
        return;
    }

    if (inpt == varVarCopyTable::pointerToValue) {
        fromSize = toSize;
        code.addInstruction(asmSyntax.movRamReg16 + inRegToString(regFrom) + asmSyntax.regStackOper);
        while (true) {
            code.addInstruction(asmSyntax.movRamRam + asmSyntax.regStackOper + inRegToString(regTo));
            toSize -= 1;
            fromSize -= 1;
            if (toSize <= 0) {
                return;
            }
            if (fromSize <= 0) {
                code.addInstruction(asmSyntax.movRamReg + asmSyntax.regStackOper + asmSyntax.reg5);
                code.addInstruction(asmSyntax.setBySign + asmSyntax.reg5 + asmSyntax.signReg1);
                code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
                break;
            }
            code.addInstruction(asmSyntax.incrementReg + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
        }
        while (true) {
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.signReg1 + inRegToString(regTo));
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
            toSize -= 1;
            if (toSize <= 0) {
                return;
            }
        }
    }

    if (inpt == varVarCopyTable::valueToPointer) {
        if (changeOriginal) {
            code.addInstruction(asmSyntax.movRamReg16 + inRegToString(regTo) + asmSyntax.regStackOper);
            while (true) {
                code.addInstruction(asmSyntax.movRamRam + inRegToString(regFrom) + asmSyntax.regStackOper);
                toSize -= 1;
                fromSize -= 1;
                if (toSize <= 0) {
                    return;
                }
                if (fromSize <= 0) {
                    code.addInstruction(asmSyntax.movRamReg + inRegToString(regFrom) + asmSyntax.reg5);
                    code.addInstruction(asmSyntax.setBySign + asmSyntax.reg5 + asmSyntax.signReg1);
                    code.addInstruction(asmSyntax.incrementReg + asmSyntax.regStackOper);
                    break;
                }
                code.addInstruction(asmSyntax.incrementReg + asmSyntax.regStackOper);
                code.addInstruction(asmSyntax.incrementReg + inRegToString(regFrom));
            }
            while (true) {
                code.addInstruction(asmSyntax.movRegRam + asmSyntax.signReg1 + asmSyntax.regStackOper);
                code.addInstruction(asmSyntax.incrementReg + asmSyntax.regStackOper);
                toSize -= 1;
                if (toSize <= 0) {
                    return;
                }
            }
        }
        code.addInstruction(asmSyntax.movRegRam16 + inRegToString(regFrom) + inRegToString(regTo));
        return;
    }

    if (inpt == varVarCopyTable::valueToValue) {
        while (true) {
            code.addInstruction(asmSyntax.movRamRam + inRegToString(regFrom) + inRegToString(regTo));
            toSize -= 1;
            fromSize -= 1;
            if (toSize <= 0) {
                return;
            }
            if (fromSize <= 0) {
                code.addInstruction(asmSyntax.movRamReg + inRegToString(regFrom) + asmSyntax.reg5);
                code.addInstruction(asmSyntax.setBySign + asmSyntax.reg5 + asmSyntax.signReg1);
                code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
                break;
            }
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regFrom));
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
        }
        while (true) {
            code.addInstruction(asmSyntax.movRegRam + asmSyntax.signReg1 + inRegToString(regTo));
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
            toSize -= 1;
            if (toSize <= 0) {
                return;
            }
        }
        return;
    }
}


varVarCopyTable varsToTable(place &from, place &to) {
    if (from.isRef) {
        if (to.isRef) {
            return varVarCopyTable::pointerToPointer;
        }
        return varVarCopyTable::pointerToValue;
    }

    if (to.isRef) {
        return varVarCopyTable::valueToPointer;
    }

    return varVarCopyTable::valueToValue;
}


bool copyFromTo(place from, place to, fullCompiledCode &code, bool pointingMode) {
    if ((!to.isRef && to.pointer == -2) || (!from.isRef && from.pointer == -2)) {
        //nullptr nic nekopirujeme, nebo void, taky nic nekopirujeme
        return false;
    }
    if ((from.isInReg && to.isInReg) && (from.regLoc == to.regLoc)) {
        //jsou ve stejnem hrnicku !!!
        return false;
    }
    
    int toSize = to.size;
    if (pointingMode) {
        //davame duveru uzivateli
        toSize = from.size;
    }

    if (from.isInReg) {
        if (to.isInReg) {
            copyByTable(varsToTable(from, to), toSize, code, from.regLoc, to.regLoc, pointingMode, from.size);
            return true;
        }
        to.turnToGlobal(inRegOposite(from.regLoc), code);
        copyByTable(varsToTable(from, to), toSize, code, from.regLoc, inRegOposite(from.regLoc), pointingMode, from.size);
        return true;
    }

    if (to.isInReg) {
        from.turnToGlobal(inRegOposite(to.regLoc), code);
        copyByTable(varsToTable(from, to), toSize, code, inRegOposite(to.regLoc), to.regLoc, pointingMode, from.size);
        return true;
    }

    from.turnToGlobal(inReg::reg1, code);
    to.turnToGlobal(inReg::reg2, code);
    copyByTable(varsToTable(from, to), toSize, code, inReg::reg1, inReg::reg2, pointingMode, from.size);

    return true;
}


bool setRamLocal(int number, int localPos, int size, fullCompiledCode &code) {
    if (localPos > 65537 || size > 65537) {
        return false;
    }
    for (int i = 0; i < size; i++) {
        code.addInstruction(asmSyntax.addRegNoCarry + asmSyntax.regStackPointer + std::to_string(localPos + i) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.setRam8 + asmSyntax.regStackOper + std::to_string((number >> (i*8)) & 0xFF));
    }
    return true;
}
