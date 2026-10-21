
#include "./syntaxOperations.cpp"
#include <cmath>
#include <vector>
#include <string>
#include <variant>


struct basicNode;


struct functionConstructor;


void prepareFnInput(int &textStartPos, int inputsFunqStartLoc, basicFunction* funq, std::string &oneLine, fullCompiledCode &code, segmentManager &scope);


struct basicNode {
    basicNode* parentNode = nullptr;
    basicNode* leftNode = nullptr;
    basicNode* rightNode = nullptr;
    int nodeIdCurr = -1;
    int textPosition = -1;
    nodeTypes myArType = nodeTypes::notWorking;
    bool preparedSpaceIsRef = false;
    bool negate = false;

    passerPair myPassePair;
    int wantedNum;

    basicFunction* funq;
    int textStartPos;
    
    basicNode* getRoot() {
        if (parentNode == nullptr) {
            return this;
        }
        return parentNode->getRoot();
    }
    
    place evaluateNumber(fullCompiledCode &code, segmentManager &segMan) {
        int sizeOfNum = std::ceil(std::log2f(wantedNum) / 8);
        place returnPlace = {sizeOfNum, segMan.reqFreeSub(sizeOfNum), preparedSpaceIsRef, false, inReg::reg1};
        setRamLocal(wantedNum, returnPlace.pointer, returnPlace.size, code);
        return returnPlace;
    }
    
    place evaluateVar(fullCompiledCode &code, segmentManager &segMan) {
        int thatSize = myPassePair.var->myTypee->size;
        if (preparedSpaceIsRef) {
            thatSize = addressSize;
        }
        place returnPlace = {thatSize, segMan.reqFreeSub(thatSize), preparedSpaceIsRef, false, inReg::reg1};
        myPassePair.var->call(myPassePair.keys, code, returnPlace, false);
        return returnPlace;
    }
    
    place evaluateFn(fullCompiledCode &code, segmentManager &segMan, std::string &oneLine) {
        int absOffset = 0;
        //output
        int outputLoc = segMan.reqFreeSub(funq->outputType->getSize());
        absOffset += funq->outputType->getSize();
        //jumpback
        segMan.reqFreeSub(addressSize);
        absOffset += addressSize;
        //inputy
        int startLoc = segMan.reqFreeSub(funq->getInputsSize());
        absOffset += funq->getInputsSize();
        prepareFnInput(textStartPos, startLoc, funq, oneLine, code, segMan);
        segMan.callDiffFunq(code, absOffset);
        funq->getCalled(code);
        //tady se nejspis uspesne zavolala funkce
        //tady ji zas ukoncime
        segMan.endFunq(code);
        return {funq->outputType->getSize(), outputLoc, funq->outputType->isRef, false, inReg::reg1};
    }

    //pred vracenim jeste chybi check a pripadna funkce pro negaci!!!
    place createBinary(fullCompiledCode &code, segmentManager &segMan, std::string &oneLine) {
        if (myArType == nodeTypes::notWorking || textPosition == -1 || nodeIdCurr == -1) {
            return {-2, -1, false, false, inReg::reg1};
        }

        if (myArType == nodeTypes::aritmo) {
            if (leftNode == nullptr || rightNode == nullptr) {
                return {-2, -1, false, false, inReg::reg1};
            }

            if (leftNode->textPosition < rightNode->textPosition) {

            }

            if (leftNode->textPosition > rightNode->textPosition) {

            }
        }

        if (myArType == nodeTypes::number) {

            return evaluateNumber(code, segMan);
        }

        if (myArType == nodeTypes::variable) {

            return evaluateVar(code, segMan);
        }

        if (myArType == nodeTypes::function) {

            return evaluateFn(code, segMan, oneLine);
        }


    }


    void placeToTree() {

    }
};


basicNode parseBT(int textStartPos, std::string &oneLine) {

}



//vraci place na kterem je ta vec ulozena
place createParsedSegment(typee* typePlace, segmentManager &scope, int &textStartPos, std::string &oneLine, fullCompiledCode &code) {
    //nejdriv rozdeleni bintree/typeConstructor
    //pokud plati putToIsOnEnd tak muzeme stavet od putTo dal, a nemusime vlastne skoro vubec kopirovat navic

    goodConstructStart ctrStr = findConstructStart(oneLine, textStartPos);
    if (ctrStr.exists) {
        //pro kazdy z typu zavolame zase nasi funkci
        textStartPos = ctrStr.position;
        int curPointer = scope.reqFreeSub(typePlace->size);
        int originalP = curPointer;
        for (int i = 0; i < typePlace->params.size(); i++) {
            place createdPlace = createParsedSegment(typePlace->params[i], scope, textStartPos, oneLine, code);
            copyFromTo(createdPlace, {typePlace->params[i]->size, curPointer, typePlace->isRef[i],
                false, inReg::reg1}, code, false);
            curPointer += typePlace->params[i]->size;
        }

        goodConstructStart ctrEnd = findConstructStart(oneLine, textStartPos);

        if (!ctrEnd.exists) {
            //chyba! po consturktoru nemuze byt nic dalsiho
        }

        textStartPos = ctrEnd.position;

        return {typePlace->size, originalP, false, false, inReg::reg1};
    }
    //takze to neni type constructor => binarni strom


}


//bereme to tak ze misto pro output a jumpBack uz je zabrane
void prepareFnInput(int &textStartPos, int inputsFunqStartLoc, basicFunction* funq, std::string &oneLine, fullCompiledCode &code, segmentManager &scope) {
    for (int i = 0; i < funq->inputsInner.size(); i++) {
        place placeTo = {funq->inputsInner[i]->getSize(), inputsFunqStartLoc, funq->inputsInner[i]->isRef, false, inReg::reg1};
        inputsFunqStartLoc += funq->inputsInner[i]->getSize();
        place artificialPlace = createParsedSegment(funq->inputsInner[i]->myTypee, scope, textStartPos, oneLine, code);
        copyFromTo(artificialPlace, placeTo, code, false);
    }
}