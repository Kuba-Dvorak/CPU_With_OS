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
    std::string reg6 = " R6 "; // dalsi odkladaci registr
    std::string reg7 = " R7 ";
    std::string regNone = " RNone "; //pri scitani kdyz chceme pouze podle posledniho incrementu
    std::string signReg1 = " signReg1 "; //dalsi datovy reg, pojmenovani podle vyuziti v kompilatoru
    std::string signReg2 = " signReg2 ";

    //latche
    std::string latchShiftLowest = " LShiftLow "; //da se sem nejmensi bit z shiftovani do mensich cisel doprava (myslim, ze doprava), proste na mensi cislo, vyuzije se pri operaci: putToReg
    std::string latchALUCarryI = " LACI ";
    std::string latchALUCarryO = " LACO ";
    std::string latchCompare = " LC "; //co se nastavi treba pri isEqual nebo pri lookAtSign
    std::string skippingLatch = " LSkip "; //to co preskoci dalsi instrukci kdyz je = 1

    // RAM adresa 0 = vzit z registru 1
    std::string addReg16 = " addReg16 ";// add8 works like this: var1 = reg code, var2 = adding number, if the reg code is 5, similary for var3 and var4
    std::string addReg8 = " addReg8 ";
    std::string addRegNoCarry = " addNoC "; //pouzije scitacku s zablokovanymi carryIn a carryOut, 16b
    std::string multiply8 = " mut8 ";//nasobeni 2 low casti registru na vytvoreni 1 16bitoveho cisla
    std::string negateBit = " neg1 ";
    std::string negate8 = " neg8 ";
    std::string negate16 = " neg16 ";//zneguje cislo
    std::string rmReg = " rm8 ";
    std::string setBySign = " setBySign8 "; //rychla operace, ktera se podiva na nejvyssi bit daneho registru a pak setne cilovy registr na 1/0 podle tohodle
    std::string setBySign16 = " setBySign16 ";
    std::string isEqual = " cmpEqual "; // podiva se na registr 8b a rekne jestli je to same 0, a potom nastavi compareLatch: same 0 = 1, jinak 0
    std::string lookAtSign = " cmpSign "; //podiva se na 8b znacku v registru a nastavi podle toho compareLatch: je zaporne = 1, neni zaporne = 0
    std::string and8 = " and8 ";
    std::string and16 = " and16 ";
    std::string or8 = " or8 ";
    std::string or16 = " or16 ";
    std::string addRam = " addRam ";
    std::string incrementReg = " increment ";
    std::string shiftLowBy1 = " shiftL1 "; //shiftne reg 1 do mensiho do reg 2
    std::string shiftBigBy1 = " shiftB1 "; //shiftne reg 1 do vetsiho do reg 2
    std::string putToReg = " putTo "; // prida na zacatek (8bitove) latchShiftLowest
    std::string putToRegPos = " putPos "; //setne do registru na nejake specificke misto, bere reg a cislo (misto) a 1/0
    std::string xorReg = " xorReg "; //xor registru
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