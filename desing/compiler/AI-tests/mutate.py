#!/usr/bin/env python3
"""Mutacni test: overuje, ze testy opravdu neco chytaji.

Z memoryOperations.cpp postupne vyhodi jeden radek (instrukci, posun pointeru, return...),
prelozi testy proti teto rozbite kopii a podiva se, jestli aspon jeden test selze.
  zabity mutant  = testy chybu odhalily (dobre)
  prezivsi       = chybu nikdo nepoznal -> bud mezera v testech, nebo ten radek nic nedela
Kod kompilatoru se nemeni, mutanti vznikaji jen v out/mutants/.
"""
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.normpath(os.path.join(HERE, "..", "memoryManagment", "memoryOperations.cpp"))
OUT = os.path.join(HERE, "out", "mutants")

# radky, jejichz smazani je smysluplna mutace
MUTABLE = [
    (re.compile(r"\baddInstruction\s*\("), "smazana instrukce"),
    (re.compile(r"\.pointer\s*\+=\s*1"), "smazany posun adresy"),
    (re.compile(r"^\s*return\s*;"), "smazany return (propadne do dalsi vetve)"),
]


def absolutize_includes(text, src_dir):
    def repl(m):
        path = os.path.normpath(os.path.join(src_dir, m.group(1)))
        return f'#include "{path}"'
    return re.sub(r'#include\s+"([^"]+)"', repl, text)


def run_mutant(idx, lines, line_no, kind):
    mdir = os.path.join(OUT, f"m{idx:03d}")
    os.makedirs(mdir, exist_ok=True)
    mutated = lines[:line_no] + ["// MUTANT: radek smazan\n"] + lines[line_no + 1:]
    src = absolutize_includes("".join(mutated), os.path.dirname(SRC))
    mpath = os.path.join(mdir, "memoryOperations.cpp")
    with open(mpath, "w") as f:
        f.write(src)
    exe = os.path.join(mdir, "test_copy")
    c = subprocess.run(
        ["g++", "-std=c++20", "-O1", f'-DCOPY_IMPL="{mpath}"', "-o", exe, os.path.join(HERE, "test_copy.cpp")],
        capture_output=True, text=True)
    base = {"line": line_no + 1, "code": lines[line_no].strip(), "kind": kind}
    if c.returncode != 0:
        return {**base, "result": "nekompiluje"}
    r = subprocess.run([exe, os.path.join(mdir, "result.json")], capture_output=True, text=True)
    summary = r.stderr.strip()
    return {**base, "result": "zabit" if r.returncode != 0 else "prezil", "summary": summary}


def main():
    with open(SRC) as f:
        lines = f.readlines()
    targets = []
    for i, ln in enumerate(lines):
        if ln.strip().startswith("//"):
            continue
        for rx, kind in MUTABLE:
            if rx.search(ln):
                targets.append((i, kind))
                break
    jobs = os.cpu_count() or 4
    print(f"mutace: {len(targets)} mutantu, {jobs} paralelne...", file=sys.stderr)
    with ThreadPoolExecutor(jobs) as ex:
        res = list(ex.map(lambda a: run_mutant(a[0], lines, a[1][0], a[1][1]), enumerate(targets)))
    killed = sum(r["result"] == "zabit" for r in res)
    print(f"mutace: zabito {killed}/{len(res)}", file=sys.stderr)
    with open(os.path.join(HERE, "out", "mutants.json"), "w") as f:
        json.dump({"source": os.path.relpath(SRC, HERE), "mutants": res}, f, ensure_ascii=False)


if __name__ == "__main__":
    main()
