#pragma once
// Jedine misto, kde se testy dotykaji kodu kompilatoru.
// Kdyz pri prepisu zmenis jmena (copyFromTo, information, ramPointer, fullCompiledCode),
// uprav jen tenhle soubor.
#include <string>
#include <vector>
// COPY_IMPL pouziva mutate.py, aby mohl podstrcit upravenou kopii memoryOperations.cpp
#ifdef COPY_IMPL
#include COPY_IMPL
#else
#include "../memoryManagment/memoryOperations.cpp"
#endif

namespace adapter {

// jedna strana kopirovani tak, jak ji vidi kompilator
struct Side {
    bool inReg1;      // adresa slotu je v R1 (pointer.pointer se pak nepouziva)
    std::string seg;  // segment slotu ("static" / "dynamic")
    int offset;       // offset slotu v segmentu, -1 = nedefinovano
    int size;         // velikost hodnoty v bajtech
    bool isPointer;   // ve slotu je 2B pointer na skutecna data
};

inline std::vector<std::string> generateCopy(const Side &from, const Side &to) {
    fullCompiledCode code;
    information f{from.inReg1, {from.seg, from.offset}, from.size, from.isPointer};
    information t{to.inReg1, {to.seg, to.offset}, to.size, to.isPointer};
    copyFromTo(f, t, code);
    return code.code;
}

} // namespace adapter
