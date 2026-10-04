#include <string>

struct basicSyntax {
    std::string setReg = " setReg ";
    std::string movRamReg = " movRamReg ";
    std::string reg1 = " R1 ";
    std::string reg2 = " R2 ";
    std::string reg0 = " R0 ";
    std::string reg3 = " R3 ";
    std::string reg4 = " R4 ";
    std::string addReg = " add8 ";// add8 works like this: var1 = reg code, var2 = adding number, if the reg code is 5, similary for var3 and var4
    std::string addRam = " addRam ";
    std::string movRegRam = " movRegRam ";
    std::string movRamRam = " movRamRam ";
    std::string movRamRamByReg = " movRamRamByReg ";
    std::string movRegReg = " movRegReg ";
    std::string setRam16 = " setRam16 ";
    std::string setRam8 = " setRam8 ";

    //both jumps are absolute
    std::string jumpFromReg = " jumpReg ";
    std::string jumpDictate = " jump ";
    //reg 3 = carry out latch, reg 4 = carry in latch
};

basicSyntax asmSyntax;
