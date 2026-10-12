#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include "./memoryOperations.cpp"

#define numType "number"
#define voidType "void"


struct offset {
    int sizeOfOffset;
    bool exists;
};


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

    //vraci tvz. offset, pokud je offset == -1, jde to do registru 1
    offset loadFromTyper(fullCompiledCode &compilingCode, std::vector<std::string> &keysAfter, bool aboutThis, int pointer0, bool inReg1) {
        if (keysAfter.size() <= 0) {
            if (inReg1) {
                return {pointer0, false};
            }
            return {pointer0, true};
        }
        int curOffset = 0;
        bool keyIsRef = false;

        typee* lookedFor = findParamOnKey(keysAfter[0], curOffset, keyIsRef);

        if (lookedFor == nullptr) {
            return -2;
        }

        if (inReg1) {

        }

        if (keyIsRef) {
            inReg1 = true;
        }

        return lookedFor->loadFromTyper(compilingCode, keysAfter, keyIsRef, pointer0, inReg1);
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