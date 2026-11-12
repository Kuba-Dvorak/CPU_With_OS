
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
                place placeTo = {1, segMan.reqFreeSub(1), false, false, inReg::reg1};
                logicalOperation(placeReturner, {}, placeTo, code, logOperators::notBitOp);
                return placeTo;
            }

            if (singleOper == singleOperator::notOperByte) {
                place placeTo = {placeReturner.size, segMan.reqFreeSub(placeReturner.size), false, false, inReg::reg1};
                logicalOperation(placeReturner, {}, placeTo, code, logOperators::notByteOp);
                return placeTo;
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
            valueNode.parentNode = &operatorNode;
            if (parentNode->leftNode->nodeID == nodeID) {
                operatorNode.leftNode = &valueNode;
                operatorNode.parentNode = parentNode;
                operatorNode.rightNode = this;
                parentNode->leftNode = &operatorNode;
            }

            //jedna se o prave dite
            else {
                operatorNode.leftNode = this;
                operatorNode.parentNode = parentNode;
                operatorNode.rightNode = &valueNode;
                parentNode->rightNode = &operatorNode;
            }
            parentNode = &operatorNode;
            return;
        }

        int thisNodeLevel = operatorToLevel(oper);
        int newNodeLevel = operatorToLevel(operatorNode.oper);

        //novy operator je vzdy dal v textu => chronologicky se musi provest pozdeji => musi byt vys => prohodime to s timhle
        if (newNodeLevel == thisNodeLevel) {
            valueNode.parentNode = &operatorNode;
            operatorNode.rightNode = &valueNode;
            //jedna se o root
            if (parentNode == nullptr) {
                parentNode = &operatorNode;
                operatorNode.leftNode = this;
                return;
            }

            //jedna se o leve dite
            if (parentNode->leftNode->nodeID == nodeID) {
                parentNode->leftNode = &operatorNode;
                operatorNode.leftNode = this;
                operatorNode.parentNode = parentNode;
                parentNode = &operatorNode;
            }

            //jedna se o prave dite
            else {
                parentNode->rightNode = &operatorNode;
                operatorNode.leftNode = this;
                operatorNode.parentNode = parentNode;
                parentNode = &operatorNode;
            }

            return;
        }

        //jsme v loopu => musime protlacit mezi nas
        if (secondLastId == nodeID) {
            //dite = novy
            if (newNodeLevel > thisNodeLevel) {
                rightNode->placeToTree(operatorNode, valueNode, nodes, lastId, secondLastId, code, segMan, oneLine);
                return;
            }

            valueNode.parentNode = &operatorNode;

            //rodic = novy
            if (newNodeLevel < thisNodeLevel) {
                //root = k nemu nedojde

                //leve dite
                if (parentNode->leftNode->nodeID == nodeID) {
                    parentNode->leftNode = &operatorNode;
                }

                //prave dite
                else {
                    parentNode->rightNode = &operatorNode;
                }
                operatorNode.leftNode = this;
                operatorNode.rightNode = &valueNode;
                operatorNode.parentNode = parentNode;
                parentNode = &operatorNode;
            }
            return;
        }

        if (newNodeLevel > thisNodeLevel) {
            return rightNode->placeToTree(operatorNode, valueNode, nodes, nodeID, lastId, code, segMan, oneLine);
        }

        if (newNodeLevel < thisNodeLevel) {
            valueNode.parentNode = &operatorNode;
            if (parentNode == nullptr) {
                //toto implikuje ze to ma nizsi level nez root => dame to nad root a nynejsi root bude vlevo, cislo nove vpravo
                operatorNode.leftNode = this;
                operatorNode.rightNode = &valueNode;
                parentNode = &operatorNode;
                return;
            }
            return parentNode->placeToTree(operatorNode, valueNode, nodes, nodeID, lastId, code, segMan, oneLine);
        }
    }
};


basicNode parseBT(int &textStartPos, std::string &oneLine, fullCompiledCode &code, segmentManager &segMan,
    std::deque<basicNode> &nodes, int &nextId, varHandle &vars, functionHandle &funqHandle);


//vrati value node
basicNode getValueNode(int &textStartPos, std::string &oneLine, fullCompiledCode &code, segmentManager &segMan,
                        int &nextId, varHandle &vars, functionHandle &funqHandle, std::deque<basicNode> &nodes, singleOperator opertSingle) {
    goodVar firstpossibleVar = findVar(oneLine, textStartPos, vars);
    goodBracket firstpossibleBracket = findBracket(oneLine, textStartPos);
    goodNumber firstpossibleNumber = findNumber(oneLine, textStartPos);
    goodFunction firstpossibleFunction = findFunction(oneLine, textStartPos, funqHandle);
    basicNode returningNode;
    returningNode.nodeID = nextId;
    returningNode.singleOper = opertSingle;
    nextId += 1;

    //1. variable
    if (firstpossibleVar.exists) {
        returningNode.myPassePair.keys = firstpossibleVar.varKeys;
        returningNode.myPassePair.var = firstpossibleVar.var;
        returningNode.myArType = nodeTypes::variable;
        textStartPos = firstpossibleVar.position;
        return returningNode;
    }
    //2. funkce
    else if (firstpossibleFunction.exists) {
        returningNode.funq = firstpossibleFunction.func;
        returningNode.myArType = nodeTypes::function;
        textStartPos = firstpossibleFunction.position;
        return returningNode;
    }
    //3. number
    else if (firstpossibleNumber.exists) {
        returningNode.wantedNum = firstpossibleNumber.value;
        returningNode.myArType = nodeTypes::number;
        textStartPos = firstpossibleNumber.position;
        return returningNode;
    }

    //4. zavorka
    else if (firstpossibleBracket.exists) {
        returningNode.myArType = nodeTypes::funqPlace;
        textStartPos = firstpossibleBracket.position;
        basicNode valueNode = parseBT(textStartPos, oneLine, code, segMan, nodes, nextId, vars, funqHandle);
        returningNode.potPlace = valueNode.createBinary(code, segMan, oneLine);
        return returningNode;
    }

    //5. chyba
    else {
        return {nullptr, nullptr};
    }
}


//odevzda node s typem funqPlace a potPlacem
basicNode parseBT(int &textStartPos, std::string &oneLine, fullCompiledCode &code, segmentManager &segMan,
    std::deque<basicNode> &nodes, int &nextId, varHandle &vars, functionHandle &funqHandle) {

    nodes.push_back(getValueNode(textStartPos, oneLine, code, segMan, nextId, vars, funqHandle, nodes, singleOperator::noSingleOper));
    //nejdriv nacte cislo, var, funkci nebo zavorku
    basicNode* overNode = &nodes.back();
    bool firstThrought = true;

    //nacita vyrazi stylem:  operator, single operator, zavorka, variable / funq / number; priklad:  + !x
    while (true) {
        //nacist operator
        goodOperator opert = findOperator(oneLine, textStartPos);
        //konec stromu
        if (!opert.exists) {
            break;
        }
        textStartPos = opert.position;

        //nacist single operator
        goodSingleOperator opertSingle = findSingleOperator(oneLine, textStartPos);
        if (opertSingle.op != singleOperator::noSingleOper) {
            textStartPos = opertSingle.position;
        }

        int opertID = nodes.size();
        nodes.push_back({nullptr, nullptr, nullptr, opert.position,
            nodeTypes::aritmo, nextId++, {},
            0, nullptr, 0, singleOperator::noSingleOper, opert.op});
        textStartPos = opertSingle.position;
        if (firstThrought) {
            firstThrought = false;
            nodes[opertID].leftNode = overNode;
            nodes.push_back(getValueNode(textStartPos, oneLine, code, segMan, nextId, vars, funqHandle, nodes, opertSingle.op));
            int valueID = nodes.size() - 1;
            nodes[opertID].rightNode = &nodes[valueID];
        }
        else {
            nodes.push_back(getValueNode(textStartPos, oneLine, code, segMan, nextId, vars, funqHandle, nodes, opertSingle.op));
            int valueID = nodes.size() - 1;
            overNode->placeToTree(nodes[opertID], nodes[valueID], nodes, -1, -1, code, segMan, oneLine);
            overNode = overNode->getRoot();
        }
    }

    //nakonec vytvorit binarku z bt segmentu, a pak node s pot placem
    place potPlace = overNode->getRoot()->createBinary(code, segMan, oneLine);
    basicNode quickNode;
    quickNode.potPlace = potPlace;
    quickNode.nodeID = nextId++;
    nodes.push_back(quickNode);
    return nodes.back();
}



//vraci place na kterem je ta vec ulozena a uz je vytvorena
place createParsedSegment(typee* typePlace, segmentManager &scope, int &textStartPos, std::string &oneLine, fullCompiledCode &code) {
    //nejdriv rozdeleni bintree/typeConstructor
    //pokud plati putToIsOnEnd tak muzeme stavet od putTo dal, a nemusime vlastne skoro vubec kopirovat navic
    int takenUpSpace = 0;
    //do takenUpSpace se NEPOCITA return place
    goodConstructStart ctrStr = findConstructStart(oneLine, textStartPos);
    if (ctrStr.exists) {
        //pro kazdou promenou z typu zavolame zase nasi funkci
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
    scope.myCache.curSize -= takenUpSpace;
}


//bereme to tak ze misto pro output a jumpBack uz je zabrane
void prepareFnInput(int &textStartPos, int inputsPointerStart, basicFunction* funq, std::string &oneLine, fullCompiledCode &code, segmentManager &scope) {
    for (int i = 0; i < funq->inputsInner.size(); i++) {
        place placeTo = {funq->inputsInner[i]->getSize(), inputsPointerStart, funq->inputsInner[i]->isRef, false, inReg::reg1};
        inputsPointerStart += funq->inputsInner[i]->getSize();
        place artificialPlace = createParsedSegment(funq->inputsInner[i]->myTypee, scope, textStartPos, oneLine, code);
        copyFromTo(artificialPlace, placeTo, code, false);
        goodSeparator separator = findSeparator(oneLine, textStartPos);
        if (((i + 1) < funq->inputsInner.size()) || !separator.exists) {
            break;
        }
        textStartPos = separator.position;
    }
    //hledani ukonceni funkce -> )
    goodBracket bracket = findBracket(oneLine, textStartPos);
    if (!bracket.exists) {
        //chyba v ukonceni funkce
    }
    textStartPos = bracket.position;
}

