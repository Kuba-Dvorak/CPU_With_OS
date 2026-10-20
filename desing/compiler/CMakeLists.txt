cmake_minimum_required(VERSION 4.4)
project(compiler)

set(CMAKE_CXX_STANDARD 20)

add_executable(compiler
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
)
