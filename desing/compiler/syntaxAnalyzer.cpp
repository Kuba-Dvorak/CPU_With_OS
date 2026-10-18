#include "./memoryManagment/functions.cpp"
#include <cmath>
#include <vector>
#include <string>


enum class nodeTypes {
    notWorking = -1,
    aritmo = 1,
    number = 2,
    variable = 3,
    function = 4
};


enum class singleOperator {
    notOper = 1,
    isPointer = 2
};


struct varPairer {
    variable* var;
    std::vector<std::string> varKeys;
    std::vector<singleOperator> whatToDoAfter = {};
};


enum class operators {
    plus = 1,
    remove = 2,
    multiply = 3,
    divade = 4,
    andOperator = 6,
    orOperator = 7,
    equal = 9,
    bigger = 10,
    lesser = 11,
    ebig = 12,
    lbig = 13,
    notEqual = 14
};


void callTypeConstructor() {

}


void callFunctionConstructor() {

}


struct basicNode {
    basicNode* parentNode = nullptr;
    basicNode* leftNode = nullptr;
    basicNode* rightNode = nullptr;
    int nodeIdCurr = -1;
    int textPosition = -1;
    nodeTypes myArType = nodeTypes::notWorking;
    
    basicNode* getRoot() {
        if (parentNode == nullptr) {
            return this;
        }
        return parentNode->getRoot();
    }
    
    void evaluateNumber() {
        
    }
    
    void evaluateVar() {
        
    }
    
    void evaluateFn() {
        callFunctionConstructor();
    }

    bool createBinary(fullCompiledCode &code, segmentManager &segMan) {
        if (myArType == nodeTypes::notWorking || textPosition == -1 || nodeIdCurr == -1) {
            return false;
        }

        if (myArType == nodeTypes::aritmo) {
            if (leftNode == nullptr || rightNode == nullptr) {
                return false;
            }
            if (leftNode->textPosition < rightNode->textPosition) {

            }

            if (leftNode->textPosition > rightNode->textPosition) {

            }
        }

        if (myArType == nodeTypes::number) {

            return true;
        }

        if (myArType == nodeTypes::variable) {

            return true;
        }

        if (myArType == nodeTypes::function) {

            return true;
        }


    }

    void placeToTree() {

    }

};


