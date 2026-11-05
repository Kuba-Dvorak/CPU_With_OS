
#include "./syntaxOperations.cpp"
#include <cmath>
#include <vector>
#include <string>
#include <variant>
#include <oneapi/tbb/partitioner.h>


//v tomhle soubore jsem vynechal detekci chyb spatne napsaneho kodu, stejne jako ve zbytku kompilatoru

struct basicNode;


void prepareFnInput(int &textStartPos, int inputsFunqStartLoc, basicFunction* funq, std::string &oneLine, fullCompiledCode &code, segmentManager &scope);


//co jde chronologicky ve vypoctu driv ma vyssi prioritu: napr: nasobeni ma vyssi level nez scitani
int operatorToLevel(operators oper) {

}


struct basicNode {
    basicNode* parentNode = nullptr;
    basicNode* leftNode = nullptr;
    basicNode* rightNode = nullptr;
    int textPosition = -1;
    nodeTypes myArType = nodeTypes::notWorking;
    int nodeID = -1;

    //pro variable
    passerPair myPassePair;

    //pro cislo
    int wantedNum;

    //pro funkci
    basicFunction* funq;
    int textStartPos;

    //pro operaci
    singleOperator singleOper;
    operators oper;

    //pro uz zpracovany place
    place potPlace;
    
    basicNode* getRoot() {
        if (parentNode == nullptr) {
            return this;
        }
        return parentNode->getRoot();
    }
    
    place evaluateNumber(fullCompiledCode &code, segmentManager &segMan) {
        bool preparedSpaceIsRef = false;
        if (singleOper == singleOperator::isPointer) {
            preparedSpaceIsRef = true;
        }
        int sizeOfNum = (int)std::ceil(std::log2f(wantedNum)) / 8;
        place returnPlace = {sizeOfNum, segMan.reqFreeSub(sizeOfNum), preparedSpaceIsRef, false, inReg::reg1};
        setRamLocal(wantedNum, returnPlace.pointer, returnPlace.size, code);
        if (singleOper == singleOperator::notOperBit) {
            logicalOperation(returnPlace, {}, returnPlace, code, logOperators::notBitOp);
        }
        if (singleOper == singleOperator::notOperByte) {
            logicalOperation(returnPlace, {}, returnPlace, code, logOperators::notByteOp);
        }
        return returnPlace;
    }
    
    place evaluateVar(fullCompiledCode &code, segmentManager &segMan) {
        bool preparedSpaceIsRef = false;
        if (singleOper == singleOperator::isPointer) {
            preparedSpaceIsRef = true;
        }
        int thatSize = myPassePair.var->myTypee->size;
        if (preparedSpaceIsRef) {
            thatSize = addressSize;
        }
        place returnPlace = {thatSize, segMan.reqFreeSub(thatSize), preparedSpaceIsRef, false, inReg::reg1};
        myPassePair.var->call(myPassePair.keys, code, returnPlace, false);
        if (singleOper == singleOperator::notOperBit) {
            logicalOperation(returnPlace, {}, returnPlace, code, logOperators::notBitOp);
        }
        if (singleOper == singleOperator::notOperByte) {
            logicalOperation(returnPlace, {}, returnPlace, code, logOperators::notByteOp);
        }
        return returnPlace;
    }
    
    place evaluateFn(fullCompiledCode &code, segmentManager &segMan, std::string &oneLine) {
        bool preparedSpaceIsRef = false;
        if (singleOper == singleOperator::isPointer) {
            preparedSpaceIsRef = true;
        }
        int absOffset = 0;
        //output
        segMan.reqFreeSub(funq->outputType->getSize());
        absOffset += funq->outputType->getSize();
        //jumpback
        segMan.reqFreeSub(addressSize);
        absOffset += addressSize;
        //inputy
        int startLoc = segMan.reqFreeSub(funq->getInputsSize());
        absOffset += funq->getInputsSize();
        prepareFnInput(textStartPos, startLoc, funq, oneLine, code, segMan);
        //volani
        segMan.callDiffFunq(code, absOffset);
        funq->getCalled(code);
        //tady se nejspis uspesne zavolala funkce se skokem v instrukcich
        //tady ji zas ukoncime
        segMan.endFunq(code);
        int outputLoc = segMan.reqFreeSub(funq->outputType->getSize());
        place returnPlace = {funq->outputType->getSize(), outputLoc, funq->outputType->isRef || preparedSpaceIsRef, false, inReg::reg1};
        if (singleOper == singleOperator::notOperBit) {
            logicalOperation(returnPlace, {}, returnPlace, code, logOperators::notBitOp);
        }
        if (singleOper == singleOperator::notOperByte) {
            logicalOperation(returnPlace, {}, returnPlace, code, logOperators::notByteOp);
        }
        return returnPlace;
    }

    //funkce ktera je repetetivni if-else strom, napsana pomoci claude code
    place evaluateOperator(fullCompiledCode &code, segmentManager &segMan, place &numA, place &numB) {
        int biggerSize = std::max(numA.size, numB.size);

        if (oper == operators::plus) {
            //+1 bajt na preteceni
            place placeTo = {biggerSize + 1, segMan.reqFreeSub(biggerSize + 1), false, false, inReg::reg1};
            addPlaces(numA, numB, placeTo, code);
            return placeTo;
        }

        if (oper == operators::remove) {
            //+1 bajt na preteceni
            place placeTo = {biggerSize + 1, segMan.reqFreeSub(biggerSize + 1), false, false, inReg::reg1};
            substitudePlaces(numA, numB, placeTo, code);
            return placeTo;
        }

        if (oper == operators::multiply) {
            //soucin se vzdy vejde do soucetu velikosti
            place placeTo = {numA.size + numB.size, segMan.reqFreeSub(numA.size + numB.size), false, false, inReg::reg1};
            multiplyPlaces(numA, numB, placeTo, code);
            return placeTo;
        }

        if (oper == operators::divade) {
            //podil neni nikdy vetsi nez delenec
            place placeTo = {numA.size, segMan.reqFreeSub(numA.size), false, false, inReg::reg1};
            dividePlaces(numA, numB, placeTo, code, segMan);
            return placeTo;
        }

        if (oper == operators::andOperator) {
            place placeTo = {biggerSize, segMan.reqFreeSub(biggerSize), false, false, inReg::reg1};
            logicalOperation(numA, numB, placeTo, code, logOperators::andOp);
            return placeTo;
        }

        if (oper == operators::orOperator) {
            place placeTo = {biggerSize, segMan.reqFreeSub(biggerSize), false, false, inReg::reg1};
            logicalOperation(numA, numB, placeTo, code, logOperators::orOp);
            return placeTo;
        }

        if (oper == operators::xorOperator) {
            place placeTo = {biggerSize, segMan.reqFreeSub(biggerSize), false, false, inReg::reg1};
            logicalOperation(numA, numB, placeTo, code, logOperators::xorOp);
            return placeTo;
        }

        if (oper == operators::equal || oper == operators::notEqual || oper == operators::bigger ||
            oper == operators::lesser || oper == operators::ebig || oper == operators::esml) {
            //vysledek porovnani je 1 bajt (0 / 1)
            place placeTo = {1, segMan.reqFreeSub(1), false, false, inReg::reg1};
            comparePlaces(numA, numB, placeTo, code, oper, segMan);
            return placeTo;
        }

        //neznamy operator
        return {-2, -1, false, false, inReg::reg1};
    }


    place createBinary(fullCompiledCode &code, segmentManager &segMan, std::string &oneLine) {
        if (myArType == nodeTypes::notWorking || textPosition == -1 || nodeID == -1) {
            return {-2, -1, false, false, inReg::reg1};
        }

        if (myArType == nodeTypes::aritmo) {
            if (leftNode == nullptr || rightNode == nullptr) {
                return {-2, -1, false, false, inReg::reg1};
            }

            place numA = leftNode->createBinary(code, segMan, oneLine);
            place numB = rightNode->createBinary(code, segMan, oneLine);
            bool preparedSpaceIsRef = false;
            place placeReturner;
            if (singleOper == singleOperator::isPointer) {
                preparedSpaceIsRef = true;
            }

            if (leftNode->textPosition < rightNode->textPosition) {
                placeReturner = evaluateOperator(code, segMan, numA, numB);
            }

            if (leftNode->textPosition > rightNode->textPosition) {
                placeReturner = evaluateOperator(code, segMan, numB, numA);
            }

            if (singleOper == singleOperator::notOperBit) {
                logicalOperation(placeReturner, {}, placeReturner, code, logOperators::notBitOp);
            }
            if (singleOper == singleOperator::notOperByte) {
                logicalOperation(placeReturner, {}, placeReturner, code, logOperators::notByteOp);
            }

            return {placeReturner.size, placeReturner.pointer, preparedSpaceIsRef, false, inReg::reg1};
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

        if (myArType == nodeTypes::funqPlace) {
            return potPlace;
        }

        return {-2, -1};
    }


    void placeToTree(basicNode &operatorNode, basicNode &valueNode, std::deque<basicNode> &nodes, int lastId, int secondLastId,
        fullCompiledCode &code, segmentManager &segMan, std::string &oneLine) {

        //jedna se o konec listu, nas node se udela ditetem parenta
        if (myArType != nodeTypes::aritmo) {
            //jedna se o leve dite
            if (parentNode->leftNode->nodeID == nodeID) {
                parentNode->leftNode = &operatorNode;
                operatorNode.leftNode = this;
                operatorNode.parentNode = parentNode;
                operatorNode.rightNode = &valueNode;
                parentNode = &operatorNode;
            }

            //jedna se o prave dite
            else {
                parentNode->rightNode = &operatorNode;
                operatorNode.leftNode = this;
                operatorNode.parentNode = parentNode;
                operatorNode.rightNode = &valueNode;
                parentNode = &operatorNode;
            }
            return;
        }

        int thisNodeLevel = operatorToLevel(oper);
        int newNodeLevel = operatorToLevel(operatorNode.oper);
        if (newNodeLevel == thisNodeLevel) {
            return leftNode->placeToTree(operatorNode, valueNode, nodes, nodeID, lastId, code, segMan, oneLine);
        }

        //jsme v loopu => musime protlacit mezi nas
        if (secondLastId == nodeID) {
            //rodic -> stary -> novy
            if (newNodeLevel > thisNodeLevel) {
                rightNode->parentNode = &operatorNode;
                operatorNode.rightNode = rightNode;
                rightNode = &operatorNode;
                operatorNode.leftNode = &valueNode;
            }

            //rodic -> novy -> stary
            if (newNodeLevel < thisNodeLevel) {
                //leve dite
                if (parentNode->leftNode->nodeID == nodeID) {
                    parentNode->leftNode = &operatorNode;
                }

                //prave dite
                else {
                    parentNode->rightNode = &operatorNode;
                }
                operatorNode.leftNode = &valueNode;
                operatorNode.rightNode = this;
                parentNode = &operatorNode;
            }
            return;
        }

        if (newNodeLevel > thisNodeLevel) {
            return rightNode->placeToTree(operatorNode, valueNode, nodes, nodeID, lastId, code, segMan, oneLine);
        }
        if (newNodeLevel < thisNodeLevel) {
            if (parentNode == nullptr) {
                //toto implikuje ze to ma nizsi level nez root => zkompilujeme tuhle 1 cast a potom delame jako kdyby tohle byla "promena"
                nodes.push_back({&operatorNode, nullptr, nullptr, textPosition, nodeTypes::funqPlace,
                                     nodeID, {}, 0, nullptr, 0, singleOperator::noSingleOper, operators::plus,
                                     createBinary(code, segMan, oneLine)});
                operatorNode.leftNode = &nodes.back();
                operatorNode.rightNode = &valueNode;
                return;
            }
            return parentNode->placeToTree(operatorNode, valueNode, nodes, nodeID, lastId, code, segMan, oneLine);
        }
    }
};


basicNode parseBT(int textStartPos, std::string &oneLine) {

}



//vraci place na kterem je ta vec ulozena a uz je vytvorena
place createParsedSegment(typee* typePlace, segmentManager &scope, int &textStartPos, std::string &oneLine, fullCompiledCode &code) {
    //nejdriv rozdeleni bintree/typeConstructor
    //pokud plati putToIsOnEnd tak muzeme stavet od putTo dal, a nemusime vlastne skoro vubec kopirovat navic
    int takenUpSpace = 0;
    //do takenUpSpace se NEPOCITA return place
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

    //TODO: dodelat delani BT

    scope.myCache.curSize -= takenUpSpace;
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

