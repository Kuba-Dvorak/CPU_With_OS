cmake_minimum_required(VERSION 4.4)
project(compiler)

set(CMAKE_CXX_STANDARD 20)

add_executable(compiler main.cpp
        textNodeCompiler.cpp
        binaryTree.cpp)
