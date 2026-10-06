 #include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include "./syntax.cpp"

#define numType "number"
#define voidType "void"
#define STATIC_RAM "static"
#define DYNAMIC_RAM "dynamic"


struct fullCompiledCode {
    std::vector<std::string> code = {};
    int currentLine = 0;

    void addInstruction(std::string instrText) {
        code.push_back(instrText);
        currentLine += 1;
    }

    void changeBackFunq(std::string instrText, int index) {
        code[index] = instrText;
    }
};


struct staticDynamicPointr {
    std::string ramStart;
    int pointer = -1;

    std::string getString() {
        return (" " + ramStart + " " + std::to_string(pointer));
    }
};


struct numberVar {
    staticDynamicPointr pointer;
    int sizeB;
    bool isRef;
};


//TODO: dost pravdepodobne ze typee, a dalsi tyhle funkce maji otocene CIL, ZDROJ v asm delani kodu
struct typee {
    std::string name;
    std::vector<typee*> params;
    std::vector<std::string> paramKeys;
    int size;
    std::vector<bool> isRef;

    typee* findParamOnKey(std::string &key, int &diff, bool &wasITRef) {
        for (int i = 0; i < params.size(); i++) {
            if (paramKeys[i] == key) {
                wasITRef = isRef[i];
                return params[i];
            }
            if (isRef[i]) {
                diff += 2;
                continue;
            }
            diff += params[i]->size;
        }
        //non existent parameter
        return nullptr;
    }

    //pokud z toho vyjde wasReferenced jako true, tak nam tahle funkce garantuje ze pointer na to misto je v Reg1!!!
    numberVar paramPointer(int pointer0, std::string const &ramStartPlace, bool &wasReferenced, bool aboutThis, fullCompiledCode &compilingCode, std::vector<std::string> &keysAfter) {
        if (name == voidType) {
            return {{ramStartPlace, -1}, -1, false};
        }
        if (wasReferenced) {
            //kdyz jsme ukoncili radu, tak musime tohle vratit
            if (keysAfter.size() <= 0) {
                return {{ramStartPlace, 0}, size, aboutThis};
            }

            //kdyz je to cislo tak musime vratit to cislo o velikosti cislo, protoze cislo pak pod sebou nema dalsi
            if (name == numType) {
                return {{ramStartPlace, 0}, size, aboutThis};
            }

            int toAdd = 0;
            bool someRandom = false;
            typee* lookedFor = findParamOnKey(keysAfter[0], toAdd, someRandom);
            keysAfter.erase(keysAfter.begin());

            if (toAdd != 0) {
                compilingCode.addInstruction(asmSyntax.setReg + asmSyntax.reg2 + " " + std::to_string(toAdd));
                compilingCode.addInstruction(asmSyntax.addReg + asmSyntax.reg2 + asmSyntax.reg1);
            }

            compilingCode.addInstruction(asmSyntax.movRegReg + asmSyntax.reg1 + asmSyntax.reg0);

            if (someRandom) {
                compilingCode.addInstruction(asmSyntax.movRamReg + asmSyntax.reg1 + asmSyntax.addr0);
            }

            return lookedFor->paramPointer(0, ramStartPlace, wasReferenced, someRandom, compilingCode, keysAfter);
        }

        else {
            //kdyz jsme ukoncili radu, tak musime tohle vratit
            if (keysAfter.size() <= 0) {
                return {{ramStartPlace, pointer0}, size, aboutThis};
            }

            //kdyz je to cislo tak musime vratit to cislo o velikosti cislo, protoze cislo pak pod sebou nema dalsi
            if (name == numType) {
                return {{ramStartPlace, pointer0}, size, aboutThis};
            }

            int toAdd = 0;
            bool someRandom = false;
            typee* lookedFor = findParamOnKey(keysAfter[0], toAdd, someRandom);
            keysAfter.erase(keysAfter.begin());
            wasReferenced = someRandom;

            if (someRandom) {
                compilingCode.addInstruction(asmSyntax.movRamReg + " " + asmSyntax.reg1 + std::to_string((toAdd + pointer0)));
                return lookedFor->paramPointer(0, ramStartPlace, wasReferenced, someRandom, compilingCode, keysAfter);
            }
            return lookedFor->paramPointer(toAdd + pointer0, ramStartPlace, wasReferenced, someRandom, compilingCode, keysAfter);
        }

    }

    void calculateSize(int potSize = -1) {
        if (name == numType) {
            size = potSize;
        }

        for (int i = 0; i < params.size(); i++) {
            if (isRef[i]) {
                size += 2;
                continue;
            }
            size += params[i]->size;
        }
    }
};


struct typesHandle {
    std::deque<typee> typeList;

    void createNewType(std::string name, std::vector<typee*> &params, std::vector<std::string> &paramKeys, std::vector<bool> &refers) {
        typeList.push_back({name, params, paramKeys, 0, refers});
        typeList.back().calculateSize();
    }

    typee* findType(std::string key) {
        for (size_t i = 0; i < typeList.size(); i++) {
            if (typeList[i].name == key) {
                return &typeList[i];
            }
        }
        std::cout << "Type doesn't exist: " << key << "\n" << std::endl;
        return nullptr;
    }
};


void copyVar(fullCompiledCode &compilingCode, numberVar varTo, numberVar varFrom) {
    for (int i = 0; i < varTo.sizeB; i++) {
        compilingCode.addInstruction(asmSyntax.movRamRam + " " + std::to_string(varTo.pointer.pointer + i) + std::to_string(varFrom.pointer.pointer + i));
    }
}


//do silne definovaneho varTo se kopiruje to co je v varFrom
//isInReg1 mluvi o varTo
void copy(fullCompiledCode &compilingCode, numberVar varTo, numberVar varFrom, bool isInReg1) {
    if (varTo.pointer.pointer == -1 || varFrom.pointer.pointer == -1) {
        //pointery nejsou definovane => nullptr nebo void
        return;
    }
    //pointer na to moje je ted v registru 1
    if (isInReg1) {
        //to moje je pointer (neboli ta vec co je na adrese (ktera je ulozena v reg1) je pointer)
        if (varTo.isRef) {
            //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
            if (varFrom.isRef) {
                compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + varFrom.pointer.getString());
                return;
            }

            //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak ulozim na sebe jeho pointer
            compilingCode.addInstruction(asmSyntax.setRam16 + asmSyntax.addr0 + varFrom.pointer.getString());
            return;
        }

        //jeho je reference, ale moje ne => musim zkopirovat to na co ukazuje ta jeho reference
        if (varFrom.isRef) {
            compilingCode.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + varFrom.pointer.getString());

            for (int i = 0; i < varTo.sizeB; i++) {
                compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + asmSyntax.addr1);
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
            }

            return;
        }

        //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + " " + varFrom.pointer.getString());
            varFrom.pointer.pointer += 1;
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
        }

        return;
    }

    //jinak neni pointer na ukazatel na varTo ulozen v reg1, podobne pripady

    //to moje je pointer, takze co je v varTo.pointer je pointer na vec v pameti coz je pointer
    if (varTo.isRef) {
        //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
        if (varFrom.isRef) {
            compilingCode.addInstruction(asmSyntax.movRamRam + varTo.pointer.getString() + varFrom.pointer.getString());
            return;
        }

        //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak ulozim na sebe jeho pointer
        compilingCode.addInstruction(asmSyntax.setRam16 + varTo.pointer.getString() + varFrom.pointer.getString());
        return;
    }

    //jeho je reference, ale moje ne => musim zkopirovat to na co ukazuje ta jeho reference
    if (varFrom.isRef) {
        compilingCode.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + varFrom.pointer.getString());

        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + varTo.pointer.getString() + asmSyntax.addr1);
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
            varTo.pointer.pointer += 1;
        }

        return;
    }

    //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
    for (int i = 0; i < varTo.sizeB; i++) {
        compilingCode.addInstruction(asmSyntax.movRamRam + varTo.pointer.getString() + " " + varFrom.pointer.getString());
        varFrom.pointer.pointer += 1;
        varTo.pointer.pointer += 1;
    }
}


//z silne definovaneho varFrom se kopiruje do varTo
//isInReg1 rika neco o varFrom
void callVar(fullCompiledCode &compilingCode, numberVar varTo, numberVar varFrom, bool isInReg1) {
    if (varTo.pointer.pointer == -1 || varFrom.pointer.pointer == -1) {
        //pointery nejsou definovane => nullptr nebo void
        return;
    }
    //pointer na to moje je ted v registru 1
    if (isInReg1) {
        //to moje je pointer (neboli ta vec co je na adrese (ktera je ulozena v reg1) je pointer)
        if (varFrom.isRef) {
            //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
            if (varTo.isRef) {
                compilingCode.addInstruction(asmSyntax.movRamRam + " " + varTo.pointer.getString() + asmSyntax.addr0);
                return;
            }

            //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak musim projit od sebe na skutecnou vec a zkopirovat tamto na tamto misto

            compilingCode.addInstruction(asmSyntax.movRamReg + asmSyntax.reg2 + asmSyntax.addr0);

            for (int i = 0; i < varTo.sizeB; i++) {
                compilingCode.addInstruction(asmSyntax.movRamRam + varTo.pointer.getString() + " " + asmSyntax.addr1);
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
                varTo.pointer.pointer += 1;
            }

            return;
        }

        //kdyz to do ceho se mam kopirovat je reference, tak tam proste hodim do nej sebe
        if (varTo.isRef) {
            compilingCode.addInstruction(asmSyntax.movRegRam + varTo.pointer.getString() + " " + asmSyntax.addr0);
            return;
        }

        //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + varTo.pointer.getString() + " " + asmSyntax.addr0);
            varTo.pointer.pointer += 1;
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
        }

        return;
    }

    //jinak neni pointer na ukazatel na varTo ulozen v reg1, podobne pripady

    if (varFrom.isRef) {
        //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
        if (varTo.isRef) {
            compilingCode.addInstruction(asmSyntax.movRamRam + varTo.pointer.getString() + " " + varFrom.pointer.getString());
            return;
        }

        //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak musim projit od sebe na skutecnou vec a zkopirovat tamto na tamto misto

        compilingCode.addInstruction(asmSyntax.movRamReg + asmSyntax.reg2 + varTo.pointer.getString());

        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + " " + varTo.pointer.getString() + " " + asmSyntax.addr1);
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
            varTo.pointer.pointer += 1;
        }

        return;
    }

    //kdyz to do ceho se mam kopirovat je reference, tak tam proste hodim do nej sebe
    if (varTo.isRef) {
        compilingCode.addInstruction(asmSyntax.movRegRam + varTo.pointer.getString() + " " + varFrom.pointer.getString());
        return;
    }

    //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
    for (int i = 0; i < varTo.sizeB; i++) {
        compilingCode.addInstruction(asmSyntax.movRamRam + varTo.pointer.getString() + " " + varFrom.pointer.getString());
        varTo.pointer.pointer += 1;
        varFrom.pointer.pointer += 1;
    }

    return;
}


//set funkce = nastav to moje podle var
//call funkce = nastav var podle toho meho
//resulty kopirovani:
//  - promena + promena = kopirovani
//  - reference + promena = kopirovani z reference
//  - reference + reference = kopirovani pointeru
struct variable {
    int pointer0;
    std::string name;
    typee* typee;
    std::string ramPart;
    bool isRef = false;

    int getSize() {
        if (isRef) {
            return 2;
        }
        return typee->size;
    }

    //var urcuje misto odkud musime kopirovat
    //verime programatorovi ze var je vytvoren z promene jejiz typ = nasemu po keyed list
    void set(numberVar var, fullCompiledCode &compilingCode, std::vector<std::string> &keyedList) {
        //keyed list je pro NAS!!!, u varu verime ze je spravneho typu
        if (keyedList.size() == 0) {
            copy(compilingCode, {{ramPart, pointer0}, getSize(), isRef}, var, false);
            return;
        }

        bool willBeRef = isRef;
        numberVar result = typee->paramPointer(pointer0, ramPart, willBeRef, false, compilingCode, keyedList);

        copy(compilingCode, result, var, willBeRef);
    }

    //var urcuje misto kam musime kopirovat
    //keyed list je pro nas, a zase verime ze var je to ten stejny typ jako ten z keyed list nasi promenne
    void call(numberVar var, fullCompiledCode &compilingCode, std::vector<std::string> &keyedList) {
        if (keyedList.size() == 0) {
            callVar(compilingCode, var, {{ramPart, pointer0}, getSize(), isRef}, false);
            return;
        }

        bool willBeRef = isRef;
        numberVar result = typee->paramPointer(pointer0, ramPart, willBeRef, false, compilingCode, keyedList);

        callVar(compilingCode, var, result, willBeRef);
    }
};


struct varHandle {
    std::deque<variable> varList;

    void addNewVariable(std::string &name, std::string const &ramPart, typee* typee, int pointer, bool isRef) {
        varList.push_back({pointer, name, typee, ramPart, isRef});
    }

    variable* returnVar(std::string &name) {
        for (size_t i = 0; i < varList.size(); i++) {
            if (varList[i].name == name) {
                return &varList[i];
            }
        }
        std::cout << "Variable doesn't exist: " << name << "\n" << std::endl;
        return nullptr;
    }


    void deleteVar(std::string &name) {
        for (size_t i = 0; i < varList.size(); i++) {
            if (varList[i].name == name) {
                varList.erase(varList.begin() + i);
                return;
            }
        }
        std::cout << "Variable doesn't exist: " << name << "\n" << std::endl;
    }
};


struct timeLineManager {
    std::vector<std::string> varNames = {};

    void destroyLocals(varHandle &handle) {
        for (size_t i = 0; i < varNames.size(); i++) {
            handle.deleteVar(varNames[i]);
        }
    }

    void addVar(std::string const &name) {
        varNames.push_back(name);
    }
};


struct cacheSegment {
    int start;
    int end;
    std::string ramPart;
    int current;

    cacheSegment(int pointer0, int fullSize, std::string ramPart) {
        start = pointer0;
        this->ramPart = ramPart;
        end = pointer0 + fullSize;
        current = start;
    }

    numberVar requestFreeSub(int itsSize) {
        if (current + itsSize > end) {
            std::cout << "Ran out of one line stack" << std::endl;
            return {{ramPart, -1}, -1};
        }
        current += itsSize;
        return {{ramPart, current - itsSize}, itsSize};
    }

    void resetCache() {
        current = start;
    }
};


//code segment je pro promene, cacheSegment pro mezivypocty
struct codeSegmentRam {
    int start;
    int end;

    codeSegmentRam(int pointer0) {
        start = pointer0;
        end = pointer0;
    }

    void expand(int size) {
        end += size;
    }
};


struct bigSegment {
    cacheSegment myCacheSeg = cacheSegment(0, 0, "");
    std::vector<codeSegmentRam> myCodeSeg = {};
    bool isFunq = false;
    int deepness;
    int maxLenght;

    bigSegment(int sizeOfCache, bool isFunq, int pointer0, std::string ramSection) {
        this->isFunq = isFunq;
        deepness = 0;
        myCacheSeg = cacheSegment(pointer0, sizeOfCache, ramSection);
        myCodeSeg.push_back({pointer0 + sizeOfCache});
        deepness += 1;
        maxLenght = myCacheSeg.start;
    }

    void newScope() {
        myCodeSeg.push_back({myCodeSeg.back().end});
        deepness += 1;
    }

    bool endScope() {
        if (myCodeSeg.back().end >= maxLenght) {
            maxLenght = myCodeSeg.back().end;
        }
        myCodeSeg.pop_back();
        deepness -= 1;
        if (deepness <= 0) {
            return true;
        }
        return false;
    }
};


struct basicFunction {
    std::string name;
    bigSegment* myRamSeg;
    std::vector<variable> inputsInner;
    variable output;
    //jump to function start se pouzije na skoceni na radky, kde je kod funkce
    int jumpToFunction;
    numberVar jumpBackToOrigin;
    std::vector<std::string> emptyKeyList = {};

    bool getCalled(fullCompiledCode &compilingCode, std::vector<numberVar> &inputsOuter, numberVar whereTo) {
        if (inputsOuter.size() != inputsInner.size()) {
            std::cout << "Function hasn't got the correct amount of parameters" << std::endl;
            return false;
        }


        for (int i = 0; i < inputsInner.size(); i++) {
            inputsInner[i].set(inputsOuter[i], compilingCode, emptyKeyList);
        }

        compilingCode.addInstruction(asmSyntax.setRam16 + " " + jumpBackToOrigin.pointer.getString() + " " + std::to_string(compilingCode.currentLine + 2));
        compilingCode.addInstruction(asmSyntax.jumpDictate + " " + std::to_string(jumpToFunction));
        output.call(whereTo, compilingCode, emptyKeyList);
        return true;
    }

    bool define(std::string name, std::vector<bool> &isReferences, std::vector<std::string> &varNames, std::vector<typee*> &varTypes,
        varHandle &varsHandle, timeLineManager &timeLine, bigSegment* functions_Segment, typee* outPutType, int codeStartLine) {
        if (varNames.size() != varTypes.size()) {
            std::cout << "Function hasn't got the same amount of names and types" << std::endl;
            return false;
        }

        this->name = name;
        myRamSeg = functions_Segment;
        jumpToFunction = codeStartLine;
        jumpBackToOrigin = {{STATIC_RAM, myRamSeg->myCodeSeg.back().end}, 2, false};
        myRamSeg->myCodeSeg.back().expand(2);

        output = variable(myRamSeg->myCodeSeg.back().end, name, outPutType, STATIC_RAM, false);
        myRamSeg->myCodeSeg.back().expand(outPutType->size);

        for (size_t i = 0; i < varNames.size(); i++) {
            timeLine.addVar(varNames[i]);
            varsHandle.addNewVariable(varNames[i], STATIC_RAM, varTypes[i], myRamSeg->myCodeSeg.back().end, isReferences[i]);
            inputsInner.push_back(varsHandle.varList[varsHandle.varList.size() - 1]);
            if (isReferences[i]) {
                myRamSeg->myCodeSeg.back().expand(2);
                continue;
            }
            myRamSeg->myCodeSeg.back().expand(varTypes[i]->size);
        }

        return true;
    }
};


struct functionHandle {
    std::vector<basicFunction> functionList;

    basicFunction* returnVar(std::string &name) {
        for (size_t i = 0; i < functionList.size(); i++) {
            if (functionList[i].name == name) {
                return &functionList[i];
            }
        }
        std::cout << "Function doesn't exist: " << name << "\n" << std::endl;
        return nullptr;
    }

    void createFunction(std::string name, std::vector<bool> &isReferences, std::vector<std::string> varNames, std::vector<typee*> varTypes,
        varHandle &varsHandle, timeLineManager &timeLine, typee* outPutType, bigSegment* functions_Segment, fullCompiledCode &code) {
        functionList.push_back({});
        //tento jump preskoci funkci pri normalnim hiearchickem zpracovani, aby se mohla psat rovnou do kodu, a stacilo na ni skocit
        //nasledne se instrukce po dokompilovani funkce zmeni na spravny jumpPointer, probehne to nejak takhle:
        //code.changeBackFunq(asmSyntax.jumpDictate + " " + std::to_string(code.currentLine), functionList.back().jumpToFunction - 1);
        code.addInstruction(asmSyntax.jumpDictate + " -1");
        functionList.back().define(name, isReferences, varNames, varTypes, varsHandle, timeLine, functions_Segment, outPutType, code.currentLine);
    }
};


struct memoryManager {
    fullCompiledCode asmCode;
    functionHandle functionsHandle;
    varHandle varsHandle;
    typesHandle typesHandle;
    bigSegment normalSegments = bigSegment(0, false, 0, ""); // dynamicke
    std::deque<bigSegment> staticSegments = {};
    std::vector<timeLineManager> timelinesList;
    bigSegment* currentSegment;

    void constructThing(int cacheSize = 100) {
        normalSegments = bigSegment(cacheSize, false, 0, DYNAMIC_RAM);
        currentSegment = &normalSegments;
    }

    void newFunq(int cacheSize = 60) {
        if (currentSegment->isFunq) {
            std::cout << "Cannot create a function inside a function" << std::endl;
            return;
        }
        staticSegments.push_back(bigSegment(cacheSize, true, staticSegments.back().maxLenght, STATIC_RAM));
        currentSegment = &staticSegments.back();
        timelinesList.push_back({});
    }

    void newScope() {
        timelinesList.push_back({});
        currentSegment->newScope();
    }


    void endCurScope() {
        timelinesList.back().destroyLocals(varsHandle);
        timelinesList.pop_back();
        if (currentSegment->endScope()) {
            if (currentSegment->isFunq) {
                currentSegment = &normalSegments;
                return;
            }
            std::cout << "End of program, ending" << std::endl;
            //dodelat konceni!!!
        }
    }

};


