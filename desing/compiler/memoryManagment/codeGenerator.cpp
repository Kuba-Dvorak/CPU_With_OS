#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>


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