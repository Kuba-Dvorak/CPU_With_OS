#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <cstdint>
#include <deque>
#include <memory>
#include <string_view>
#include "./scopes.cpp"



struct basicFunction {
    std::string name;
    segmentManager mySegmentManager = {};
    std::vector<variable> inputsInner;
    variable output;
    std::vector<std::string> emptyKeyList = {};


};
