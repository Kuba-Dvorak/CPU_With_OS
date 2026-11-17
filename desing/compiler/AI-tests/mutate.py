#!/usr/bin/env python3
"""Mutacni test: overuje, ze testy opravdu neco chytaji.

Ze souboru kompilatoru postupne vyhodi jeden radek (instrukci, posun offsetu, return, ...),
prelozi testy proti teto rozbite kopii a porovna vysledky s puvodnim kodem.
  zabity mutant  = aspon jeden test dopadl jinak (testy zmenu poznaly)
  prezivsi       = nikdo nic nepoznal -> mezera v testech, nebo ten radek nic nedela
Porovnava se s vysledky puvodniho kodu, takze to funguje i kdyz uz nektere testy selhavaji.
Kod kompilatoru se nemeni, mutanti vznikaji jen v out/mutants/.
"""
import json
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
OUT = os.path.join(HERE, "out", "mutants")
LEXER = "--lexer" in sys.argv  # s tvym lexerem misto nahradniho
FILES = ["memoryManagment/memoryOperations.cpp", "memoryManagment/variables.cpp", "memoryManagment/scopes.cpp",
         "memoryManagment/functions.cpp", "syntaxOperations.cpp", "syntaxAnalyzer.cpp"]

MUTABLE = [
    (re.compile(r"\baddInstruction\s*\("), "smazaná instrukce"),
    (re.compile(r"[\w\]\)]\s*(\+|-)=\s*"), "smazaný posun / přičtení"),
    (re.compile(r"^\s*pass\.\w+\s*=[^=]"), "smazaná změna stavu cesty"),
    (re.compile(r"\.(pop_back|erase|reset|push_back)\s*\("), "smazaná operace se seznamem"),
    (re.compile(r"^\s*return\s*(true|false)?\s*;"), "smazaný return (propadne dál)"),
]


def signature(path):
    """Co presne testy nahlasily - na tom se porovnava mutant s originalem."""
    with open(path) as f:
        d = json.load(f)
    sig = []
    for suite in ("copy", "calls", "scopes"):
        sig += [(suite, r["group"], r["row"], r["col"], r["status"], r["reason"]) for r in d[suite]]
    sig += [("ops", c["row"], c["col"], c["pass"], c["fail"]) for c in d["ops"]["cells"]]
    sig += [("expr-pair", c["row"], c["col"], c["pass"], c["fail"]) for c in d["expr"]["pairs"]]
    sig += [("expr", c["row"], c["col"], c["pass"], c["fail"]) for c in d["expr"]["cells"]]
    sig += [("copyFuzz", f["row"], f["pass"], f["fail"]) for f in d["copyFuzz"]]
    sig += [("paths", c["shape"], c["col"], c["pass"], c["fail"]) for c in d["paths"]["cells"]]
    return sig


def build_and_run(root_dir, exe, result):
    c = subprocess.run(["g++", "-std=c++20", "-O1", f"-I{root_dir}", "-o", exe, os.path.join(HERE, "tests.cpp")],
                       capture_output=True, text=True)
    if c.returncode != 0:
        return "nekompiluje"
    try:
        subprocess.run([exe, result], capture_output=True, text=True, timeout=120)
    except subprocess.TimeoutExpired:
        return "spadl"
    return signature(result) if os.path.exists(result) else "spadl"


def prepare_tree(dst):
    # stejne jako run.sh: kopie stromu + nahradni lexer
    os.makedirs(os.path.join(dst, "memoryManagment"), exist_ok=True)
    for f in os.listdir(ROOT):
        if f.endswith(".cpp"):
            shutil.copy(os.path.join(ROOT, f), dst)
    for f in os.listdir(os.path.join(ROOT, "memoryManagment")):
        shutil.copy(os.path.join(ROOT, "memoryManagment", f), os.path.join(dst, "memoryManagment"))
    if not LEXER:
        shutil.copy(os.path.join(HERE, "mock_lexer.cpp"), os.path.join(dst, "lexicalAnalyzer.cpp"))


def run_mutant(idx, fname, lines, line_no, kind, base_sig):
    mdir = os.path.join(OUT, f"m{idx:03d}")
    prepare_tree(mdir)
    mutated = lines[:line_no] + ["// MUTANT: radek smazan\n"] + lines[line_no + 1:]
    with open(os.path.join(mdir, fname), "w") as f:
        f.writelines(mutated)
    sig = build_and_run(mdir, os.path.join(mdir, "tests"), os.path.join(mdir, "result.json"))
    info = {"file": fname, "line": line_no + 1, "code": lines[line_no].strip(), "kind": kind}
    if sig == "nekompiluje":
        return {**info, "result": "nekompiluje"}
    if sig == "spadl":
        # testy spadly nebo se zacyklily = zmenu urcite poznaly
        return {**info, "result": "zabit", "crash": True}
    changed = sum(a != b for a, b in zip(sig, base_sig)) + abs(len(sig) - len(base_sig))
    return {**info, "result": "zabit" if changed else "prezil", "changed": changed}


def main():
    shutil.rmtree(OUT, ignore_errors=True)
    base = os.path.join(OUT, "base")
    prepare_tree(base)
    base_sig = build_and_run(base, os.path.join(base, "tests"), os.path.join(base, "result.json"))
    if not isinstance(base_sig, list):
        print(f"mutace: puvodni kod {'nejde prelozit' if base_sig == 'nekompiluje' else 'pada'}, neni s cim porovnavat", file=sys.stderr)
        sys.exit(2)

    targets = []
    for fname in FILES:
        with open(os.path.join(ROOT, fname)) as f:
            lines = f.readlines()
        for i, ln in enumerate(lines):
            s = ln.strip()
            if s.startswith("//") or s.startswith("#"):
                continue
            for rx, kind in MUTABLE:
                if rx.search(ln):
                    targets.append((fname, lines, i, kind))
                    break

    jobs = os.cpu_count() or 4
    print(f"mutace: {len(targets)} mutantu, {jobs} paralelne...", file=sys.stderr)
    with ThreadPoolExecutor(jobs) as ex:
        res = list(ex.map(lambda a: run_mutant(a[0], *a[1], base_sig), enumerate(targets)))
    killed = sum(r["result"] == "zabit" for r in res)
    alive = sum(r["result"] == "prezil" for r in res)
    print(f"mutace: zabito {killed}, prezilo {alive}, nekompiluje {len(res) - killed - alive}", file=sys.stderr)
    with open(os.path.join(HERE, "out", "mutants.json"), "w") as f:
        json.dump({"files": FILES, "mutants": res}, f, ensure_ascii=False)


if __name__ == "__main__":
    main()
