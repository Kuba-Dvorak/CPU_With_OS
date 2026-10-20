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
};


