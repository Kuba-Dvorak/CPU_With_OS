# Zadání: lexikální analyzátor (`lexicalAnalyzer.cpp`)

Úkol je doplnit všechny prázdné funkce v `desing/compiler/lexicalAnalyzer.cpp`. Zbytek kompilátoru už je hotový a lexer jen volá. Proto níž je u každé funkce popsáno i to, **kdo ji volá a co s výsledkem dělá**. To je důležitější než název funkce.

## Rozsah práce

- **Měnit se smí jen** `lexicalAnalyzer.cpp` a do `lexicalSyntax.cpp` se smí přidávat chybějící klíčová slova a znaky (viz níž).
- Ostatní soubory (`astGenerator.cpp`, `syntaxAnalyzer.cpp`, `memoryManagment/*` atd.) jsou autorova práce. Když se v nich narazí na problém, napiš ho do shrnutí, ale **neupravuj je**.
- Styl kódu: if-řetězce, krátké funkce, komentáře česky **bez diakritiky**, stejně jako ve zbytku projektu.
- Překládá se jako C++20 a soubory se navzájem includují řetězcem (`lexicalAnalyzer.cpp` → `lexicalSyntax.cpp` → `memoryManagment/functions.cpp` → …). Nic do toho řetězce nepřidávej.

## Jak kompilátor funguje (to podstatné pro lexer)

1. `compiler::compile()` (`astGenerator.cpp`) prochází program **po příkazech**: `loadNextLine(originCode, lineNum)` pro `lineNum = 0, 1, 2…`. Každý příkaz pošle do `decideLineType` a podle typu zavolá `compileSegment` / `compileDeclaration` / `compileSetting` / `compileReturn` / `compileContinuation` / `compilePureCall`.
2. Tyto funkce si z příkazu vytáhnou části přes `loadVarDef`, `loadDefFunq`, `loadVarSetter`, `loadTypeDefName` a `findSegmenter`. `findSegmenter` vrací pozici, kde v příkazu začíná výraz.
3. Výraz zpracuje `createParsedSegment` / `parseBT` (`syntaxAnalyzer.cpp`). Ten volá **findery na úrovni znaků** (`findVar`, `findNumber`, `findOperator`, …), vždy s pozicí v řetězci příkazu.
4. Definici typu čte `createNewType` (`typeConstructor.cpp`). Sám si přes `loadNextLine` načítá další příkazy (pole typu) až po `};` a posouvá `lineNum` kompilátoru.

Lexer tedy má **dvě úrovně**: dělení programu na příkazy (A) a findery uvnitř výrazu (B).

---

## Syntaxe jazyka (rozhodnuto autorem)

**Každý příkaz končí `;`, i řádky se `{` a `}`.** Středník je jediný ukončovač příkazu. Program se tak dělí na příkazy jen podle `;`.

```
type Pair {;
    var x: number;
    var y: number(4);
    var next: $Pair;
};

function add(number a, number b) number {;
    return a + b;
};

function setX($Pair p, number v) number {;
    $p.x = v;
    return v;
};

var i: number;
var p: Pair;
i = 0;
p = [1, 2];
while {;
    if (i == 10) {;
        break;
    };
    i = call add /_ i, 1 _/;
    continue;
};
{;
    var t: number;
    t = !i & p.x;
};
call setX /_ $p, 5 _/;
```

| konstrukce | zápis | poznámka |
|---|---|---|
| deklarace proměnné | `var JMENO: TYP;` | `$` před typem = reference. Velikost čísla `number(N)` v bajtech. |
| velikost čísla | `number` = 2 B, `number(N)` = N B | výchozí 2 B jsou stejné jako ve `findType` |
| pole typu | stejně jako deklarace proměnné | uvnitř `type JMENO {;` … `};` |
| deklarace typu | `type JMENO {;` … `};` | |
| deklarace funkce | `function JMENO(TYP a, $TYP b) TYP {;` … `};` | parametry jsou `TYP JMENO` (bez `var` a dvojtečky), `$TYP` = reference; výstup `TYP` nebo `$TYP` |
| přiřazení | `JMENO.klic.klic = VYRAZ;` | `$` na začátku = zapsat do originálu, na který reference ukazuje (`changeOriginal`) |
| podmínka | `if (VYRAZ) {;` … `};` | jediný podmíněný příkaz, **žádné `else`** |
| cyklus | `while {;` … `};` | **bez podmínky**, končí jen přes `break`. Jiný cyklus není (`for` ne). |
| blok | `{;` … `};` | |
| návrat | `return VYRAZ;` | jen uvnitř funkce |
| skoky | `break;` `continue;` | |
| samostatné volání | `call JMENO /_ arg, arg _/;` | |
| výraz | čísla, proměnné s klíči, `call f /_ … _/`, `( … )`, konstruktor `[a, b]`, operátory, prefixy `!` `$` | podrobně v části B |
| komentáře | **žádné** | |

### Co přidat do `lexicalSyntax.cpp` (`syntaxDefiners`)

- `breakSignature = "break"` (chybí, i když AST typ `breaker` má),
- `varSignature = "var"`,
- oddělovač typu v deklaraci `":"`.

Velikost čísla `number(N)` může použít existující `normalBracketStart` / `normalBracketEnd`.

`forSignature` a `elseSignature` v syntaxi jsou, ale jazyk je nepodporuje (rozhodnutí autora).

---

## A. Úroveň příkazů

### Co je „příkaz“ (`loadNextLine`)

`goodLine loadNextLine(std::string &origCode, int lastLine)` vrátí **příkaz s pořadovým číslem `lastLine`** (od 0) z celého programu.

- **Bez stavu:** stejné číslo musí vždycky vrátit stejný příkaz. Volá ji `compile()` se svým čítačem a `createNewType` s vlastním čítačem.
  - Klidně se dá program pokaždé rozdělit znovu.
  - Nebo si rozdělení uložit do mezipaměti podle obsahu `origCode`.
- **Hranice příkazů:** příkaz končí **jen `;`** (`lexSyntax.endOfLine`) a středník do příkazu patří. `{` a `}` jsou obyčejné znaky uvnitř příkazu: `while {;`, `if (i == 10) {;`, `function … {;`, `type X {;`, `{;`, `};`.
- **Bílé znaky:**
  - Mezery, tabulátory a konce řádků mezi příkazy se zahodí.
  - Uvnitř příkazu se konce řádků a tabulátory nahradí mezerou, takže příkaz může být přes víc řádků.
  - Okraje příkazu se oříznou.
- **Návrat:**
  - Příkaz existuje: `{exists = true, oneLine = text příkazu včetně koncového ;, problem = false}`.
  - Za posledním příkazem: `{exists = false, "", problem = false}`.
  - Na konci zbyl text bez `;` (třeba `i = 1` nebo `}`): pro to číslo vrať `{exists = true, oneLine = ten text, problem = true}`. `compile()` pak nahlásí „chybí ukončovač řádku“.

> **Důležité: `{` u funkce, typu, `if` a `while` musí být ve stejném příkazu jako hlavička** (`function f() number {;`). Kdyby ho programátor napsal jako samostatný příkaz (`function f() number;` a pak `{;`), vznikl by z něj `segmentAvg`. Funkce by pak měla o scope víc a uzavírací `};` by ji nikdy neukončila, protože konec funkce se pozná podle toho, že zbývá jediný scope (`isLastScope`). Hlavička bez `{` je proto chyba programu (stačí ji nahlásit ve shrnutí jako otevřenou věc, mechanismus pro chyby tu není).

### `astNodeNames decideLineType(std::string &oneLine)`

Rozhoduje se podle **prvního slova nebo znaku** příkazu:

| začátek | typ |
|---|---|
| `}` (příkaz `};`) | `segmentEnder` |
| `{` (příkaz `{;`) | `segmentAvg` |
| `while` | `segmentCyclic` |
| `if` | `segmentConditional` |
| `type` | `declareType` |
| `function` | `declareFunction` |
| `var` | `declareVar` |
| `return` | `returner` |
| `continue` | `continuer` |
| `break` | `breaker` |
| `call` | `callFunq` |
| `$` nebo jméno, za ním volitelné `.klic` a pak `=` | `setVar` |

- Klíčová slova porovnávej **celá**. Proměnná `iffy` ani `variable` není `if` ani `var`: za klíčovým slovem nesmí následovat znak jména.
- Pro neznámý příkaz v `astNodeNames` žádná hodnota není a `compile()` takový příkaz potichu přeskočí. Nic nevymýšlej: vrať třeba `callFunq`, aby to aspoň spadlo při parsování, a v shrnutí doporuč autorovi přidat hodnotu pro chybu.

### `goodFoundSegment findSegmenter(std::string &oneLine)`

Vrací pozici, odkud začíná **výraz nebo konstruktor** příkazu. Ten se pak předá do `createParsedSegment`.

| příkaz | `oneLinePos` |
|---|---|
| `if (VYRAZ) {;` | hned za `if` (závorku zpracuje výraz sám jako závorku) |
| `JMENO.klic = VYRAZ;` | hned za `=` (to přiřazovací, ne `==`) |
| `return VYRAZ;` | hned za `return` |
| `call f /_ … _/;` | začátek `call` |
| jiné | `exists = false` |

Výraz sám skončí na `;` nebo `{`, protože to nejsou operátory (viz `findOperator`).

### `goodVarDecl loadVarDef(std::string &oneLine)`

Pro `var JMENO: [$]TYP[(N)];` vrátí:
- `varName = JMENO`,
- `varTypeName = TYP` (bez `$`),
- `isRef = (je tam $)`,
- `posibleSize = N`, bez `(N)` **2**.

Pozor: v struktuře je výchozí hodnota `1`, takže 2 vždycky nastav výslovně. U jiných typů než `number` na `posibleSize` nezáleží. Stejnou funkci volá `createNewType` pro pole typu.

### `goodLoadedFunctionDefinition loadDefFunq(std::string &oneLine)`

Pro `function JMENO(P1, P2, …) [$]TYP {;`, kde parametr je `[$]TYP JMENO`, vrátí:
- `funqName`,
- `inputNames`, `inputTypes` (bez `$`), `inputRefs`,
- `outputType` (bez `$`), `outputRef`.

Prázdné závorky `()` znamenají žádné parametry.

Struktura nemá pole pro velikost čísla, takže `number(N)` v hlavičce funkce zatím **nepodporuj**. Parametry a výstupy typu `number` mají vždycky 2 B (`define` volá `findType` bez velikosti). Uveď to v shrnutí jako otevřenou věc.

### `goodVarSet loadVarSetter(std::string &oneLine)`

Pro `[$]JMENO.k1.k2 = …;` vrátí:
- `varName = JMENO`,
- `keys = {k1, k2}`,
- `changeOriginal = (je tam $ na začátku)`.

### `std::string loadTypeDefName(std::string &oneLine)`

Z `type JMENO {;` vrátí `JMENO`.

### `goodBracket findSegmentEnd(std::string &oneLine)`

`exists = true`, právě když je příkaz `};` (po oříznutí mezer jen `}` a `;`). Volá ji `createNewType` a podle ní pozná konec typu. `position` = pozice za `}`.

### Funkce, které se zatím nikde nevolají

- `findSegmentStart`: udělej obdobu `findSegmentEnd` pro příkaz `{;`.
- `loadNodes`: vrať prázdný vektor.
- `findFunqStrBrc`: najde `/_`, stejně jako ostatní findery.

Prázdná funkce s návratovou hodnotou je v C++ nedefinované chování, takže i tyhle musí něco vracet.

---

## B. Úroveň výrazu (findery)

Pravidla pro **všechny** findery (komentář v souboru):
- Začínají na `startPosition` a nejdřív přeskočí mezery.
- Když tam hledaná věc **není**: `exists = false` a **`position = startPosition`**.
  - Volající někdy pozici použijí i při neúspěchu: `prepareFnInput` u `findFunqEndBrc`, `createParsedSegment` u `findConstructEnd`. Pozice proto nesmí být nesmysl.
- Když tam je: `position` = **první znak za ní**.
- **Nic nevypisují.** Pozor: `varHandle::returnVar`, `functionHandle::findFunction` i `typesHandle::findType` při nenalezení píšou na `cout`. Findery se ale volají „na zkoušku“: `getValueNode` zkouší proměnnou, číslo, funkci i závorku na stejné pozici. Hledej proto přímo v `varList` / `functionList`.

### Jak je volá `parseBT`

```
[prefix] HODNOTA ( OPERATOR [prefix] HODNOTA )*
```
- **prefix:** `findSingleOperator`. Je nejvýš jeden a zpracuje ho `parseBT`, ne finder hodnoty.
- **HODNOTA:** `getValueNode` zkusí na stejné pozici všechny čtyři možnosti a vybere v pořadí funkce → proměnná → číslo → závorka.
- Výraz končí, když `findOperator` nic nenajde. Proto `;` `{` `,` `)` `]` `_/` a samotné `=` **nesmí** být operátory.

| funkce | najde | `position` / výsledek |
|---|---|---|
| `loadName(line, pos)` | `[A-Za-z_][A-Za-z0-9_]*` | vrací jméno nebo `""`. Jméno končí i před `_/` (konec volání), protože `_` je znak jména: `a_/` je jméno `a` a pak konec volání. |
| `findVar(line, pos, vars)` | jméno a pak libovolně `.klic` | hledá ve `vars.varList` **odzadu** (vnitřní scope stíní vnější). Když jméno není proměnná nebo je to `call`: `exists = false`. `varKeys` jsou klíče a `position` je za posledním klíčem. Klíče se tu nekontrolují. **Bez prefixů** (`$`, `!` zpracovává `parseBT`), komentář `[prefixy]` v souboru je zastaralý. |
| `findFunction(line, pos, funcs)` | `call` JMENO `/_` | funkci hledej ve `funcs.functionList` podle jména (`main` má prázdné jméno). `position` = hned za `/_` (začátek argumentů), `endOfFunq` = za **odpovídajícím** `_/`. Počítá se zanoření: `/_` +1, `_/` −1. |
| `findNumber` | desítkové číslice | `value` jako `int`. Bez znaménka, protože unární mínus jazyk nemá. |
| `findStartBracket` / `findEndBracket` | `(` / `)` | bez prefixů (stejný důvod jako u `findVar`) |
| `findConstructStart` / `findConstructEnd` | `[` / `]` | konstruktor typu `[a, b]` |
| `findFunqStrBrc` / `findFunqEndBrc` | `/_` / `_/` | |
| `findSeparator` | `,` | mezi argumenty a v konstruktoru |
| `findOperator` | dvouznakové **dřív** než jednoznakové: `==` `=>` `=<` `=!`, pak `+ - * / & \| > <` | `/` za kterým je `_` není dělení (je to `/_`). Samotné `=` operátor není. |
| `findSingleOperator` | `!` → `notOperBit`, `$` → `isPointer` | když prefix není: `{noSingleOper, startPosition}` |

Operátory bez znaku v syntaxi: `xorOperator` (výpočet umí) a prefix `notOperByte`. Znaky pro ně nevymýšlej, napiš do shrnutí, že chybí.

### Referenční implementace a testy

- `AI-tests/mock_lexer.cpp` je náhradní lexer, se kterým testy teď běží. Implementuje **celou část B** podle těchto pravidel a prochází v testech: 1 200 náhodných výrazů, 144 dvojic operátorů a 75 volání funkcí z textu. Můžeš ho použít jako vzor. Část A v něm není.
- Ověření části B: `cd desing/compiler/AI-tests && ./run.sh --lexer` spustí všechny testy s **tvým** `lexicalAnalyzer.cpp` místo náhradního. Cíl: stejný výsledek jako `./run.sh`, tedy 0 selhání ve všech řádcích.
- Pro část A zatím testy nejsou. Tabulka níž je zadání, které musí sedět. Testy a pomocný kód patří jen do `AI-tests/`, ne do souborů kompilátoru.

---

## Očekávané výsledky pro ukázkový program

Program je ten z části „Syntaxe jazyka“.

| # | příkaz (`oneLine`) | `decideLineType` | další funkce |
|---|---|---|---|
| 0 | `type Pair {;` | `declareType` | `loadTypeDefName` = `Pair` |
| 1 | `var x: number;` | (čte `createNewType`) | `loadVarDef` = `{x, number, false, 2}` |
| 2 | `var y: number(4);` | | `{y, number, false, 4}` |
| 3 | `var next: $Pair;` | | `{next, Pair, true, 2}` |
| 4 | `};` | `segmentEnder` | `findSegmentEnd.exists = true` |
| 5 | `function add(number a, number b) number {;` | `declareFunction` | `{add, [a, b], [false, false], [number, number], number, false}` |
| 6 | `return a + b;` | `returner` | `findSegmenter` → za `return` |
| 7 | `};` | `segmentEnder` | |
| 8 | `function setX($Pair p, number v) number {;` | `declareFunction` | `{setX, [p, v], [true, false], [Pair, number], number, false}` |
| 9 | `$p.x = v;` | `setVar` | `loadVarSetter` = `{p, [x], true}`, `findSegmenter` → za `=` |
| 10 | `return v;` | `returner` | |
| 11 | `};` | `segmentEnder` | |
| 12 | `var i: number;` | `declareVar` | `{i, number, false, 2}` |
| 13 | `var p: Pair;` | `declareVar` | `{p, Pair, false, 2}` |
| 14 | `i = 0;` | `setVar` | `{i, [], false}` |
| 15 | `p = [1, 2];` | `setVar` | `findSegmenter` → před `[` (za `=`) |
| 16 | `while {;` | `segmentCyclic` | |
| 17 | `if (i == 10) {;` | `segmentConditional` | `findSegmenter` → za `if` |
| 18 | `break;` | `breaker` | |
| 19 | `};` | `segmentEnder` | |
| 20 | `i = call add /_ i, 1 _/;` | `setVar` | |
| 21 | `continue;` | `continuer` | |
| 22 | `};` | `segmentEnder` | |
| 23 | `{;` | `segmentAvg` | |
| 24 | `var t: number;` | `declareVar` | |
| 25 | `t = !i & p.x;` | `setVar` | |
| 26 | `};` | `segmentEnder` | |
| 27 | `call setX /_ $p, 5 _/;` | `callFunq` | `findSegmenter` → 0 |
| 28 | — | | `loadNextLine.exists = false` |

Kdyby program končil `i = 1` nebo `}` bez středníku, `loadNextLine` pro to číslo vrátí `problem = true`.

## Na co si dát pozor (shrnutí)

1. `loadNextLine` musí být bez stavu (stejné číslo → stejný příkaz). Příkazy se dělí **jen podle `;`**, i řádky se `{` a `}` (`while {;`, `};`).
2. Findery nic nevypisují a při neúspěchu vracejí `position = startPosition`.
3. Klíčová slova se porovnávají jako celá slova.
4. `_` může být ve jménu, ale jméno končí před `_/`.
5. `=` (přiřazení) není operátor. `==`, `=>`, `=<`, `=!` operátory jsou a testují se dřív než jednoznakové.
6. Prefixy `$` a `!` zpracovává `parseBT`. `findVar` a `findStartBracket` je nečtou.
7. `posibleSize` nastavuj výslovně (bez `(N)` na 2).
8. Všechny funkce s návratovou hodnotou musí něco vracet, i ty nepoužívané.
9. Když něco nejde udělat bez změny mimo `lexicalAnalyzer.cpp` a `lexicalSyntax.cpp`, napiš to do shrnutí a nic neměň.

## Otevřené věci (rozhodne autor, agent je jen nahlásí)

- Velikost čísla `number(N)` u parametrů a výstupu funkce: `goodLoadedFunctionDefinition` pro ni nemá pole.
- Znak pro `xor` a pro `notOperByte`.
- Hodnota `astNodeNames` pro neznámý nebo chybný příkaz (teď ho `compile()` potichu přeskočí).
- Funkce bez výstupu (`void`): `define` volá `findType(outPutTypeName)->size`, takže by spadla.
