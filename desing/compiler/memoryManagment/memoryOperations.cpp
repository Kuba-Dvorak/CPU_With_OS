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
        code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + asmSyntax.regStackOper + inRegToString(desiredReg));
    }
};


//hloupa funkce kterou ovlada copyFromTo, kopiruje podle adres a to veci z reg1 do reg2
//bere "tabulku"
void copyByTable(varVarCopyTable inpt, int toSize, fullCompiledCode &code, inReg regFrom, inReg regTo) {
    if (inpt == varVarCopyTable::pointerToPointer) {
        code.addInstruction(asmSyntax.movRamRam16 + inRegToString(regFrom) + inRegToString(regTo));
        return;
    }

    if (inpt == varVarCopyTable::pointerToValue) {
        code.addInstruction(asmSyntax.movRamReg16 + inRegToString(regFrom) + asmSyntax.regStackOper);
        for (int i = 0; i < toSize; i++) {
            code.addInstruction(asmSyntax.movRamRam + asmSyntax.regStackOper + inRegToString(regTo));
            code.addInstruction(asmSyntax.incrementReg + asmSyntax.regStackOper);
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
        }
        return;
    }

    if (inpt == varVarCopyTable::valueToPointer) {
        code.addInstruction(asmSyntax.movRegRam16 + inRegToString(regFrom) + inRegToString(regTo));
        return;
    }

    if (inpt == varVarCopyTable::valueToValue) {
        for (int i = 0; i < toSize; i++) {
            code.addInstruction(asmSyntax.movRamRam + inRegToString(regFrom) + inRegToString(regTo));
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regFrom));
            code.addInstruction(asmSyntax.incrementReg + inRegToString(regTo));
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


bool copyFromTo(place from, place to, fullCompiledCode &code) {
    if ((!to.isRef && to.pointer == -1) || (!from.isRef && from.pointer == -1)) {
        //nullptr nic nekopirujeme, nebo void, taky nic nekopirujeme
        return false;
    }
    if ((from.isInReg && to.isInReg) && (from.regLoc == to.regLoc)) {
        //jsou ve stejnem hrnicku !!!
        return false;
    }

    if (from.isInReg) {
        if (to.isInReg) {
            copyByTable(varsToTable(from, to), to.size, code, from.regLoc, to.regLoc);
            return true;
        }
        to.turnToGlobal(inRegOposite(from.regLoc), code);
        copyByTable(varsToTable(from, to), to.size, code, from.regLoc, inRegOposite(from.regLoc));
        return true;
    }

    if (to.isInReg) {
        from.turnToGlobal(inRegOposite(to.regLoc), code);
        copyByTable(varsToTable(from, to), to.size, code, inRegOposite(to.regLoc), to.regLoc);
        return true;
    }
    from.turnToGlobal(inReg::reg1, code);
    to.turnToGlobal(inReg::reg2, code);
    copyByTable(varsToTable(from, to), to.size, code, inReg::reg1, inReg::reg2);

    return true;
}


bool setRamLocal(int number, int localPos, int size, fullCompiledCode &code) {
    if (localPos > 65537 || size > 65537) {
        return false;
    }
    for (int i = 0; i < size; i++) {
        code.addInstruction(asmSyntax.addReg + asmSyntax.regStackPointer + std::to_string(localPos + i) + asmSyntax.regStackOper);
        code.addInstruction(asmSyntax.setRam8 + asmSyntax.regStackOper + std::to_string((number >> (i*8)) & 0xFF));
    }
    return true;
}
