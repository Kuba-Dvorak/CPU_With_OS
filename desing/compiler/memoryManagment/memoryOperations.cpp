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


enum class regs {
    reg1 = 1,
    reg2 = 2,
    inRam = -1
};


struct ramPointer {
    std::string locale0;
    int pointer = -1;

    std::string getString() {
        return (" " + locale0 + " " + std::to_string(pointer));
    }
};



struct information {
    regs placement;
    ramPointer pointer;
    int size;
    bool valueIsPointer;
};


//
void copyFromTo(information placeFrom, information placeTo, fullCompiledCode &code) {
    if ((placeFrom.pointer.pointer == -1 && placeFrom.placement == regs::inRam) || (placeTo.pointer.pointer == -1 && placeTo.placement == regs::inRam) ) {
        //pointery nejsou definovane => nullptr nebo void
        return;
    }

    if ((placeFrom.placement != regs::inRam && !placeFrom.valueIsPointer) || (placeTo.placement == regs::inRam && !placeTo.valueIsPointer)) {
        //situace neni normalni, nepouzivame tuhle funkce na kopirovani mezi registry totiz, takze tahle situace nema nastat
        return;
    }

    if (placeFrom.placement != regs::inRam && (placeFrom.placement == placeTo.placement)) {
        //pointery jsou na stejnem registru, opet neakceptujeme kopirovani
        return;
    }

    bool incrementReg1 = false;
    bool incrementReg2 = false;
    //kde je pointer na tu vec (at je ta vec uz pointer nebo ne)
    std::string fromMemoryDesc;
    std::string toMemoryDesc;

    if (placeFrom.placement == regs::reg1) {
        fromMemoryDesc = asmSyntax.addr1;
        incrementReg1 = true;
    }

    if (placeFrom.placement == regs::reg2) {
        fromMemoryDesc = asmSyntax.addr2;
        incrementReg2 = true;
    }

    if (placeFrom.placement == regs::inRam) {
        fromMemoryDesc = placeFrom.pointer.getString();
    }

    if (placeTo.placement == regs::reg1) {
        toMemoryDesc = asmSyntax.addr1;
        incrementReg1 = true;
    }

    if (placeTo.placement == regs::reg2) {
        toMemoryDesc = asmSyntax.addr2;
        incrementReg2 = true;
    }

    if (placeTo.placement == regs::inRam) {
        toMemoryDesc = placeFrom.pointer.getString();
    }

    //4 moznosti s referencemi (from, to):
    //ne a ne = kopirovani, podle velikosti to
    //ano a ne = obsah toho kam ted ukazujeme je pointer, takze naloadim tamto do reg1
    //ne a ano = vezmi pointer na from
    //ano a ano = zkopiruj obsah from do to, velikost 2

    if (placeTo.valueIsPointer) {
        //ano a ano
        if (placeFrom.valueIsPointer) {
            code.addInstruction(asmSyntax.mov + toMemoryDesc + fromMemoryDesc);
            return;
        }

        //ne a ano
        if (placeFrom.placement == regs::inRam) {

        }
        return;
    }

    //ano a ne
    if (placeFrom.valueIsPointer) {
        return;
    }

    //ne a ne


}



