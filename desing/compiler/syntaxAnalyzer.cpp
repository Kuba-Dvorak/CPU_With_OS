
#include "./lexicalAnalyzer.cpp"
#include <cmath>
#include <vector>
#include <string>
#include <variant>


struct basicNode;


struct functionConstructor;


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


struct functionConstructor {

};


basicNode parseBT(place putTo, int textStartPos, std::string &oneLine) {

}


//konci na ,
//nic jako typeConstructor neexistuje, vse se resi v tomhle if v ctrStr.exists
void createParsedSegment(place putTo, typee* typePlace, bool putToIsOnEnd, int &textStartPos, std::string &oneLine, fullCompiledCode &code) {
    //nejdriv rozdeleni bintree/typeConstructor
    //pokud plati putToIsOnEnd tak muzeme stavet od putTo dal, a nemusime vlastne skoro vubec kopirovat navic

    goodConstructStart ctrStr = findConstructStart(oneLine, textStartPos);
    if (ctrStr.exists) {

        if (putTo.isRef) {

        }

        textStartPos = ctrStr.position;
        int curIncPos = 0;
        for (int i = 0; i <= typePlace->params.size(); i++) {
            createParsedSegment();
            //clovek muze ukoncit konstruktor kdy chce
            goodConstructStart ctrEnd = findConstructStart(oneLine, textStartPos);
            if (ctrEnd.exists) {
                break;
            }
        }
    }
}

