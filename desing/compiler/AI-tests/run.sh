#!/usr/bin/env bash
# Prelozi a spusti AI testy, vygeneruje out/report.html
#   ./run.sh            testy + report (s nahradnim lexerem mock_lexer.cpp)
#   ./run.sh --lexer    s tvym lexicalAnalyzer.cpp misto nahradniho
#   ./run.sh --mutace   navic mutacni test (~5 min)
set -u
cd "$(dirname "$0")"
mkdir -p build out

# kopie stromu kompilatoru; tvoje soubory se nemeni
rm -rf build/stage && mkdir -p build/stage
cp ../*.cpp build/stage/ && cp -r ../memoryManagment build/stage/
[[ " $* " == *" --lexer "* ]] || cp mock_lexer.cpp build/stage/lexicalAnalyzer.cpp

g++ -std=c++20 -O2 -Ibuild/stage -o build/tests tests.cpp || exit 2
./build/tests out/results.json
status=$?
if (( status > 1 )); then
    echo "testy spadly (kod $status), report se nevytvori; podrobnosti: g++ -fsanitize=address -D_GLIBCXX_ASSERTIONS ..." >&2
    exit 3
fi

if [[ " $* " == *" --mutace "* ]]; then
    python3 mutate.py $([[ " $* " == *" --lexer "* ]] && echo --lexer)
fi

python3 report.py
exit $status
