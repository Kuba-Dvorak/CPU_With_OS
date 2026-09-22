#include <iostream>
#include <string>
#include <vector>


struct customType;


struct storedUnit {
    std::string key;
    int pointer;
    customType* type;
};


struct customType {
    std::string name;
    int sizeB;
    std::vector<storedUnit> parameters;

    int sizet() {
        return parameters.size();
    }
};


struct typesStorage {
    std::vector<customType> types;

    void createNewType(std::string name, int sizeB, std::vector<storedUnit> params) {
        types.push_back({name, sizeB, params});
    }
};


enum class NodeTypes {
    number = 0,
    nameNode = 1,
    binaryTree = 2,
    basicType = 3,
    ownType = 4,
    ifStatement = 5,
    elseStatement = 6,
    whileCycle = 7,
    forCycle = 8,
    continueStatement = 9,
    breakStatement = 10,
    returnStatement = 11,

    typeNode = 12,
    structNode = 13,
    functionNode = 14
};


struct storage {
    std::vector<storedUnit> units;
    int lastPointerValue;

    storage() {
        lastPointerValue = 0;
    }

    void addNewVariable(std::string name, std::string type) {

    }
};
