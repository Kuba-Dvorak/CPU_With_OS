#include "./lexicalSyntax.cpp"


syntaxDefiners lexSyntax;
syntaxForBT btSyntax;


//napsano pomoci AI
//dopredne deklarace funkci z nizsi casti souboru, pouzivaji je i funkce nahore
std::string loadName(std::string &lineCode, int startPosition);
goodNumber findNumber(std::string &lineCode, int startPosition);
goodBracket findStartBracket(std::string &lineCode, int startPosition);
goodBracket findEndBracket(std::string &lineCode, int startPosition);
goodBracket findFunqStrBrc(std::string &lineCode, int startPosition);
goodBracket findFunqEndBrc(std::string &lineCode, int startPosition);
goodSeparator findSeparator(std::string &lineCode, int startPosition);
goodFoundSegment findSegmenter(std::string &oneLine);



//napsano pomoci AI
//[$]JMENO.k1.k2 = ...; -> {JMENO, {k1, k2}, je tam $}
goodVarSet loadVarSetter(std::string &oneLine) {
    goodVarSet result = {"", {}, false};
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    //$ na zacatku = zapsat do originalu, na ktery reference ukazuje
    if (position < oneLine.length() && oneLine.substr(position, btSyntax.isPointerSign.length()) == btSyntax.isPointerSign) {
        result.changeOriginal = true;
        position += btSyntax.isPointerSign.length();
    }
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    result.varName = loadName(oneLine, position);
    position += result.varName.length();

    //klice .k1.k2
    while (position < oneLine.length() && oneLine.substr(position, btSyntax.keyingSymbol.length()) == btSyntax.keyingSymbol) {
        int keyPosition = position + btSyntax.keyingSymbol.length();
        while (keyPosition < oneLine.length() && oneLine[keyPosition] == ' ') {
            keyPosition += 1;
        }
        std::string key = loadName(oneLine, keyPosition);
        if (key.empty()) {
            break;
        }
        result.keys.push_back(key);
        position = keyPosition + key.length();
    }
    return result;
}


//napsano pomoci AI
//var JMENO [$]TYP[(N)]; -> {JMENO, TYP, je tam $, N}, bez (N) je velikost 2 B
//stejne se ctou i pole typu (vola createNewType)
goodVarDecl loadVarDef(std::string &oneLine) {
    goodVarDecl result = {"", "", false, 2};
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    //preskocit "var"
    position += loadName(oneLine, position).length();
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    result.varName = loadName(oneLine, position);
    position += result.varName.length();
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }

    //$ pred typem = reference
    if (position < oneLine.length() && oneLine.substr(position, btSyntax.isPointerSign.length()) == btSyntax.isPointerSign) {
        result.isRef = true;
        position += btSyntax.isPointerSign.length();
    }
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    result.varTypeName = loadName(oneLine, position);
    position += result.varTypeName.length();

    //velikost cisla v bajtech: number(N)
    goodBracket sizeBracket = findStartBracket(oneLine, position);
    if (sizeBracket.exists) {
        goodNumber size = findNumber(oneLine, sizeBracket.position);
        if (size.exists) {
            result.posibleSize = size.value;
        }
    }
    return result;
}


//napsano pomoci AI
//function JMENO(P1, P2) [$]TYP[(N)] {;   parametr je [$]TYP[(N)] JMENO
//(N) = velikost cisla v bajtech, bez (N) 2 B (u jinych typu nez number se ignoruje)
goodLoadedFunctionDefinition loadDefFunq(std::string &oneLine) {
    goodLoadedFunctionDefinition result = {"", {}, {}, {}, "", false, {}, 2};
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    //preskocit "function"
    position += loadName(oneLine, position).length();
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    result.funqName = loadName(oneLine, position);
    position += result.funqName.length();

    //zacatek parametru (
    goodBracket paramStart = findStartBracket(oneLine, position);
    if (!paramStart.exists) {
        return result;
    }
    position = paramStart.position;

    //parametry [$]TYP JMENO oddelene carkou az po )
    while (true) {
        goodBracket paramEnd = findEndBracket(oneLine, position);
        if (paramEnd.exists) {
            position = paramEnd.position;
            break;
        }
        while (position < oneLine.length() && oneLine[position] == ' ') {
            position += 1;
        }
        if (position >= oneLine.length()) {
            break;
        }
        bool isRef = false;
        if (oneLine.substr(position, btSyntax.isPointerSign.length()) == btSyntax.isPointerSign) {
            isRef = true;
            position += btSyntax.isPointerSign.length();
        }
        while (position < oneLine.length() && oneLine[position] == ' ') {
            position += 1;
        }
        std::string typeName = loadName(oneLine, position);
        position += typeName.length();

        //velikost cisla (N)
        int paramSize = 2;
        goodBracket sizeStart = findStartBracket(oneLine, position);
        if (sizeStart.exists) {
            goodNumber size = findNumber(oneLine, sizeStart.position);
            goodBracket sizeEnd = findEndBracket(oneLine, size.position);
            if (size.exists && sizeEnd.exists) {
                paramSize = size.value;
                position = sizeEnd.position;
            }
        }

        while (position < oneLine.length() && oneLine[position] == ' ') {
            position += 1;
        }
        std::string paramName = loadName(oneLine, position);
        position += paramName.length();
        //chyba v parametru, dal se nacitat neda
        if (typeName.empty() || paramName.empty()) {
            break;
        }
        result.inputRefs.push_back(isRef);
        result.inputTypes.push_back(typeName);
        result.inputNames.push_back(paramName);
        result.inputSizes.push_back(paramSize);

        goodSeparator separator = findSeparator(oneLine, position);
        if (separator.exists) {
            position = separator.position;
        }
    }

    //vystup [$]TYP
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position < oneLine.length() && oneLine.substr(position, btSyntax.isPointerSign.length()) == btSyntax.isPointerSign) {
        result.outputRef = true;
        position += btSyntax.isPointerSign.length();
    }
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    result.outputType = loadName(oneLine, position);
    position += result.outputType.length();

    //velikost vystupniho cisla (N)
    goodBracket sizeStart = findStartBracket(oneLine, position);
    if (sizeStart.exists) {
        goodNumber size = findNumber(oneLine, sizeStart.position);
        goodBracket sizeEnd = findEndBracket(oneLine, size.position);
        if (size.exists && sizeEnd.exists) {
            result.outputSize = size.value;
        }
    }
    return result;
}


//napsano pomoci AI
//type JMENO {; -> JMENO
std::string loadTypeDefName(std::string &oneLine) {
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    //preskocit "type"
    position += loadName(oneLine, position).length();
    return loadName(oneLine, position);
}


//napsano pomoci AI (prepsana puvodni verze autora)
//vrati prikaz cislo lastLine (od 0), prikaz konci jen strednikem a strednik do nej patri
//bez stavu: pokazde se kod projde od zacatku, takze stejne cislo = stejny prikaz
//bile znaky mezi prikazy se zahodi, konce radku a tabulatory uvnitr prikazu se nahradi mezerou
goodLine loadNextLine(std::string &origCode, int lastLine) {
    int lineChar = 0;
    //preskocit lastLine prikazu, vcetne jejich stredniku
    for (int i = 0; i < lastLine; i++) {
        while (lineChar < origCode.length() && origCode.substr(lineChar, lexSyntax.endOfLine.length()) != lexSyntax.endOfLine) {
            lineChar += 1;
        }
        if (lineChar >= origCode.length()) {
            return {false, "", false};
        }
        lineChar += lexSyntax.endOfLine.length();
    }

    std::string oneLine = "";
    bool foundEnd = false;
    while (lineChar < origCode.length()) {
        if (origCode.substr(lineChar, lexSyntax.endOfLine.length()) == lexSyntax.endOfLine) {
            oneLine += lexSyntax.endOfLine;
            foundEnd = true;
            break;
        }
        char oneChar = origCode[lineChar];
        if (oneChar == '\n' || oneChar == '\r' || oneChar == '\t') {
            oneChar = ' ';
        }
        //mezery pred prikazem se zahodi
        if (oneChar != ' ' || !oneLine.empty()) {
            oneLine += oneChar;
        }
        lineChar += 1;
    }

    if (foundEnd) {
        return {true, oneLine, false};
    }
    //konec kodu: bud uz zadny prikaz neni, nebo zbyl text bez stredniku
    while (!oneLine.empty() && oneLine.back() == ' ') {
        oneLine.pop_back();
    }
    if (oneLine.empty()) {
        return {false, "", false};
    }
    return {true, oneLine, true};
}


//napsano pomoci AI
//rozhoduje podle prvniho znaku nebo slova prikazu, klicova slova se porovnavaji cela (iffy neni if)
astNodeNames decideLineType(std::string &oneLine) {
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position < oneLine.length() && oneLine.substr(position, lexSyntax.endOfSegment.length()) == lexSyntax.endOfSegment) {
        return astNodeNames::segmentEnder;
    }
    if (position < oneLine.length() && oneLine.substr(position, lexSyntax.startOfSegment.length()) == lexSyntax.startOfSegment) {
        return astNodeNames::segmentAvg;
    }

    std::string firstWord = loadName(oneLine, position);
    if (firstWord == lexSyntax.whileSignature) {
        return astNodeNames::segmentCyclic;
    }
    if (firstWord == lexSyntax.ifSignature) {
        return astNodeNames::segmentConditional;
    }
    if (firstWord == lexSyntax.typeSignature) {
        return astNodeNames::declareType;
    }
    if (firstWord == lexSyntax.functionSignature) {
        return astNodeNames::declareFunction;
    }
    if (firstWord == lexSyntax.varSignature) {
        return astNodeNames::declareVar;
    }
    if (firstWord == lexSyntax.returnSignature) {
        return astNodeNames::returner;
    }
    if (firstWord == lexSyntax.continueSignature) {
        return astNodeNames::continuer;
    }
    if (firstWord == lexSyntax.breakSignature) {
        return astNodeNames::breaker;
    }
    if (firstWord == lexSyntax.callFunq) {
        return astNodeNames::callFunq;
    }
    //[$]JMENO[.klic] = VYRAZ
    if (findSegmenter(oneLine).exists) {
        return astNodeNames::setVar;
    }
    //neznamy prikaz: astNodeNames pro chybu nema hodnotu, takze callFunq, aby to aspon spadlo pri parsovani
    return astNodeNames::callFunq;
}


//najde konstruktor / BT v 1 radku, POZOR NENI TO JENOM KONSTRUKTOR, muze to byt i BT, presneji, tohle je vlastne input do funkce createParsedSegment
//napsano pomoci AI
//if -> za if (zavorku s podminkou zpracuje vyraz sam), return -> za return, call -> zacatek call, prirazeni -> za =
goodFoundSegment findSegmenter(std::string &oneLine) {
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    std::string firstWord = loadName(oneLine, position);
    if (firstWord == lexSyntax.ifSignature || firstWord == lexSyntax.returnSignature) {
        return {true, position + (int)firstWord.length()};
    }
    if (firstWord == lexSyntax.callFunq) {
        return {true, position};
    }

    //prirazeni: [$]JMENO[.klic.klic] = VYRAZ
    if (position < oneLine.length() && oneLine.substr(position, btSyntax.isPointerSign.length()) == btSyntax.isPointerSign) {
        position += btSyntax.isPointerSign.length();
    }
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    std::string varName = loadName(oneLine, position);
    if (varName.empty()) {
        return {false, 0};
    }
    position += varName.length();
    while (position < oneLine.length() && oneLine.substr(position, btSyntax.keyingSymbol.length()) == btSyntax.keyingSymbol) {
        position += btSyntax.keyingSymbol.length();
        while (position < oneLine.length() && oneLine[position] == ' ') {
            position += 1;
        }
        std::string key = loadName(oneLine, position);
        if (key.empty()) {
            return {false, 0};
        }
        position += key.length();
    }
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    //prirazovaci "=", ne operator "=="
    if (position >= oneLine.length() || oneLine.substr(position, lexSyntax.setSignature.length()) != lexSyntax.setSignature) {
        return {false, 0};
    }
    if (oneLine.substr(position, btSyntax.equalSign.length()) == btSyntax.equalSign) {
        return {false, 0};
    }
    return {true, position + (int)lexSyntax.setSignature.length()};
}


//napsano pomoci AI
//zatim se nikde nevola
std::vector<syntaxDefiners> loadNodes() {
    return {};
}


//napsano pomoci AI
//exists = true jen pro prikaz "{;" (krome mezer nic jineho), position = za {
goodBracket findSegmentStart(std::string &oneLine) {
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position >= oneLine.length() || oneLine.substr(position, lexSyntax.startOfSegment.length()) != lexSyntax.startOfSegment) {
        return {0, false};
    }
    int afterBracket = position + lexSyntax.startOfSegment.length();
    position = afterBracket;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position >= oneLine.length() || oneLine.substr(position, lexSyntax.endOfLine.length()) != lexSyntax.endOfLine) {
        return {0, false};
    }
    position += lexSyntax.endOfLine.length();
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position != oneLine.length()) {
        return {0, false};
    }
    return {afterBracket, true};
}


//napsano pomoci AI
//exists = true jen pro prikaz "};" (krome mezer nic jineho), position = za }
goodBracket findSegmentEnd(std::string &oneLine) {
    int position = 0;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position >= oneLine.length() || oneLine.substr(position, lexSyntax.endOfSegment.length()) != lexSyntax.endOfSegment) {
        return {0, false};
    }
    int afterBracket = position + lexSyntax.endOfSegment.length();
    position = afterBracket;
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position >= oneLine.length() || oneLine.substr(position, lexSyntax.endOfLine.length()) != lexSyntax.endOfLine) {
        return {0, false};
    }
    position += lexSyntax.endOfLine.length();
    while (position < oneLine.length() && oneLine[position] == ' ') {
        position += 1;
    }
    if (position != oneLine.length()) {
        return {0, false};
    }
    return {afterBracket, true};
}


//funkce psane pomoci AI
//vsechny findery berou lineCode a pozici od ktere se ma zacit hledat
//kdyz na dane pozici ta vec neni, vraci se struktura s exists = false
//position ve vracene strukture = pozice PRVNIHO znaku za nactenou veci (odkud pokracovat dal)


//nacte jmeno (var, typ, funkce) od startPosition, tzn. vse do prvniho znaku ktery neni soucasti jmena
//napsano pomoci AI
//jmeno zacina pismenem nebo _, dal muzou byt i cislice, mezery na zacatku preskoci, kdyz tam jmeno neni vrati ""
//jmeno konci i pred koncem volani "_/", protoze _ muze byt ve jmenu ("a_/" = jmeno a + konec volani)
std::string loadName(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    std::string name = "";
    while (position < lineCode.length()) {
        char oneChar = lineCode[position];
        bool isLetter = (oneChar >= 'a' && oneChar <= 'z') || (oneChar >= 'A' && oneChar <= 'Z');
        bool isDigit = oneChar >= '0' && oneChar <= '9';
        if (!isLetter && oneChar != '_' && !(isDigit && !name.empty())) {
            break;
        }
        if (lineCode.substr(position, btSyntax.functionCallEnd.length()) == btSyntax.functionCallEnd) {
            break;
        }
        name += oneChar;
        position += 1;
    }
    return name;
}


//zacatek konstrukce typu: typeConstrStart "[" nebo konec typu "]"
//napsano pomoci AI
goodConstructStart findConstructStart(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.typeConstrStart.length()) == btSyntax.typeConstrStart) {
        return {position + (int)btSyntax.typeConstrStart.length(), true};
    }
    return {startPosition, false};
}


//napsano pomoci AI
goodConstructStart findConstructEnd(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.typeConstrEnd.length()) == btSyntax.typeConstrEnd) {
        return {position + (int)btSyntax.typeConstrEnd.length(), true};
    }
    return {startPosition, false};
}


//promena: [prefixy] jmeno [.keys]
//napsano pomoci AI
//bez prefixu ($, ! a ~ zpracovava parseBT), klice se tu nekontroluji, jen nactou
//promenna se hleda pres varHandle::returnVar
goodVar findVar(std::string &lineCode, int startPosition, varHandle &vars) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    std::string name = loadName(lineCode, position);
    //call je volani funkce, ne promenna (getValueNode zkousi findVar i na volani)
    if (name.empty() || name == lexSyntax.callFunq) {
        return {nullptr, {}, startPosition, false};
    }
    variable* foundVar = vars.returnVar(name);
    if (foundVar == nullptr) {
        return {nullptr, {}, startPosition, false};
    }
    position += name.length();

    //klice .k1.k2
    std::vector<std::string> keys = {};
    while (position < lineCode.length() && lineCode.substr(position, btSyntax.keyingSymbol.length()) == btSyntax.keyingSymbol) {
        int keyPosition = position + btSyntax.keyingSymbol.length();
        while (keyPosition < lineCode.length() && lineCode[keyPosition] == ' ') {
            keyPosition += 1;
        }
        std::string key = loadName(lineCode, keyPosition);
        if (key.empty()) {
            break;
        }
        keys.push_back(key);
        position = keyPosition + key.length();
    }
    return {foundVar, keys, position, true};
}


//volani funkce: callFunq "call" + jmeno + functionCallStart "<"
//napsano pomoci AI
//call JMENO /_ ... _/ -> position = hned za /_ (zacatek argumentu), endOfFunq = za odpovidajicim _/
//funkce se hleda pres functionHandle::findFunction
goodFunction findFunction(std::string &lineCode, int startPosition, functionHandle &funcs) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (loadName(lineCode, position) != lexSyntax.callFunq) {
        return {nullptr, startPosition, startPosition, false};
    }
    position += lexSyntax.callFunq.length();
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    std::string name = loadName(lineCode, position);
    if (name.empty()) {
        return {nullptr, startPosition, startPosition, false};
    }
    position += name.length();
    goodBracket argsStart = findFunqStrBrc(lineCode, position);
    if (!argsStart.exists) {
        return {nullptr, startPosition, startPosition, false};
    }

    basicFunction* foundFunq = funcs.findFunction(name);
    if (foundFunq == nullptr) {
        return {nullptr, startPosition, startPosition, false};
    }

    //odpovidajici konec volani: /_ +1, _/ -1
    int depth = 1;
    int endPosition = argsStart.position;
    while (endPosition < lineCode.length() && depth > 0) {
        goodBracket callEnd = findFunqEndBrc(lineCode, endPosition);
        goodBracket callStart = findFunqStrBrc(lineCode, endPosition);
        if (callEnd.exists) {
            depth -= 1;
            endPosition = callEnd.position;
        }
        else if (callStart.exists) {
            depth += 1;
            endPosition = callStart.position;
        }
        else {
            endPosition += 1;
        }
    }
    if (depth > 0) {
        return {nullptr, startPosition, startPosition, false};
    }
    return {foundFunq, argsStart.position, endPosition, true};
}


//cislo (zatim jen int)
//napsano pomoci AI
//jen desitkove cislice bez znamenka (unarni minus jazyk nema)
goodNumber findNumber(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position >= lineCode.length() || lineCode[position] < '0' || lineCode[position] > '9') {
        return {0, startPosition, false};
    }
    long long value = 0;
    while (position < lineCode.length() && lineCode[position] >= '0' && lineCode[position] <= '9') {
        value = value * 10 + (lineCode[position] - '0');
        position += 1;
    }
    return {(int)value, position, true};
}


//normalni zavorka: [prefixy] normalBracketStart "("
//napsano pomoci AI
//prefixy ($, !, ~) tu nectu, ty zpracovava parseBT pres findSingleOperator
goodBracket findStartBracket(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.normalBracketStart.length()) == btSyntax.normalBracketStart) {
        return {position + (int)btSyntax.normalBracketStart.length(), true};
    }
    return {startPosition, false};
}


//napsano pomoci AI
goodBracket findEndBracket(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.normalBracketEnd.length()) == btSyntax.normalBracketEnd) {
        return {position + (int)btSyntax.normalBracketEnd.length(), true};
    }
    return {startPosition, false};
}


//hleda zavorku z funkce (dle lexical syntax)
//napsano pomoci AI
goodBracket findFunqStrBrc(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.functionCallStart.length()) == btSyntax.functionCallStart) {
        return {position + (int)btSyntax.functionCallStart.length(), true};
    }
    return {startPosition, false};
}


//napsano pomoci AI
goodBracket findFunqEndBrc(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.functionCallEnd.length()) == btSyntax.functionCallEnd) {
        return {position + (int)btSyntax.functionCallEnd.length(), true};
    }
    return {startPosition, false};
}



//operator ze syntaxForBT (2 znakove se musi testovat driv nez 1 znakove)
//napsano pomoci AI
//samotne = neni operator (prirazeni), "/_" neni deleni (zacatek volani)
goodOperator findOperator(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position >= lineCode.length()) {
        return {operators::plus, startPosition, false};
    }
    if (findFunqStrBrc(lineCode, position).exists) {
        return {operators::plus, startPosition, false};
    }
    //2 znakove
    if (lineCode.substr(position, btSyntax.equalSign.length()) == btSyntax.equalSign) {
        return {operators::equal, position + (int)btSyntax.equalSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.eqBigSign.length()) == btSyntax.eqBigSign) {
        return {operators::ebig, position + (int)btSyntax.eqBigSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.eqSmlSign.length()) == btSyntax.eqSmlSign) {
        return {operators::esml, position + (int)btSyntax.eqSmlSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.notEqualSign.length()) == btSyntax.notEqualSign) {
        return {operators::notEqual, position + (int)btSyntax.notEqualSign.length(), true};
    }
    //1 znakove
    if (lineCode.substr(position, btSyntax.plusSign.length()) == btSyntax.plusSign) {
        return {operators::plus, position + (int)btSyntax.plusSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.minusSign.length()) == btSyntax.minusSign) {
        return {operators::remove, position + (int)btSyntax.minusSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.timesSign.length()) == btSyntax.timesSign) {
        return {operators::multiply, position + (int)btSyntax.timesSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.divideSign.length()) == btSyntax.divideSign) {
        return {operators::divade, position + (int)btSyntax.divideSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.andSign.length()) == btSyntax.andSign) {
        return {operators::andOperator, position + (int)btSyntax.andSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.orSign.length()) == btSyntax.orSign) {
        return {operators::orOperator, position + (int)btSyntax.orSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.bigSign.length()) == btSyntax.bigSign) {
        return {operators::bigger, position + (int)btSyntax.bigSign.length(), true};
    }
    if (lineCode.substr(position, btSyntax.smlSign.length()) == btSyntax.smlSign) {
        return {operators::lesser, position + (int)btSyntax.smlSign.length(), true};
    }
    return {operators::plus, startPosition, false};
}


//napsano pomoci AI
//! -> notOperBit, ~ -> notOperByte, $ -> isPointer, kdyz tam prefix neni: {noSingleOper, startPosition}
goodSingleOperator findSingleOperator(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.notSign.length()) == btSyntax.notSign) {
        return {singleOperator::notOperBit, position + (int)btSyntax.notSign.length()};
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.negateBytes.length()) == btSyntax.negateBytes) {
        return {singleOperator::notOperByte, position + (int)btSyntax.negateBytes.length()};
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.isPointerSign.length()) == btSyntax.isPointerSign) {
        return {singleOperator::isPointer, position + (int)btSyntax.isPointerSign.length()};
    }
    return {singleOperator::noSingleOper, startPosition};
}


//separator: nextSign "," pri konstrukci typu / oddeleni argumentu funkce
//napsano pomoci AI
goodSeparator findSeparator(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.nextSign.length()) == btSyntax.nextSign) {
        return {position + (int)btSyntax.nextSign.length(), true};
    }
    return {startPosition, false};
}
