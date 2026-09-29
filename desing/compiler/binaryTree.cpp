#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include "./main.cpp"


enum class operators {
    plus = 1,
    remove = 2,
    multiply = 3,
    divade = 4,
    voidOperator = 5,
    andOperator = 6,
    orOperator = 7,
    notOperator = 8,
    equal = 9,
    bigger = 10,
    lesser = 11,
    ebig = 12,
    lbig = 13,
    lesbig = 14
};

enum class nonAritmoNodes {
    //ve value ulozeno cislo
    functionType = 1,
    //ve value ulozen pointer
    // => nejdriv se musi udelat movRamReg
    variableType = 2,
    //ve value ulozen pointer na jeho oznacenou return pamet
    //=> jump na funkci
    numberType = 3,
    voidType = 4
};


struct basicSyntax {
    std::string setReg = " setReg ";
    std::string movRamReg = " movRamReg ";
    std::string reg1 = " R1 ";
    std::string reg2 = " R2 ";
    std::string reg0 = " R0 ";
    std::string reg3 = " R3 ";
    std::string reg4 = " R4 ";
    std::string addReg = " add8 ";// add8 works like this: var1 = reg code, var2 = adding number, if the reg code is 5, similary for var3 and var4
    std::string addRam = " addRam ";
    std::string movRegRam = " movRegRam ";
    std::string movRamRam = " movRamRam ";
    std::string movRegReg = " movRegReg ";
    std::string setRam = " setRam ";
    //reg 3 = carry out latch, reg 4 = carry in latch
};


struct mathSymbols {
    char plus = '+';
    char minus = '-';
    char multiply = '*';
    char andGate = '&';
    char orGate = '|';
    char notGate = '!';
    std::string equal = "==";
    char quickEqual = '=';
    char lesser = '<';
    char bigger = '>';
    std::string nonEq = "><";
    std::string lesEq = "<=";
    std::string bigEq = ">=";
    char bracketStart = '(';
    char bracketEnd = ')';
};


mathSymbols myMathSyntax;


basicSyntax myOwnsyntax;


int requestFreeSubPointer(int allocSize) {
    //dodelam pozdeji
    return 5;
}


void requestFreeingSize(int pointer, int freeingSize) {

}


struct nonAritmoNode {
    nonAritmoNodes type;
    int value;
    int numberSize;
    int textPosition;

    std::string evaluate(int toWhichToSave) {
        if (type == nonAritmoNodes::numberType) {
            if (toWhichToSave == 1) {
                return myOwnsyntax.setReg + myOwnsyntax.reg1 + std::to_string(value);
            }

            if (toWhichToSave == 2) {
                return myOwnsyntax.setReg + myOwnsyntax.reg2 + std::to_string(value);
            }

            if (toWhichToSave < 0) {
                toWhichToSave *= -1;
                std::string returnText = "";
                int sizeQuick = 0;
                while (true) {
                    sizeQuick += 1;
                    if (sizeQuick > numberSize) {
                        break;
                    }
                    returnText += myOwnsyntax.setRam + std::to_string(toWhichToSave + sizeQuick - 1) + " " +
                        std::to_string((value >> (8 * (sizeQuick - 1))) & 0xFF);
                }
                return returnText;
            }
        }

        if (type == nonAritmoNodes::variableType) {
            if (toWhichToSave == 1) {
                return myOwnsyntax.movRamReg + std::to_string(value) + myOwnsyntax.reg1;
            }

            if (toWhichToSave == 2) {
                return myOwnsyntax.movRamReg + std::to_string(value) + myOwnsyntax.reg2;
            }

            if (toWhichToSave < 0) {
                requestFreeingSize(toWhichToSave * -1, numberSize);
                return "";
            }
        }

        //funkce udelam pozdeji
        return "";
    }
};


void addingLeftOvers(std::string &text, int currentPlusPointer, int leftOverNodeSize, int leftOverNodePointer , int output) {
    text += myOwnsyntax.movRamReg + std::to_string(leftOverNodePointer + currentPlusPointer) + myOwnsyntax.reg1;
    text += myOwnsyntax.addReg + myOwnsyntax.reg1 + std::to_string(0);
    text += myOwnsyntax.movRegRam + myOwnsyntax.reg0 + std::to_string(output + currentPlusPointer);
    if (currentPlusPointer + 1 < leftOverNodeSize) {
        text += myOwnsyntax.movRegReg + myOwnsyntax.reg3 + myOwnsyntax.reg4;
        addingLeftOvers(text, currentPlusPointer + 1, leftOverNodeSize, leftOverNodePointer, output);
    }

    else {
        text += myOwnsyntax.movRegRam + myOwnsyntax.reg3 + " " + std::to_string(output + currentPlusPointer + 1);
    }
}


void addingOperation(std::string &text, int currentPlusPointer, int leftNodeSize, int rightNodeSize, int leftNodePointer, int rightNodePointer, int output) {
    text += myOwnsyntax.addRam + std::to_string(leftNodePointer + currentPlusPointer) + " " + std::to_string(rightNodePointer + currentPlusPointer);
    text += myOwnsyntax.movRegRam + myOwnsyntax.reg0 + std::to_string(output + currentPlusPointer);

    if (currentPlusPointer + 1 < std::min(leftNodeSize, rightNodeSize)) {
        text += myOwnsyntax.movRegReg + myOwnsyntax.reg3 + myOwnsyntax.reg4;
        addingOperation(text, currentPlusPointer + 1, leftNodeSize, rightNodeSize, leftNodePointer, rightNodePointer, output);
    }

    else {
        text += myOwnsyntax.movRegReg + myOwnsyntax.reg3 + myOwnsyntax.reg4;
        if (leftNodeSize == rightNodeSize) {
            text += myOwnsyntax.movRegRam + myOwnsyntax.reg3 + " " + std::to_string(output + currentPlusPointer + 1);
            return;
        }

        if (currentPlusPointer + 1 >= leftNodeSize) {
            addingLeftOvers(text, currentPlusPointer + 1, rightNodeSize, rightNodePointer, output);
        }

        else {
            addingLeftOvers(text, currentPlusPointer + 1, leftNodeSize, leftNodePointer, output);
        }
    }
}


int operatorPriority(char letter) {
    if (letter == myMathSyntax.notGate) return 1;
    if (letter == myMathSyntax.multiply) return 2;
    if (letter == myMathSyntax.plus || letter == myMathSyntax.minus) return 3;
    if (letter == myMathSyntax.andGate) return 4;
    if (letter == myMathSyntax.orGate) return 5;
    if (letter == myMathSyntax.lesser || letter == myMathSyntax.bigger) return 6;
    if (letter == myMathSyntax.quickEqual ) return 7;
    return 0;
}


int operatorPriority(operators op) {
    if (op == operators::notOperator) return 1;
    if (op == operators::multiply || op == operators::divade) return 2;
    if (op == operators::plus || op == operators::remove) return 3;
    if (op == operators::andOperator) return 4;
    if (op == operators::orOperator) return 5;
    if (op == operators::lesser || op == operators::bigger ||
        op == operators::lbig || op == operators::ebig) return 6;
    if (op == operators::equal || op == operators::lesbig) return 7;
    return 0;
}


operators intToOperator(int op) {

}


bool containsMath(char letter) {
    return operatorPriority(letter) > 0;
}

int nodeIdCur = 0;


struct aritmeticNode {
    operators nodeOperator;
    int nodeIdCurr;
    aritmeticNode* leftAritmoNode = nullptr;
    aritmeticNode* rightAritmoNode = nullptr;
    nonAritmoNode* leftNonANode = nullptr;
    nonAritmoNode* rightNonANode = nullptr;
    aritmeticNode* parent = nullptr;
    int outputPointer = -1;
    int outputSize = -1;

    aritmeticNode* getRoot() {
        if (parent == nullptr) {
            return this;
        }
        return parent->getRoot();
    }

    std::string createBinary() {
        std::string returnee = myOwnsyntax.setReg + myOwnsyntax.reg4 + "0";
        if (leftNonANode == nullptr || rightNonANode == nullptr) {
            //nemuzeme udelat zanoreni protoze nejsou jenom cisla
            if (leftNonANode == nullptr && (rightNonANode != nullptr)) {
                std::string returnee = leftAritmoNode->createBinary() + rightNonANode->evaluate(requestFreeSubPointer(rightNonANode->numberSize) * -1);

                if (nodeOperator == operators::plus) {
                    outputSize = std::max(leftAritmoNode->outputSize, rightNonANode->numberSize) + 1;
                    outputPointer = requestFreeSubPointer(outputSize);
                    addingOperation(returnee, 0, leftAritmoNode->outputSize, rightNonANode->numberSize,
                        leftAritmoNode->outputPointer, rightNonANode->value, outputPointer);
                }
                //dodelat dalsi operatory !!!

                return returnee;
            }

            if (leftNonANode != nullptr && (rightNonANode == nullptr)) {
                std::string returnee = leftNonANode->evaluate(requestFreeSubPointer(leftNonANode->numberSize) * -1) + rightAritmoNode->createBinary();

                if (nodeOperator == operators::plus) {
                    outputSize = std::max(leftNonANode->numberSize, rightAritmoNode->outputSize) + 1;
                    outputPointer = requestFreeSubPointer(outputSize);
                    addingOperation(returnee, 0, leftNonANode->numberSize, rightAritmoNode->outputSize,
                        leftNonANode->value, rightAritmoNode->outputPointer, outputPointer);
                }
                //dodelat dalsi operatory !!!

                return returnee;
            }

            std::string returnee = leftAritmoNode->createBinary() + rightAritmoNode->createBinary();

            if (nodeOperator == operators::plus) {
                outputSize = std::max(leftAritmoNode->outputSize, rightAritmoNode->outputSize) + 1;
                outputPointer = requestFreeSubPointer(outputSize);
                addingOperation(returnee, 0, leftAritmoNode->outputSize, rightAritmoNode->outputSize,
                       leftAritmoNode->outputPointer, rightAritmoNode->outputPointer, outputPointer);
            }
            //dodelat dalsi operatory !!!

            return returnee;
        }

        //nemuzeme udelat zanoreni s vystupem do registru, protoze nikdy si nemuzeme byt jisti co ta funkce dela, taky kdyz je cislo > 1 bajt tak musime pracovat sloziteji
        if (leftNonANode->type == nonAritmoNodes::functionType || rightNonANode->type == nonAritmoNodes::functionType || leftNonANode->numberSize > 1 || rightNonANode->numberSize > 1) {

            std::string returnee = leftNonANode->evaluate(requestFreeSubPointer(leftNonANode->numberSize) * -1) +
                                   rightNonANode->evaluate(requestFreeSubPointer(rightNonANode->numberSize) * -1);

            if (nodeOperator == operators::plus) {
                outputSize = std::max(leftNonANode->numberSize, rightNonANode->numberSize) + 1;
                outputPointer = requestFreeSubPointer(outputSize);
                addingOperation(returnee, 0, leftNonANode->numberSize, rightNonANode->numberSize,
                         leftNonANode->value, rightNonANode->value, outputPointer);
            }
            //dodelat dalsi operatory !!!

            return returnee;
        }

        //jinak dame na registry
        returnee = leftNonANode->evaluate(1) + rightNonANode->evaluate(2);
        if (nodeOperator == operators::plus) {
            returnee += myOwnsyntax.addReg;
            returnee += myOwnsyntax.reg1 + myOwnsyntax.reg2;
            outputSize = std::max(leftNonANode->numberSize, rightNonANode->numberSize) + 1;
            outputPointer = requestFreeSubPointer(outputSize);
            returnee += myOwnsyntax.movRegRam + myOwnsyntax.reg0 + std::to_string(outputPointer);
            returnee += myOwnsyntax.movRegRam + myOwnsyntax.reg3 + " " + std::to_string(outputPointer + 1);
        }

        //dodelat dalsi operatory !!!
        return returnee;
    }


    //to which to save to urcuje kam se da vysledek daneho stromu v registrech v CPU => 1 = R1, 2 = R2, 3 = outputPointer (musi do RAM)
    void saveToTree(operators newOperatorr, nonAritmoNode* itsAritmoNode, std::deque<aritmeticNode> &list, int lastOperatorTier, int secondLast, bool doSpecialThing = false) {
        int operatorThis = operatorPriority(nodeOperator);
        int operatorThat = operatorPriority(newOperatorr);


        if (doSpecialThing) {
            list.push_back({newOperatorr, nodeIdCur++, nullptr, rightAritmoNode,
                      itsAritmoNode, nullptr, this});
            rightAritmoNode->parent = &(list.back());
            rightAritmoNode = &(list.back());
            return;
        }


        //opakuje se
        if (nodeIdCurr == secondLast) {
            if (lastOperatorTier > operatorThis) {
                parent->saveToTree(newOperatorr, itsAritmoNode, list, nodeIdCurr, lastOperatorTier, true);
                return;
            }

            if (lastOperatorTier < operatorThis) {
                saveToTree(newOperatorr, itsAritmoNode, list, nodeIdCurr, lastOperatorTier, true);
                return;
            }
        }

        else {
            if (operatorThis < operatorThat) {
                if (parent != nullptr) {
                    parent->saveToTree(newOperatorr, itsAritmoNode, list, nodeIdCurr, lastOperatorTier);
                    return;
                }
                std::cout << "What the fuck, how can it not be nullptr, anyway, your code's broken sorry" << std::endl;
            }

            else if (operatorThis == operatorThat) {
                if (leftAritmoNode != nullptr) {
                    leftAritmoNode->saveToTree(newOperatorr, itsAritmoNode, list, nodeIdCurr, lastOperatorTier);
                    return;
                }
                list.push_back({newOperatorr, nodeIdCur++, nullptr, nullptr,
                                    leftNonANode, itsAritmoNode, this});
                leftAritmoNode = &(list.back());
                leftNonANode = nullptr;
            }

            else if (operatorThis > operatorThat) {
                if (rightAritmoNode != nullptr) {
                    rightAritmoNode->saveToTree(newOperatorr, itsAritmoNode, list, nodeIdCurr, lastOperatorTier);
                    return;
                }
                list.push_back({newOperatorr, nodeIdCur++, nullptr, nullptr,
                                    rightNonANode, itsAritmoNode, this});
                rightAritmoNode = &(list.back());
                rightNonANode = nullptr;
            }
        }
    }
};


bool isNumber(char oneChar) {
    if (oneChar == '1' || oneChar == '0' || oneChar == '2' || oneChar == '3' ||
        oneChar == '4' || oneChar == '5' || oneChar == '6' ||
        oneChar == '7' || oneChar == '8' || oneChar == '9') {
        return true;
    }
    return false;
}


bool isName(char c) {
    return (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'));
}


bool isBracket(char c) {
    if (c == '(' || c == ')') {
        return true;
    }
    return false;
}


struct goodInt {
    int value;
    int endingPos;
    bool isIt;
};


struct goodString {
    std::string value;
    int endingPos;
    bool isIt;
};


struct goodOperator {
    int operatorr;
    int endingPos;
};


//tohle psala AI protoze jsem fakt liny psat rutini funkce
goodInt loadNextNum(std::string &fileString, int position) {
    int value = 0;

    while (true) {
        if (position < (int)fileString.size()) {
            return {0, position, false};
        }
        char c = fileString[position];
        if (containsMath(c) || isName(c) || isBracket(c)) {
            return {0, position, false};
        }
        if (isNumber(c)) break;
        position++;
    }

    while (position < (int)fileString.size()) {
        char c = fileString[position];
        if (containsMath(c) || isName(c) || isBracket(c)) {
            return {0, position, false};
        }
        if (!isNumber(c)) break;
        value = value * 10 + (c - '0');
        position++;
    }
    return {value, position, true};
}


goodString loadNextName(std::string &fileString, int position) {
    std::string name;

    while (true) {
        if (position < (int)fileString.size()) {
            return {"", position, false};
        }
        char c = fileString[position];
        if (containsMath(c) || isNumber(c) || isBracket(c)) {
            return {"", position, false};
        }
        if (isName(c)) break;
        position++;
    }

    while (position < (int)fileString.size()) {
        char c = fileString[position];
        if (containsMath(c) || isNumber(c) || isBracket(c)) {
            return {"", position, false};
        }
        if (!isName(c)) break;
        name += c;
        position++;
    }
    return {name, position, true};
}


goodOperator loadOperator(std::string &fileString, int position) {
    while (true) {
        if (position >= (int)fileString.size()) return {0, position};
        char c = fileString[position];
        if (isName(c) || isNumber(c) || isBracket(c)) return {0, position};
        if (containsMath(c)) break;
        position++;
    }

    std::string two = fileString.substr(position, 2);
    char c = fileString[position];

    if (two == myMathSyntax.equal) return {(int)operators::equal, position + 2};
    else if (two == myMathSyntax.nonEq) return {(int)operators::lesbig, position + 2};
    else if (two == myMathSyntax.lesEq) return {(int)operators::lbig, position + 2};
    else if (two == myMathSyntax.bigEq) return {(int)operators::ebig, position + 2};
    else if (c == myMathSyntax.plus) return {(int)operators::plus, position + 1};
    else if (c == myMathSyntax.minus) return {(int)operators::remove, position + 1};
    else if (c == myMathSyntax.multiply) return {(int)operators::multiply, position + 1};
    else if (c == myMathSyntax.andGate) return {(int)operators::andOperator, position + 1};
    else if (c == myMathSyntax.orGate) return {(int)operators::orOperator, position + 1};
    else if (c == myMathSyntax.notGate) return {(int)operators::notOperator, position + 1};
    else if (c == myMathSyntax.lesser) return {(int)operators::lesser, position + 1};
    else if (c == myMathSyntax.bigger) return {(int)operators::bigger, position + 1};

    return {0, position};
}


struct variableInRam {
    int pointer;
    int sssize;
};


variableInRam askStorageForVariable(std::string name) {
    return {};
}


//zase pisu ja
//1. nacist operatora
//2. nacist operand

int loadOneNode(std::deque<aritmeticNode> &list, std::deque<nonAritmoNode> &listOfNons, std::string &quickCompile, std::string &text, int weakestOperator, int &position) {
    goodOperator op = loadOperator(text, position);
    if (op.operatorr == 0) {
        //konec vyrazu
        return 0;
    }
    position = op.endingPos;
    goodInt number = loadNextNum(text, position);
    if (!number.isIt) {
        goodString name = loadNextName(text, position);
        if (!name.isIt) {
            std::cout << "You messed up big time, no thing after a operator" << std::endl;
            return -1;
        }
        variableInRam var = askStorageForVariable(name.value);
        listOfNons.push_back({nonAritmoNodes::variableType, var.pointer, var.sssize, name.endingPos});
        position = name.endingPos;
    }
    else {
        if (!number.isIt) {
            std::cout << "You messed up big time, no thing after a operator" << std::endl;
        }
        listOfNons.push_back({nonAritmoNodes::variableType, number.value, sizeof(number.value), number.endingPos});
        position = number.endingPos;
    }

    if (weakestOperator > op.operatorr) {
        quickCompile += list.back().getRoot()->createBinary();

    }

    list.back().saveToTree(intToOperator(op.operatorr), &listOfNons.back(), list, 0, 0, false);
    return 1;
}


aritmeticNode operateBT(std::string &oneLineString) {

}











