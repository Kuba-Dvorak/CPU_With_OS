#include <string>

//mov: odkud kam
//set: kam co
//add: co1, co2, kam ulozit
struct basicSyntax {
    std::string setReg = " setReg ";
    std::string movRamReg = " movRamReg ";

    //registry
    std::string reg0 = " R0 ";
    std::string reg1 = " R1 "; // registr, kam se dava pri hledani pres reference
    std::string reg2 = " R2 "; // registr
    std::string regStackPointer = " R3 "; // registr kde je stack pointer
    std::string regStackOper = " R4 "; // registr kde se uklada vysledek stackPointeru
    std::string reg5 = " R5 "; // dalsi odkladaci registr
    std::string regNone = " RNone ";//pri scitani kdyz chceme pouze podle posledniho incrementu

    //latche
    std::string latchALUCarryI = " LACI ";
    std::string latchALUCarryO = " LACO ";
    std::string latchCompare = " LC "; //neni uz zapojen primo na preskoceni dalsi instrukce jako v orig. navrhu, da se setovat
    std::string skippingLatch = " LSkip "; //vstupni brana mezi LC a preskocenim dalsi insturkce jako v origin. navrhu, da se setovat a resetuje se pri dalsi instrukci
    //vstup z LC do LSkip je negovany

    // RAM adresa 0 = vzit z registru 1
    std::string addReg = " add8 ";// add8 works like this: var1 = reg code, var2 = adding number, if the reg code is 5, similary for var3 and var4
    std::string rmReg = " rm8 ";
    std::string addRam = " addRam ";
    std::string incrementReg = " increment ";
    std::string movRegRam = " movRegRam ";
    std::string movRamRam = " movRamRam ";
    std::string movRegReg = " movRegReg ";
    std::string movRegRam16 = " movRegRam16 ";
    std::string movRamReg16 = " movRamReg16 ";
    std::string movRamRam16 = " movRamRam16 ";
    std::string movRegReg16 = " movRegReg16 ";
    std::string setRam16 = " setRam16 ";
    std::string setRam8 = " setRam8 ";

    //both jumps are absolute
    std::string jumpFromReg = " jumpReg ";
    std::string jumpDictate = " jump ";


    //comparative
    std::string compare = " compare ";
    //pro compare flagy
    std::string equal = " eq ";
    std::string biggerThan = " big ";
    std::string smallerThan = " sml ";
    std::string bigSmallThan = " smbg ";
    std::string bigequal = " ebig ";
    std::string smequal = " esml ";
};

basicSyntax asmSyntax;
std::string constexpr staticRamSeg = "static";
std::string constexpr dynamicRamSeg = "dynamic";
std::string constexpr returnNamer = "returnNamer";
int constexpr addressSize = 2;