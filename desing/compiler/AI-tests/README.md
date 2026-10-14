# AI-tests

Testy psané AI. Kód kompilátoru nemění, jen ho volají přes `adapter.hpp`.

```
./run.sh            # přeloží a spustí testy, vytvoří out/report.html
./run.sh --mutace   # navíc mutační test (~30 s): ověří, že testy chytí každé smazání řádku
```

| soubor | co dělá |
|---|---|
| `sim.hpp` | referenční simulátor ISA podle `syntax.cpp`; předpoklady o CPU jsou v hlavičce |
| `adapter.hpp` | jediné místo, kde se testy dotýkají kódu kompilátoru (při přejmenování uprav jen tady) |
| `test_copy.cpp` | `copyFromTo`: všechny cesty × umístění × velikosti, okrajové případy, fuzz |
| `mutate.py` | mutační test nad `memoryManagment/memoryOperations.cpp` |
| `report.py` | z `out/*.json` vyrobí `out/report.html` |

Návratový kód `run.sh`: 0 = vše prošlo, 1 = něco selhalo, 2 = nejde přeložit.
