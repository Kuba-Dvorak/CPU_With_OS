cmake_minimum_required(VERSION 4.4)
project(compiler)

set(CMAKE_CXX_STANDARD 20)

# AI: soubory se navzajem includuji retezcem (main.cpp -> astGenerator.cpp -> ...),
# takze se preklada jen main.cpp, ostatni jsou v projektu jen aby je videlo IDE
set(includedSources
        syntax.cpp
        memoryManagment/memoryOperations.cpp
        memoryManagment/codeGenerator.cpp
        memoryManagment/scopes.cpp
        memoryManagment/variables.cpp
        memoryManagment/functions.cpp
        astGenerator.cpp
        lexicalSyntax.cpp
        lexicalAnalyzer.cpp
        syntaxAnalyzer.cpp
        syntaxOperations.cpp
        typeConstructor.cpp
)
set_source_files_properties(${includedSources} PROPERTIES HEADER_FILE_ONLY ON)

add_executable(compiler main.cpp ${includedSources})
