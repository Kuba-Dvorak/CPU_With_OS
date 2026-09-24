#include <vector>
#include <string>


enum class operators {
    plus = 1,
    remove = 2,
    multiply = 3,
    divade = 4
};


enum class nonAritmoNodes {
    //ve value ulozeno cislo
    functionType = 1,
    //ve value ulozen pointer
    // => nejdriv se musi udelat movRamReg
    variableType = 2,
    //ve value ulozen pointer na jeho oznacenou return pamet
    //=> jump na funkci
    numberType = 3,
    voidType = 4
};


struct basicSyntax {
    std::string setReg = " setReg ";
    std::string movRamReg = " movRamReg ";
    std::string reg1 = " R1 ";
    std::string reg2 = " R2 ";
    std::string reg0 = " R0 ";
    std::string addReg = " addReg ";
    std::string movRegRam = " movRegRam ";
};


basicSyntax myOwnsyntax;


struct nonAritmoNode {
    nonAritmoNodes type;
    int value;

    std::string evaluate(int toWhichToSave) {
        if (type == nonAritmoNodes::numberType) {
            if (toWhichToSave == 1) {
                return myOwnsyntax.setReg + myOwnsyntax.reg1 + std::to_string(value);
            }
            if (toWhichToSave == 2) {
                return myOwnsyntax.setReg + myOwnsyntax.reg2 + std::to_string(value);
            }
        }

        if (type == nonAritmoNodes::variableType) {
            if (toWhichToSave == 1) {
                return myOwnsyntax.movRamReg + std::to_string(value) + myOwnsyntax.reg1;
            }
            if (toWhichToSave == 2) {
                return myOwnsyntax.movRamReg + std::to_string(value) + myOwnsyntax.reg2;
            }
        }

        //funkce udelam pozdeji
        return "";
    }
};


struct aritmeticNode {
    operators nodeOperator;
    aritmeticNode* leftAritmoNode;
    aritmeticNode* rightAritmoNode;
    nonAritmoNode leftNonANode;
    nonAritmoNode rightNonANode;
    int outputPointer;

    std::string createBinary(int toWhichToSave) {
        if (leftNonANode.type == nonAritmoNodes::voidType && rightNonANode.type == nonAritmoNodes::voidType) {
            //u tohodle zanoreni automaticky jako kdyz toWhichToSave = 3

            std::string returnee = leftAritmoNode->createBinary(3) + rightAritmoNode->createBinary(3) +
                    myOwnsyntax.movRamReg + std::to_string(leftAritmoNode->outputPointer) + myOwnsyntax.reg1 +
                    myOwnsyntax.movRamReg + std::to_string(rightAritmoNode->outputPointer) + myOwnsyntax.reg2;


            if (nodeOperator == operators::plus) {
                returnee += myOwnsyntax.addReg;
            }
            //dodelat dalsi operatory (pozdeji)

            returnee += myOwnsyntax.reg1 + myOwnsyntax.reg2 +
                    myOwnsyntax.movRegRam + myOwnsyntax.reg0 + std::to_string(outputPointer);
            return returnee;
        }

        if (!(leftNonANode.type == nonAritmoNodes::voidType && rightNonANode.type == nonAritmoNodes::voidType)) {

        }


        return "";
    }

    //to which to save to urcuje kam se da vysledek daneho stromu v registrech v CPU => 1 = R1, 2 = R2, 3 = outputPointer (musi do RAM)
};


aritmeticNode operateNotatation(std::string &oneString, int &pos) {

}