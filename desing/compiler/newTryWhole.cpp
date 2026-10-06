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


struct fullCompiledCode {
    std::string code = "";
    int currentLine = 0;

    void addInstruction(std::string instrText) {
        code += instrText;
        currentLine += 1;
    }
};


struct numberVar {
    int pointer;
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
    numberVar paramPointer(int pointer0, bool &wasReferenced, bool aboutThis, fullCompiledCode &compilingCode, std::vector<std::string> &keysAfter) {
        if (wasReferenced) {
            //kdyz jsme ukoncili radu, tak musime tohle vratit
            if (keysAfter.size() <= 0) {
                return {0, size, aboutThis};
            }

            //kdyz je to cislo tak musime vratit to cislo o velikosti cislo, protoze cislo pak pod sebou nema dalsi
            if (name == numType) {
                return {0, size, aboutThis};
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

            return lookedFor->paramPointer(0, wasReferenced, someRandom, compilingCode, keysAfter);
        }

        else {
            //kdyz jsme ukoncili radu, tak musime tohle vratit
            if (keysAfter.size() <= 0) {
                return {pointer0, size, aboutThis};
            }

            //kdyz je to cislo tak musime vratit to cislo o velikosti cislo, protoze cislo pak pod sebou nema dalsi
            if (name == numType) {
                return {pointer0, size, aboutThis};
            }

            int toAdd = 0;
            bool someRandom = false;
            typee* lookedFor = findParamOnKey(keysAfter[0], toAdd, someRandom);
            keysAfter.erase(keysAfter.begin());
            wasReferenced = someRandom;

            if (someRandom) {
                compilingCode.addInstruction(asmSyntax.movRamReg + " " + asmSyntax.reg1 + std::to_string((toAdd + pointer0)));
                return lookedFor->paramPointer(0, wasReferenced, someRandom, compilingCode, keysAfter);
            }
            return lookedFor->paramPointer(toAdd + pointer0, wasReferenced, someRandom, compilingCode, keysAfter);
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
        compilingCode.addInstruction(asmSyntax.movRamRam + " " + std::to_string(varTo.pointer + i) + std::to_string(varFrom.pointer + i));
    }
}


//do silne definovaneho varTo se kopiruje to co je v varFrom
//isInReg1 mluvi o varTo
void copy(fullCompiledCode &compilingCode, numberVar varTo, numberVar varFrom, bool isInReg1) {
    //pointer na to moje je ted v registru 1
    if (isInReg1) {
        //to moje je pointer (neboli ta vec co je na adrese (ktera je ulozena v reg1) je pointer)
        if (varTo.isRef) {
            //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
            if (varFrom.isRef) {
                compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + std::to_string(varFrom.pointer));
                return;
            }

            //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak ulozim na sebe jeho pointer
            compilingCode.addInstruction(asmSyntax.setRam16 + asmSyntax.addr0 + std::to_string(varFrom.pointer));
            return;
        }

        //jeho je reference, ale moje ne => musim zkopirovat to na co ukazuje ta jeho reference
        if (varFrom.isRef) {
            compilingCode.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + std::to_string(varFrom.pointer));

            for (int i = 0; i < varTo.sizeB; i++) {
                compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + asmSyntax.addr1);
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
            }

            return;
        }

        //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + asmSyntax.addr0 + " " + std::to_string(varFrom.pointer));
            varFrom.pointer += 1;
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
        }

        return;
    }

    //jinak neni pointer na ukazatel na varTo ulozen v reg1, podobne pripady

    //to moje je pointer, takze co je v varTo.pointer je pointer na vec v pameti coz je pointer
    if (varTo.isRef) {
        //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
        if (varFrom.isRef) {
            compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(varTo.pointer) + std::to_string(varFrom.pointer));
            return;
        }

        //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak ulozim na sebe jeho pointer
        compilingCode.addInstruction(asmSyntax.setRam16 + std::to_string(varTo.pointer) + std::to_string(varFrom.pointer));
        return;
    }

    //jeho je reference, ale moje ne => musim zkopirovat to na co ukazuje ta jeho reference
    if (varFrom.isRef) {
        compilingCode.addInstruction(asmSyntax.movRegRam + asmSyntax.reg2 + std::to_string(varFrom.pointer));

        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(varTo.pointer) + asmSyntax.addr1);
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
            varTo.pointer += 1;
        }

        return;
    }

    //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
    for (int i = 0; i < varTo.sizeB; i++) {
        compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(varTo.pointer) + " " + std::to_string(varFrom.pointer));
        varFrom.pointer += 1;
        varTo.pointer += 1;
    }
}


//z silne definovaneho varFrom se kopiruje do varTo
//isInReg1 rika neco o varFrom
void callVar(fullCompiledCode &compilingCode, numberVar varTo, numberVar varFrom, bool isInReg1) {
    //pointer na to moje je ted v registru 1
    if (isInReg1) {
        //to moje je pointer (neboli ta vec co je na adrese (ktera je ulozena v reg1) je pointer)
        if (varFrom.isRef) {
            //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
            if (varTo.isRef) {
                compilingCode.addInstruction(asmSyntax.movRamRam + " " + std::to_string(varTo.pointer) + asmSyntax.addr0);
                return;
            }

            //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak musim projit od sebe na skutecnou vec a zkopirovat tamto na tamto misto

            compilingCode.addInstruction(asmSyntax.movRamReg + asmSyntax.reg2 + asmSyntax.addr0);

            for (int i = 0; i < varTo.sizeB; i++) {
                compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(varTo.pointer) + " " + asmSyntax.addr1);
                compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
                varTo.pointer += 1;
            }

            return;
        }

        //kdyz to do ceho se mam kopirovat je reference, tak tam proste hodim do nej sebe
        if (varTo.isRef) {
            compilingCode.addInstruction(asmSyntax.movRegRam + std::to_string(varTo.pointer) + " " + asmSyntax.addr0);
            return;
        }

        //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(varTo.pointer) + " " + asmSyntax.addr0);
            varTo.pointer += 1;
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg1);
        }

        return;
    }

    //jinak neni pointer na ukazatel na varTo ulozen v reg1, podobne pripady

    if (varFrom.isRef) {
        //tamto je taky reference (misto kam ukazuje varFrom.pointer je nejake misto v pameti a jeho obsah je pointer na promenou)
        if (varTo.isRef) {
            compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(varTo.pointer) + " " + std::to_string(varFrom.pointer));
            return;
        }

        //kdyz je to moje reference ale to co mam zkopirovat tak neni, tak musim projit od sebe na skutecnou vec a zkopirovat tamto na tamto misto

        compilingCode.addInstruction(asmSyntax.movRamReg + asmSyntax.reg2 + std::to_string(varTo.pointer));

        for (int i = 0; i < varTo.sizeB; i++) {
            compilingCode.addInstruction(asmSyntax.movRamRam + " " + std::to_string(varTo.pointer) + " " + asmSyntax.addr1);
            compilingCode.addInstruction(asmSyntax.incrementReg + asmSyntax.reg2);
            varTo.pointer += 1;
        }

        return;
    }

    //kdyz to do ceho se mam kopirovat je reference, tak tam proste hodim do nej sebe
    if (varTo.isRef) {
        compilingCode.addInstruction(asmSyntax.movRegRam + std::to_string(varTo.pointer) + " " + std::to_string(varFrom.pointer));
        return;
    }

    //to moje neni reference a to jeho taky neni reference => musime cele kopirovat
    for (int i = 0; i < varTo.sizeB; i++) {
        compilingCode.addInstruction(asmSyntax.movRamRam + std::to_string(varTo.pointer) + " " + std::to_string(varFrom.pointer));
        varTo.pointer += 1;
        varFrom.pointer += 1;
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
    bool isRef = false;

    //var urcuje misto odkud musime kopirovat
    //verime programatorovi ze var je vytvoren z promene jejiz typ = nasemu po keyed list
    void set(numberVar var, fullCompiledCode &compilingCode, std::vector<std::string> &keyedList) {
        //keyed list je pro NAS!!!, u varu verime ze je spravneho typu
        if (keyedList.size() == 0) {
            copy(compilingCode, {pointer0, typee->size, isRef}, var, false);
            return;
        }

        bool willBeRef = isRef;
        numberVar result = typee->paramPointer(pointer0, willBeRef, false, compilingCode, keyedList);

        copy(compilingCode, result, var, willBeRef);
    }

    //var urcuje misto kam musime kopirovat
    //keyed list je pro nas, a zase verime ze var je to ten stejny typ jako ten z keyed list nasi promenne
    void call(numberVar var, fullCompiledCode &compilingCode, std::vector<std::string> &keyedList) {
        if (keyedList.size() == 0) {
            callVar(compilingCode, var, {pointer0, typee->size, isRef}, false);
            return;
        }

        bool willBeRef = isRef;
        numberVar result = typee->paramPointer(pointer0, willBeRef, false, compilingCode, keyedList);

        callVar(compilingCode, var, result, willBeRef);
    }
};


struct varHandle {
    std::deque<variable> varList;

    void addNewVariable(std::string &name, typee* typee, int pointer, bool isRef) {
        varList.push_back({pointer, name, typee});
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
    int current;

    cacheSegment(int pointer0, int fullSize) {
        start = pointer0;
        end = pointer0 + fullSize;
        current = start;
    }

    numberVar requestFreeSub(int itsSize) {
        if (current + itsSize > end) {
            std::cout << "Ran out of one line stack" << std::endl;
            return {-1, -1};
        }
        current += itsSize;
        return {current - itsSize, itsSize};
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


struct functionSegment {
    cacheSegment myCacheSeg;
    codeSegmentRam myCodeSeg;
};


struct basicFunction {
    std::string name;
    functionSegment* myRamSeg;
    std::vector<variable> inputsInner;
    variable output;
    int jumpBackToOrigin;
    int jumpToFunction;

    bool getCalled(fullCompiledCode &compilingCode, std::vector<numberVar> &inputsOuter) {
        if (inputsOuter.size() != inputsInner.size()) {
            std::cout << "Function hasn't got the correct amount of parameters" << std::endl;
            return false;
        }

        for (int i = 0; i < inputsInner.size(); i++) {
            if (inputsInner[i].reference) {
                compilingCode.addInstruction(asmSyntax.setRam16 + " " + std::to_string(inputsInner[i].pointer0) + " " + std::to_string(inputsOuter[i].pointer));
                continue;
            }

            copyVar(compilingCode, {inputsInner[i].pointer0, inputsInner[i].typee->size}, inputsOuter[i]);
        }

        compilingCode.addInstruction(asmSyntax.setRam16 + " " + std::to_string(jumpBackToOrigin) + " " + std::to_string(compilingCode.currentLine + 2));
        compilingCode.addInstruction(asmSyntax.jumpDictate + " " + std::to_string(jumpToFunction));
        return true;
    }

    void define(std::string name, std::vector<bool> &isReferences, std::vector<std::string> varNames, std::vector<typee*> varTypes, typee* outputType,
        int outputPointer, varHandle &varsHandle, timeLineManager &timeLine, functionSegment* hisPart) {
        if (varNames.size() != varTypes.size()) {
            std::cout << "Function hasn't got the same amount of names and types" << std::endl;
            return;
        }

        this->name = name;

        output = variable(outputPointer++, name, outputType, false);

        for (size_t i = 0; i < varNames.size(); i++) {
            timeLine.addVar(varNames[i]);
            varsHandle.addNewVariable(varNames[i], varTypes[i], outputPointer++, isReferences[i]);
            inputsInner.push_back(varsHandle.varList[varsHandle.varList.size() - 1]);
        }

        myRamSeg = hisPart;
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
};


struct memoryManager {
    functionHandle functionsHandle;
    varHandle varsHandle;
    typesHandle typesHandle;
    cacheSegment cacheCurrent;
    std::vector<codeSegmentRam> normalSegments; // dynamicke
    std::vector<functionSegment> staticSegments;
    std::vector<timeLineManager> timelinesList;
    bool lastWasFunction = false;

    void createSubCode(bool isFunction, int wantedLenghtFunq = 50) {
        lastWasFunction = isFunction;
        if (lastWasFunction) {
            //nejdriv cache o nejake definovane velikosti, potom codeSegment, ktery muze rust za doby existence funkce
            staticSegments.push_back({cacheSegment(staticSegments.back().myCodeSeg.end, wantedLenghtFunq),
                codeSegmentRam(staticSegments.back().myCodeSeg.end + wantedLenghtFunq)});
        }
        else {
            normalSegments.push_back({normalSegments.back().end});
        }
        timelinesList.push_back({});
    }

    void destroyCurrentTimeLine() {
        timelinesList.back().destroyLocals(varsHandle);
        if (!lastWasFunction) {
            normalSegments.pop_back();
        }
        else {
            lastWasFunction = false;
        }
        timelinesList.pop_back();
    }



};


