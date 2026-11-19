#include "astGenerator.cpp"
#include <filesystem>
#include <iostream>
#include <fstream>

namespace fsys = std::filesystem;


struct fullCompiler {
    fsys::path sourceCode;
    fsys::path compileTo;
    compiler myCompiler;

    void load() {
        if (fsys::exists(sourceCode) && !fsys::is_directory(sourceCode)) {
            std::ifstream stream(sourceCode);
            myCompiler.originCode.resize(fsys::file_size(sourceCode));
            stream.read(&myCompiler.originCode[0], fsys::file_size(sourceCode));
        }

        else {
            std::cout << "Source kod neexistuje nebo je to adresar \n" << std::endl;
            throw std::runtime_error("Neexistuje source kod nebo je to adresar \n");
        }
    }

    void run() {
        errorCode oneCode = myCompiler.compile();
        if (oneCode.exists) {
            std::cout << oneCode.errorMes + std::to_string(oneCode.line) + "\n" << std::endl;
            throw std::runtime_error(oneCode.errorMes + std::to_string(oneCode.line) + "\n");
        }
    }

    void saveBack() {
        if (fsys::exists(compileTo) && fsys::is_directory(compileTo)) {
            std::string fileName = sourceCode.stem().string() + ".txt";
            std::ofstream stream(compileTo / fileName);
            for (int i = 0; i < myCompiler.compiledCode.currentLine; i++) {
                stream.write(myCompiler.compiledCode.code[i].data(), myCompiler.compiledCode.code[i].length());
            }
        }
        else {
            std::cout << "Cilova slozka neexistuje nebo to neni slozka \n" << std::endl;
            throw std::runtime_error("Cilova slozka neexistuje nebo to neni slozka \n");
        }
    }
};


int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Spatny zapis pouziti cmd, spravny je: bcompl + jmeno kompilovaneho souboru \n" << std::endl;
        return 1;
    }

    fsys::path consolePath = fsys::current_path();

    fullCompiler myCompiler;
    myCompiler.sourceCode = consolePath / argv[1];
    myCompiler.compileTo = consolePath;
    try {
        myCompiler.load();
        myCompiler.run();
        myCompiler.saveBack();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}