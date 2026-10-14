#!/usr/bin/env bash
# Prelozi a spusti AI testy, vygeneruje out/report.html
#   ./run.sh            testy + report
#   ./run.sh --mutace   navic mutacni test (~30 s)
set -u
cd "$(dirname "$0")"
mkdir -p build out

g++ -std=c++20 -O2 -o build/test_copy test_copy.cpp || exit 2
./build/test_copy out/copy.json
status=$?

if [[ "${1:-}" == "--mutace" ]]; then
    python3 mutate.py
fi

python3 report.py
exit $status
