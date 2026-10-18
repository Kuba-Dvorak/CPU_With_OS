#!/usr/bin/env bash
# Prelozi a spusti AI testy, vygeneruje out/report.html
#   ./run.sh            testy + report
#   ./run.sh --mutace   navic mutacni test (~3 min)
set -u
cd "$(dirname "$0")"
mkdir -p build out

g++ -std=c++20 -O2 -I.. -o build/tests tests.cpp || exit 2
./build/tests out/results.json
status=$?
if (( status > 1 )); then
    echo "testy spadly (kod $status), report se nevytvori; podrobnosti: g++ -fsanitize=address -D_GLIBCXX_ASSERTIONS ..." >&2
    exit 3
fi

if [[ "${1:-}" == "--mutace" ]]; then
    python3 mutate.py
fi

python3 report.py
exit $status
