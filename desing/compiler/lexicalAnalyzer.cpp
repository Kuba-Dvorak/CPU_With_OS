#include "./lexicalSyntax.cpp"


syntaxDefiners lexSyntax;
syntaxForBT btSyntax;


struct goodLine {
    bool exists;
    std::string oneLine;
    bool problem;
};


goodLine loadNextLine(std::string &origCode, int lastLine) {

}


typePart loadTypePart(std::string &oneLine) {

}


astNodeNames decideLineType(std::string &oneLine) {

}


std::vector<syntaxDefiners> loadNodes() {

}


goodBracket findSegmentStart(std::string &oneLine) {

}


goodBracket findSegmentEnd(std::string &oneLine) {

}


//funkce psane pomoci AI
//vsechny findery berou lineCode a pozici od ktere se ma zacit hledat
//kdyz na dane pozici ta vec neni, vraci se struktura s exists = false
//position ve vracene strukture = pozice PRVNIHO znaku za nactenou veci (odkud pokracovat dal)


//nacte jmeno (var, typ, funkce) od startPosition, tzn. vse do prvniho znaku ktery neni soucasti jmena
std::string loadName(std::string &lineCode, int startPosition) {

}


//zacatek konstrukce typu: typeConstrStart "[" nebo konec typu "]"
goodConstructStart findConstructStart(std::string &lineCode, int startPosition) {

}


goodConstructStart findConstructEnd(std::string &lineCode, int startPosition) {

}


//promena: [prefixy] jmeno [.keys]
goodVar findVar(std::string &lineCode, int startPosition, varHandle &vars) {

}


//volani funkce: callFunq "call" + jmeno + functionCallStart "<"
goodFunction findFunction(std::string &lineCode, int startPosition, functionHandle &funcs) {

}


//cislo (zatim jen int)
goodNumber findNumber(std::string &lineCode, int startPosition) {

}


//normalni zavorka: [prefixy] normalBracketStart "("
goodBracket findStartBracket(std::string &lineCode, int startPosition) {

}


goodBracket findEndBracket(std::string &lineCode, int startPosition) {

}


//hleda zavorku z funkce (dle lexical syntax)
goodBracket findFunqStrBrc(std::string &lineCode, int startPosition) {

}


goodBracket findFunqEndBrc(std::string &lineCode, int startPosition) {

}



//operator ze syntaxForBT (2 znakove se musi testovat driv nez 1 znakove)
goodOperator findOperator(std::string &lineCode, int startPosition) {

}


goodSingleOperator findSingleOperator(std::string &lineCode, int startPosition) {

}


//separator: nextSign "," pri konstrukci typu / oddeleni argumentu funkce
goodSeparator findSeparator(std::string &lineCode, int startPosition) {

}
