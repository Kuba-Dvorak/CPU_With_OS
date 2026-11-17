# AI-tests

Testy psané AI. Kód kompilátoru nemění, jen ho volají přes `adapter.hpp`.

```
./run.sh            # přeloží a spustí testy, vytvoří out/report.html
./run.sh --lexer    # s tvým lexicalAnalyzer.cpp místo náhradního (až bude hotový)
./run.sh --mutace   # navíc mutační test (~15 min, 271 mutantů): ověří, že testy poznají smazání řádku
```

| soubor | co dělá |
|---|---|
| `sim.hpp` | referenční simulátor ISA (registry R0–R7, signReg, latche LACI/LACO/LC/LSkip/LShiftLow, skoky); předpoklady o CPU jsou v hlavičce |
| `mock_lexer.cpp` | náhradní `lexicalAnalyzer.cpp`, dokud je prázdný; `run.sh` ho podstrčí do kopie stromu v `build/stage/`, tvoje soubory se nemění |
| `adapter.hpp` | jediné místo, kde se testy dotýkají kódu kompilátoru (při přejmenování uprav jen tady) |
| `suite_copy.hpp` | `copyFromTo` (4 druhy × umístění zásobník/R1/R2 × SP × velikost + fuzz) a `setRamLocal` |
| `suite_paths.hpp` | `variable::call` / `set` s cestami přes struktury a reference (i v pointingMode), 6000 náhodných cest |
| `suite_calls.hpp` | celé volání funkce přes zásobník (přes `basicNode::evaluateFn` a text `call f /_ ... _/`), vnořená volání, různé počáteční SP |
| `suite_ops.hpp` | `evaluateOperator` + `syntaxOperations`: 13 operátorů × velikosti 1–4 B, okraje rozsahu + náhodná čísla |
| `suite_expr.hpp` | výrazy přes `parseBT`: tvar stromu (priorita, asociativita) a hodnota; všech 144 dvojic operátorů + náhodné výrazy s proměnnými, závorkami, voláním a `!` |
| `tests.cpp` | spustí všechny sady, výsledky do `out/results.json` |
| `mutate.py` | mutační test nad `memoryManagment/*.cpp` |
| `report.py` | z `out/*.json` vyrobí `out/report.html` |

Adaptér (`adapter.hpp`) teď includuje `syntaxAnalyzer.cpp`, takže se testuje celý řetěz od textu výrazu po instrukce.

Návratový kód `run.sh`: 0 = vše prošlo, 1 = něco selhalo, 2 = nejde přeložit, 3 = testy spadly (report se nevytvoří).
