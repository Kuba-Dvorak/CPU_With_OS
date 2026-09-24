#include <iostream>
#include <memory>
#include <string>
#include <vector>


struct customType;


struct storedUnit {
    std::string key;
    int pointer;
    customType* type;
};


struct functionUnit {
    std::string name;
    std::vector<customType*> parameters;
    std::vector<std::string> paramNames;
    customType* returnVar;
    int pointer0;
    std::string functionCode;
    int codeLocation;
};


void compile(functionUnit &unit) {

}


struct functionStorage {
    std::vector<std::unique_ptr<functionUnit>> functions;

    void createNewFunction(std::string name, std::vector<customType*> params, std::vector<std::string> names, customType* returnType, std::string uncompiledCode) {
        functions.push_back(std::unique_ptr<functionUnit>(new functionUnit{name, params, names, returnType,
            0, uncompiledCode, 0}));
        compile(*functions.back());
    }

    functionUnit* findFunction(std::string key) {
        for (int i = 0; i < functions.size(); i++) {
            if (functions[i]->name == key) {
                return functions[i].get();
            }
        }
        std::cout << "Type doesn't exist: " << key << "\n" << std::endl;
        return nullptr;
    }
};


struct customType {
    std::string name;
    int sizeB;
    std::vector<storedUnit> parameters;
    std::vector<functionUnit> metods;
};


struct typesStorage {
    std::vector<std::unique_ptr<customType>> types;

    void createNewType(std::string name, int sizeB, std::vector<storedUnit> params) {
        types.push_back(std::unique_ptr<customType>(new customType{name, sizeB, params}));
    }

    customType* findType(std::string key) {
        for (size_t i = 0; i < types.size(); i++) {
            if (types[i]->name == key) {
                return types[i].get();
            }
        }
        std::cout << "Type doesn't exist: " << key << "\n" << std::endl;
        return nullptr;
    }
};


struct storage {
    std::vector<std::unique_ptr<storedUnit>> units;
    int lastPointerValue = 0;

    storage() {
        lastPointerValue = 0;
    }

    void addNewVariable(std::string name, std::string type, std::string deep, typesStorage &types) {
        customType* itstype = types.findType(type);
        if (itstype == nullptr) return;
        units.push_back(std::unique_ptr<storedUnit>(new storedUnit{name + deep, lastPointerValue, itstype}));
        lastPointerValue += itstype->sizeB;
    }

    storedUnit* returnVar(std::string key) {
        for (size_t i = 0; i < units.size(); i++) {
            if (units[i]->key == key) {
                return units[i].get();
            }
        }
        std::cout << "Variable doesn't exist: " << key << "\n" << std::endl;
        return nullptr;
    }

};


