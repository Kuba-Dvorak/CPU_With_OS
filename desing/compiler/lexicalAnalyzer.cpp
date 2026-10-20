#include "./lexicalSyntax.cpp"


syntaxDefiners lexSyntax;
syntaxForBT btSyntax;


std::string loadOneLine(std::string &origCode) {

}


std::vector<syntaxDefiners> loadNodes() {

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
goodBracket findBracket(std::string &lineCode, int startPosition) {

}


//operator ze syntaxForBT (2 znakove se musi testovat driv nez 1 znakove)
goodOperator findOperator(std::string &lineCode, int startPosition) {

}


//separator: nextSign "," pri konstrukci typu / oddeleni argumentu funkce
goodSeparator findSeparator(std::string &lineCode, int startPosition) {

}
