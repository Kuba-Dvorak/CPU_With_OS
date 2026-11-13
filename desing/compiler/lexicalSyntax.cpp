#include "./memoryManagment/functions.cpp"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>



enum class astNodeNames {
    segmentAvg = 1,
    segmentConditional = 2,
    segmentCyclic = 3,
    declareVar = 4,
    declareType = 5,
    declareFunction = 6,
    setVar = 7,
    continuer = 8,
    returner = 9,
    breaker = 10
};



struct syntaxDefiners {
    std::string endOfLine = ";";
    std::string startOfSegment = "{";
    std::string endOfSegment = "}";
    std::string forSignature = "for";
    std::string whileSignature = "while";
    std::string ifSignature = "if";
    std::string elseSignature = "else";
    std::string continueSignature = "continue";
    std::string returnSignature = "return";
    std::string typeSignature = "type"; //deklarace typu
    std::string functionSignature = "function"; //deklarace funkce
    std::string callFunq = "call"; //volani funkce
};


struct syntaxForBT {
    std::string plusSign = "+";
    std::string minusSign = "-";
    std::string timesSign = "*";
    std::string divideSign = "/";
    std::string andSign = "&";
    std::string orSign = "|";
    std::string equalSign = "==";
    std::string bigSign = ">";
    std::string smlSign = "<";
    std::string eqBigSign = "=>";
    std::string eqSmlSign = "=<";
    std::string notEqualSign = "=!";

    //veci co dat pred var nebo pred celou ozavorkovanou cast v BT
    std::string notSign = "!"; //
    std::string isPointerSign = "$"; //rika 2 veci: 1. ze vec pred promenou/zavorkou a oznaci ji za pointer
                                     //             2. ze promena ma byt zmenena orignal (pokud se jedna o pointer

    std::string nextSign = ","; //pri konstrukci sloziteho typu (vzdy vlastne vezme ten 1 hotovy BT co byl vytvoren z minule a je ted v cachi
                                //posle ho do te promene na spravne misto kde to ma byt, resetuje cache a potom jde na dalsi var v danem typu s novym cache (savuje pamet)
                                //ve funkci funguje trochu jinak, vlastne ukoncuje

    std::string keyingSymbol = "."; //keys pro nacitani klicu z typu, tohle udela ten dlouhy vector stringu a to prvni je jmeno varu ktery ho vytahne

    //brackets:
    std::string normalBracketStart = "("; //jako zavorky a na funkce
    std::string normalBracketEnd = ")";
    std::string typeConstrStart = "[";
    std::string typeConstrEnd = "]";
    std::string functionCallStart = "<";
    std::string functionCallEnd = ">";
};


enum class singleOperator {
    notOperBit = 1,
    isPointer = 2,
    notOperByte = 3,
    noSingleOper = 4
};


enum class nodeTypes {
    notWorking = -1,
    aritmo = 1,
    number = 2,
    variable = 3,
    function = 4,
    funqPlace = 5
};


enum class operators {
    plus = 1,
    remove = 2,
    multiply = 3,
    divade = 4,
    andOperator = 6,
    orOperator = 7,
    xorOperator = 15,
    equal = 9,
    bigger = 10,
    lesser = 11,
    ebig = 12,
    esml = 13,
    notEqual = 14
};


enum class logOperators {
    andOp = 1,
    orOp = 2,
    notByteOp = 3,
    xorOp = 4,
    notBitOp = 5
};



struct goodConstructStart {
    int position;
    bool exists;
};


struct goodVar {
    variable* var;
    std::vector<std::string> varKeys;
    int position;
    bool exists;
};


struct goodFunction {
    basicFunction* func;
    int position; //pozice na zacatek zavorek
    int endOfFunq; //pozice za koncem funkce - musi to projet funkci od start funkce, a pocitat si dalsi zavorky, zaciname cislem 0, pokud najdeme dalsi zacinajici zavorku tak pricteme tomhlu cislu 1, a pokud najdeme k tehle zavirajici tak odecteme 1, tim zajistime ze zavirajici zavorka pro tuhle funkci a ne jina
    bool exists;
};


struct goodNumber {
    int value;
    int position;
    bool exists;
};


struct goodBracket {
    int position;
    bool exists;
};


struct goodOperator {
    operators op;
    int position;
    bool exists;
};


struct goodSingleOperator {
    singleOperator op;
    int position;
    //kdyz neexistuje tak vraci noSingleOper
};


struct goodSeparator {
    int position;
    bool exists;
};