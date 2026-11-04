// Vsechny sady AI testu v jednom programu (jedna kompilace = rychlejsi mutacni test).
// Vystup: JSON do souboru z argv[1]; navratovy kod 1, pokud neco selhalo.
#include <fstream>
#include <iostream>
#include "suite_calls.hpp"
#include "suite_copy.hpp"
#include "suite_paths.hpp"
#include "suite_scopes.hpp"

int main(int argc, char **argv) {
    std::string copyFuzz;
    std::vector<Result> copy = suite_copy::runAll(copyFuzz);
    std::string paths = suite_paths::runAll();
    std::vector<Result> calls = suite_calls::runAll();
    std::vector<Result> scopes = suite_scopes::runAll();

    auto count = [](const std::vector<Result> &v, const char *st) {
        int n = 0;
        for (auto &r : v) n += r.status == st;
        return n;
    };
    int pathFails = 0;
    for (size_t i = 0; (i = paths.find("\"fail\":", i)) != std::string::npos; i += 7) pathFails += std::atoi(paths.c_str() + i + 7);
    int copyFuzzFails = 0;
    for (size_t i = 0; (i = copyFuzz.find(",\"fail\":", i)) != std::string::npos; i += 8) copyFuzzFails += std::atoi(copyFuzz.c_str() + i + 8);

    std::string o = "{\"copy\":" + arr(copy, resultJson) + ",\"copyFuzz\":" + copyFuzz + ",\"paths\":" + paths +
                    ",\"calls\":" + arr(calls, resultJson) + ",\"scopes\":" + arr(scopes, resultJson) + "}";
    if (argc > 1) std::ofstream(argv[1]) << o;
    else std::cout << o;

    int fails = count(copy, "fail") + count(calls, "fail") + count(scopes, "fail") + pathFails + copyFuzzFails;
    std::cerr << "copy: " << copy.size() << " testů, " << count(copy, "fail") << " selhalo, fuzz " << copyFuzzFails << " selhalo\n"
              << "paths: " << pathFails << " selhalo\n"
              << "calls: " << calls.size() << " testů, " << count(calls, "fail") << " selhalo\n"
              << "scopes: " << scopes.size() << " kontrol, " << count(scopes, "fail") << " selhalo, " << count(scopes, "warn") << " varování\n";
    return fails ? 1 : 0;
}
