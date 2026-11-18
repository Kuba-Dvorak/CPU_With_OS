#include "./syntaxAnalyzer.cpp"




void createNewType(typesHandle &handle, std::string &originCode, std::string name, int &lastLine) {
    handle.startCreatingNewType(name, 0);
    while (true) {
        lastLine += 1;
        goodLine oneLine = loadNextLine(originCode, lastLine);
        if (!oneLine.exists) {
            break;
        }
        if (findSegmentEnd(oneLine.oneLine).exists) {
            break;
        }
        goodVarDecl oneTypePart = loadVarDef(oneLine.oneLine);
        handle.createPartNewType(oneTypePart.varTypeName, oneTypePart.varName, oneTypePart.isRef, oneTypePart.posibleSize);
    }
    handle.stopCreatingNewType();
}