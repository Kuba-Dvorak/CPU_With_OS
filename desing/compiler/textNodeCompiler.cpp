#include <iostream>
#include <vector>
#include <string>
#include <memory>


enum class linePartName {
    nameNode = 1,
    binaryTree = 2,
    ifStatement = 5,
    elseStatement = 6,
    whileCycle = 7,
    forCycle = 8,
    continueStatement = 9,
    breakStatement = 10,
    returnStatement = 11,

    typeNode = 12,
    structNode = 13,
    functionNode = 14,
    comperar = 15,
    codeSection = 16,
    paramsSection = 17,
};


struct specificNames {
    char endOfLine = ';';
    std::string functionNotation = "function";
    std::string returnNotation = "return";
    std::string structNotation = "struct";
    std::string forNotation = "for";
    std::string whileNotation = "while";
    std::string ifNotation = "if";
    std::string continueNotation = "continue";
    std::string elseNotation = "else";
    std::string breakNotation = "break";
    char setter = '=';
    std::string comperar = "==";
    char codeSectionStart = '{';
    char codeSectionEnd = '}';
    char paramsSectionStart = '(';
    char paramsSectionEnd = ')';
};

specificNames specificNames;


bool isNumber(char oneChar) {
    if (oneChar == '1' || oneChar == '0' || oneChar == '2' || oneChar == '3' || oneChar == '4' || oneChar == '5' || oneChar == '6' || oneChar == '7' || oneChar == '8' || oneChar == '9') {
        return true;
    }
    return false;
}


//tohle psala AI protoze jsem fakt liny psat rutini funkce
int loadNextNum(std::string &fileString, int &position) {
    int value = 0;
    while (position < (int)fileString.size() && isNumber(fileString[position])) {
        value = value * 10 + (fileString[position] - '0');
        position++;
    }
    return value;
}


std::string loadString(std::string &fileString, int &position) {
    while (position < (int)fileString.size() && fileString[position] == ' ') position++;
    std::string text;
    while (position < (int)fileString.size() && fileString[position] != ' ') {
        text += fileString[position];
        position++;
    }
    return text;
}


bool verifyName(const std::string &name) {
    if (name.empty()) return false;
    for (char c : name) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')) return false;
    }
    return true;
}


struct NodeArrayNode {
    int nodeType;
    std::string stringValue = "";
    int intValue = 0;


};



void compileRow(std::string &fileString, int &position) {
    while (true) {
        position += 1;
        if (fileString[position] == specificNames.endOfLine) {
            break;
        }
    }
}


