#include "./lexicalSyntax.cpp"


syntaxDefiners lexSyntax;
syntaxForBT btSyntax;


//napsano pomoci AI
//dopredne deklarace funkci z nizsi casti souboru, pouzivaji je i funkce nahore
std::string loadName(std::string &lineCode, int startPosition);
goodNumber findNumber(std::string &lineCode, int startPosition);
goodBracket findStartBracket(std::string &lineCode, int startPosition);
goodBracket findFunqStrBrc(std::string &lineCode, int startPosition);
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
//var JMENO: [$]TYP[(N)]; -> {JMENO, TYP, je tam $, N}, bez (N) je velikost 2 B
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

    //dvojtecka mezi jmenem a typem
    if (position < oneLine.length() && oneLine.substr(position, lexSyntax.varTypeSeparator.length()) == lexSyntax.varTypeSeparator) {
        position += lexSyntax.varTypeSeparator.length();
    }
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
//function JMENO(P1, P2) [$]TYP {;   parametr je [$]TYP JMENO
//parametry muzou byt v ( ) nebo v /_ _/ (v zadani je ( ), autor psal /_ _/)
//number(N) v hlavicce funkce zatim neni, struktura pro to nema pole
goodLoadedFunctionDefinition loadDefFunq(std::string &oneLine) {
    goodLoadedFunctionDefinition result = {"", {}, {}, {}, "", false};
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

    //zacatek parametru
    std::string paramEnd = btSyntax.normalBracketEnd;
    goodBracket paramStart = findStartBracket(oneLine, position);
    if (!paramStart.exists) {
        paramStart = findFunqStrBrc(oneLine, position);
        paramEnd = btSyntax.functionCallEnd;
    }
    if (!paramStart.exists) {
        return result;
    }
    position = paramStart.position;

    //parametry [$]TYP JMENO oddelene carkou az po konec zavorky
    while (true) {
        while (position < oneLine.length() && oneLine[position] == ' ') {
            position += 1;
        }
        if (position >= oneLine.length()) {
            break;
        }
        if (oneLine.substr(position, paramEnd.length()) == paramEnd) {
            position += paramEnd.length();
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
    result.outputType = loadName(oneLine, position);
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
//jmeno jsou jen pismena a _, mezery na zacatku preskoci, kdyz tam jmeno neni vrati ""
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
        if (!isLetter && oneChar != '_') {
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
//bez prefixu ($ a ! zpracovava parseBT), klice se tu nekontroluji, jen nactou
//hleda ve varList odzadu (vnitrni scope stini vnejsi) a nic nevypisuje, proto ne pres returnVar
goodVar findVar(std::string &lineCode, int startPosition, varHandle &vars) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    std::string name = loadName(lineCode, position);
    if (name.empty() || name == lexSyntax.callFunq) {
        return {nullptr, {}, startPosition, false};
    }
    variable* foundVar = nullptr;
    for (int i = vars.varList.size() - 1; i >= 0; i--) {
        if (vars.varList[i].name == name) {
            foundVar = &vars.varList[i];
            break;
        }
    }
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
//hleda ve functionList bez vypisu, proto ne pres functionHandle::findFunction
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

    basicFunction* foundFunq = nullptr;
    for (int i = 0; i < funcs.functionList.size(); i++) {
        if (funcs.functionList[i].name == name) {
            foundFunq = &funcs.functionList[i];
            break;
        }
    }
    if (foundFunq == nullptr) {
        return {nullptr, startPosition, startPosition, false};
    }

    //odpovidajici konec volani: /_ +1, _/ -1
    int depth = 1;
    int endPosition = argsStart.position;
    while (endPosition < lineCode.length() && depth > 0) {
        if (lineCode.substr(endPosition, btSyntax.functionCallEnd.length()) == btSyntax.functionCallEnd) {
            depth -= 1;
            endPosition += btSyntax.functionCallEnd.length();
        }
        else if (lineCode.substr(endPosition, btSyntax.functionCallStart.length()) == btSyntax.functionCallStart) {
            depth += 1;
            endPosition += btSyntax.functionCallStart.length();
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
//prefixy ($, !) tu nectu, ty zpracovava parseBT pres findSingleOperator
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
    if (lineCode.substr(position, btSyntax.functionCallStart.length()) == btSyntax.functionCallStart) {
        return {operators::plus, startPosition, false};
    }
    //poradi je dulezite: nejdriv 2 znakove
    std::vector<std::string> signs = {btSyntax.equalSign, btSyntax.eqBigSign, btSyntax.eqSmlSign, btSyntax.notEqualSign,
        btSyntax.plusSign, btSyntax.minusSign, btSyntax.timesSign, btSyntax.divideSign,
        btSyntax.andSign, btSyntax.orSign, btSyntax.bigSign, btSyntax.smlSign};
    std::vector<operators> ops = {operators::equal, operators::ebig, operators::esml, operators::notEqual,
        operators::plus, operators::remove, operators::multiply, operators::divade,
        operators::andOperator, operators::orOperator, operators::bigger, operators::lesser};
    for (int i = 0; i < signs.size(); i++) {
        if (lineCode.substr(position, signs[i].length()) == signs[i]) {
            return {ops[i], position + (int)signs[i].length(), true};
        }
    }
    return {operators::plus, startPosition, false};
}


//napsano pomoci AI
//! -> notOperBit, $ -> isPointer, kdyz tam prefix neni: {noSingleOper, startPosition}
goodSingleOperator findSingleOperator(std::string &lineCode, int startPosition) {
    int position = startPosition;
    while (position < lineCode.length() && lineCode[position] == ' ') {
        position += 1;
    }
    if (position < lineCode.length() && lineCode.substr(position, btSyntax.notSign.length()) == btSyntax.notSign) {
        return {singleOperator::notOperBit, position + (int)btSyntax.notSign.length()};
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
