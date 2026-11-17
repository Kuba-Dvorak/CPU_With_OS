#include "./syntaxAnalyzer.cpp"




void createNewType(typesHandle &handle, std::string &originCode, std::string &name, int lastLine) {
    handle.startCreatingNewType(name, 0);
    while (true) {
        goodLine oneLine = loadNextLine(originCode, lastLine);
        if (!oneLine.exists) {
            break;
        }
        if (findSegmentEnd(oneLine.oneLine).exists) {
            break;
        }
        typePart oneTypePart = loadTypePart(oneLine.oneLine);
        handle.createPartNewType(oneTypePart.typeOf, oneTypePart.name, oneTypePart.isRef);
    }
    handle.stopCreatingNewType();
}