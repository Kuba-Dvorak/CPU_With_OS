# AI-tests

Testy psané AI. Kód kompilátoru nemění, jen ho volají přes `adapter.hpp`.

```
./run.sh            # přeloží a spustí testy, vytvoří out/report.html
./run.sh --mutace   # navíc mutační test (~3 min): ověří, že testy poznají smazání řádku
```

| soubor | co dělá |
|---|---|
| `sim.hpp` | referenční simulátor ISA (včetně skoků a SP); předpoklady o CPU jsou v hlavičce |
| `adapter.hpp` | jediné místo, kde se testy dotýkají kódu kompilátoru (při přejmenování uprav jen tady) |
| `suite_copy.hpp` | `copyFromTo` (4 druhy × umístění zásobník/R1/R2 × SP × velikost + fuzz) a `setRamLocal` |
| `suite_paths.hpp` | `variable::call` / `set` s cestami přes struktury a reference (i v pointingMode), 6000 náhodných cest |
| `suite_calls.hpp` | celé volání funkce přes zásobník, vnořená volání, různé počáteční SP |
| `tests.cpp` | spustí všechny sady, výsledky do `out/results.json` |
| `mutate.py` | mutační test nad `memoryManagment/*.cpp` |
| `report.py` | z `out/*.json` vyrobí `out/report.html` |

Návratový kód `run.sh`: 0 = vše prošlo, 1 = něco selhalo, 2 = nejde přeložit, 3 = testy spadly (report se nevytvoří).
