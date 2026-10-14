#!/usr/bin/env python3
"""Z vysledku testu (out/*.json) udela jeden HTML report: out/report.html"""
import datetime
import json
import os
import re
import subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")


def load(name):
    p = os.path.join(OUT, name)
    if not os.path.exists(p):
        return None
    with open(p) as f:
        return json.load(f)


def git_info():
    try:
        h = subprocess.run(["git", "rev-parse", "--short", "HEAD"], cwd=HERE, capture_output=True, text=True).stdout.strip()
        dirty = subprocess.run(["git", "status", "--porcelain", "--", "../memoryManagment", "../syntax.cpp"],
                               cwd=HERE, capture_output=True, text=True).stdout.strip()
        return h + (" + neuložené změny" if dirty else "")
    except OSError:
        return "?"


def assumptions():
    # predpoklady simulatoru se ctou primo z hlavicky sim.hpp, aby report nelhal
    out = []
    with open(os.path.join(HERE, "sim.hpp")) as f:
        for ln in f:
            m = re.match(r"//\s+-\s+(.*)", ln)
            if m:
                out.append(m.group(1).strip())
    return out


data = {
    "copy": load("copy.json"),
    "mutants": load("mutants.json"),
    "meta": {
        "commit": git_info(),
        "time": datetime.datetime.now().strftime("%Y-%m-%d %H:%M"),
        "assumptions": assumptions(),
    },
}

TEMPLATE = r"""<!doctype html>
<html lang="cs">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>AI testy kompilátoru</title>
<style>
:root {
  color-scheme: light;
  --page: #f9f9f7; --surface: #fcfcfb; --surface-2: #f3f2ee;
  --ink: #0b0b0b; --ink-2: #52514e; --muted: #898781;
  --grid: #e1e0d9; --axis: #c3c2b7; --border: rgba(11,11,11,0.10);
  --s1: #2a78d6; --s2: #eb6834; --s3: #1baf7a;
  --good: #0ca30c; --warn: #fab219; --bad: #d03b3b; --none: #e1e0d9;
  --on-good: #ffffff; --on-warn: #0b0b0b; --on-bad: #ffffff;
  --accent: #2a78d6;
}
@media (prefers-color-scheme: dark) {
  :root:where(:not([data-theme="light"])) {
    color-scheme: dark;
    --page: #0d0d0d; --surface: #1a1a19; --surface-2: #232321;
    --ink: #ffffff; --ink-2: #c3c2b7; --muted: #898781;
    --grid: #2c2c2a; --axis: #383835; --border: rgba(255,255,255,0.10);
    --s1: #3987e5; --s2: #d95926; --s3: #199e70; --none: #2c2c2a;
    --accent: #3987e5;
  }
}
:root[data-theme="dark"] {
  color-scheme: dark;
  --page: #0d0d0d; --surface: #1a1a19; --surface-2: #232321;
  --ink: #ffffff; --ink-2: #c3c2b7; --muted: #898781;
  --grid: #2c2c2a; --axis: #383835; --border: rgba(255,255,255,0.10);
  --s1: #3987e5; --s2: #d95926; --s3: #199e70; --none: #2c2c2a;
  --accent: #3987e5;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--page); color: var(--ink);
  font: 14px/1.5 system-ui, -apple-system, "Segoe UI", sans-serif; }
main { max-width: 1120px; margin: 0 auto; padding: 24px 16px 64px; }
h1 { font-size: 22px; margin: 0 0 4px; font-weight: 650; }
h2 { font-size: 16px; margin: 0 0 4px; font-weight: 650; }
.sub { color: var(--ink-2); margin: 0 0 12px; }
.meta { color: var(--muted); font-size: 12px; }
section { background: var(--surface); border: 1px solid var(--border); border-radius: 10px;
  padding: 18px; margin-top: 16px; }
code, .mono { font-family: ui-monospace, "JetBrains Mono", Menlo, monospace; font-size: 12.5px; }
.tiles { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 12px; margin-top: 16px; }
.tile { background: var(--surface); border: 1px solid var(--border); border-radius: 10px; padding: 14px 16px; }
.tile .label { color: var(--ink-2); font-size: 13px; }
.tile .value { font-size: 28px; font-weight: 650; margin-top: 2px; }
.tile .hint { color: var(--muted); font-size: 12px; }
.badge { display: inline-flex; align-items: center; gap: 4px; padding: 1px 8px; border-radius: 999px;
  font-size: 12px; font-weight: 600; white-space: nowrap; }
.b-pass { background: var(--good); color: var(--on-good); }
.b-warn { background: var(--warn); color: var(--on-warn); }
.b-fail { background: var(--bad); color: var(--on-bad); }
.b-none { background: var(--none); color: var(--ink-2); }
.scroll { overflow-x: auto; }
table { border-collapse: collapse; }
.tbl { width: 100%; font-size: 13px; }
.tbl th { text-align: left; color: var(--ink-2); font-weight: 600; border-bottom: 1px solid var(--axis); padding: 6px 8px; }
.tbl td { border-bottom: 1px solid var(--grid); padding: 5px 8px; vertical-align: top; }
.tbl td.num, .tbl th.num { text-align: right; font-variant-numeric: tabular-nums; }
/* heatmap */
.hm { border-collapse: separate; border-spacing: 2px; }
.hm th { font-weight: 500; color: var(--ink-2); font-size: 12px; padding: 0 2px; text-align: center; }
.hm th.grp { color: var(--ink); font-weight: 600; border-bottom: 1px solid var(--axis); padding-bottom: 2px; }
.hm td.rl { text-align: left; color: var(--ink-2); font-size: 12.5px; padding-right: 10px; white-space: nowrap; }
.hm tr.kind td { padding-top: 10px; font-weight: 600; color: var(--ink); font-size: 13px; }
.hm tr.kind td span { color: var(--muted); font-weight: 400; }
.cell { width: 26px; height: 26px; border-radius: 4px; text-align: center; font-size: 13px; font-weight: 700;
  cursor: pointer; padding: 0; line-height: 26px; }
.cell:hover, .cell.sel { outline: 2px solid var(--ink); outline-offset: 1px; }
.c-pass { background: var(--good); color: var(--on-good); }
.c-warn { background: var(--warn); color: var(--on-warn); }
.c-fail { background: var(--bad); color: var(--on-bad); }
.fz { width: auto; min-width: 64px; padding: 0 6px; font-size: 12px; font-weight: 600; }
.gap { width: 10px; }
.legend { display: flex; flex-wrap: wrap; gap: 14px; color: var(--ink-2); font-size: 12.5px; margin: 8px 0 12px; }
.legend i { display: inline-block; width: 12px; height: 12px; border-radius: 3px; vertical-align: -2px; margin-right: 5px; }
.legend i.line { height: 2px; width: 16px; border-radius: 1px; vertical-align: 3px; }
/* detail */
#detail { margin-top: 14px; border-top: 1px solid var(--grid); padding-top: 14px; display: none; }
#detail.on { display: block; }
.asm { background: var(--surface-2); border-radius: 8px; padding: 10px 12px; margin: 8px 0; overflow-x: auto; }
.asm div { white-space: pre; }
.asm .ln { color: var(--muted); display: inline-block; width: 28px; }
.asm .err { color: var(--bad); font-weight: 600; }
.cols { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 16px; }
/* charts */
.multiples { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 18px; }
.chart h3 { font-size: 13px; margin: 0; font-weight: 600; }
.chart p { margin: 0 0 4px; color: var(--muted); font-size: 12px; }
.chart svg { width: 100%; height: auto; display: block; overflow: visible; }
.chart text { fill: var(--muted); font-size: 10px; font-variant-numeric: tabular-nums; }
#tip { position: fixed; pointer-events: none; background: var(--surface); color: var(--ink); border: 1px solid var(--border);
  border-radius: 8px; padding: 8px 10px; font-size: 12px; box-shadow: 0 4px 16px rgba(0,0,0,.18); display: none;
  z-index: 10; max-width: 340px; }
#tip .k { display: inline-block; width: 10px; height: 2px; vertical-align: 3px; margin-right: 6px; }
#tip b { font-weight: 600; }
details { margin-top: 10px; }
summary { cursor: pointer; color: var(--ink-2); }
.dot { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: var(--accent); }
ul.findings { margin: 8px 0 0; padding-left: 0; list-style: none; }
ul.findings li { padding: 8px 0; border-bottom: 1px solid var(--grid); display: flex; gap: 10px; align-items: flex-start; }
ul.findings li:last-child { border-bottom: 0; }
ul.assume { margin: 6px 0 0; padding-left: 18px; color: var(--ink-2); }
.trace td { white-space: nowrap; }
</style>
</head>
<body>
<main>
  <h1>AI testy kompilátoru</h1>
  <p class="sub">Generovaný kód z <code>memoryManagment/memoryOperations.cpp</code> spuštěný v referenčním simulátoru ISA podle <code>syntax.cpp</code>.</p>
  <div class="meta" id="meta"></div>
  <div class="tiles" id="tiles"></div>

  <section>
    <h2>Co testy našly</h2>
    <p class="sub">Selhání = kód udělá s pamětí něco jiného, než má. Varování = chování, které je spíš otázka návrhu než chyba.</p>
    <ul class="findings" id="findings"></ul>
  </section>

  <section>
    <h2>copyFromTo: matice všech kombinací</h2>
    <p class="sub">Řádek = cesta v kódu (co je zdroj/cíl a jestli je adresa v R1). Sloupec = umístění v paměti × velikost v bajtech. Klikni na buňku pro vygenerovaný asm, trasu a rozdíly v RAM.</p>
    <div class="legend">
      <span><i class="c-pass"></i>✓ prošlo</span><span><i class="c-warn"></i>! varování</span><span><i class="c-fail"></i>✗ selhalo</span>
    </div>
    <div class="scroll"><table class="hm" id="heatmap"></table></div>
    <div id="detail"></div>
  </section>

  <section>
    <h2>Cena kopírování: počet instrukcí podle velikosti</h2>
    <p class="sub">Kolik instrukcí vygeneruje jedno přiřazení. Všechny 4 grafy mají stejnou osu Y, aby šly porovnat.</p>
    <div class="legend">
      <span><i class="line" style="background:var(--s1)"></i>RAM → RAM</span>
      <span><i class="line" style="background:var(--s2)"></i>zdroj v R1</span>
      <span><i class="line" style="background:var(--s3)"></i>cíl v R1</span>
    </div>
    <div class="multiples" id="charts"></div>
    <details><summary>Tabulka</summary><div class="scroll" id="countTable"></div></details>
  </section>

  <section>
    <h2>Rozdílné velikosti zdroje a cíle</h2>
    <p class="sub">Očekávání testu: zkopírují se dolní bajty, a když je cíl větší, horní bajty se vynulují. Pokud to není tvoje sémantika, ber to jako otázku, ne chybu.</p>
    <div class="scroll" id="sizes"></div>
  </section>

  <section>
    <h2>Které registry kód mění</h2>
    <p class="sub">Sjednocení přes všechny testy matice. Hodí se pro konvenci registrů: co volající nesmí čekat zachované.</p>
    <div class="scroll" id="regs"></div>
  </section>

  <section id="mutSec">
    <h2>Mutační test: jsou testy dost citlivé?</h2>
    <p class="sub">Z <code>memoryOperations.cpp</code> se vždy smaže jeden řádek a testy se pustí znovu. Zabitý mutant znamená, že testy rozbití poznaly.</p>
    <div id="mutants"></div>
  </section>

  <section>
    <h2>Trasy krok po kroku</h2>
    <p class="sub">Velikost 2 B, stejný segment. Registry po každé instrukci a každý zápis do RAM.</p>
    <div id="traces"></div>
  </section>

  <section>
    <h2>Předpoklady simulátoru</h2>
    <p class="sub">Načteno z hlavičky <code>AI-tests/sim.hpp</code>. Pokud něco nesedí s CPU, oprav to tam a testy se tím budou řídit.</p>
    <ul class="assume" id="assume"></ul>
  </section>
</main>
<div id="tip"></div>
<script>
const DATA = __DATA__;
const C = DATA.copy, M = DATA.mutants;
const $ = id => document.getElementById(id);
const esc = s => String(s).replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const hex = (v, w = 4) => '0x' + v.toString(16).toUpperCase().padStart(w, '0');
const segs = Object.entries(C.segments).sort((a, b) => b[1] - a[1]);
function sym(a) {
  for (const [n, b] of segs) if (a >= b && a - b < 0x4000) return n + '+' + (a - b);
  return hex(a);
}
const ICON = {pass: '✓', warn: '!', fail: '✗'};
const WORD = {pass: 'prošlo', warn: 'varování', fail: 'selhalo'};
const KIND = {
  'ne a ne': 'hodnota → hodnota', 'ano a ne': 'pointer → hodnota (kopie přes dereferenci)',
  'ne a ano': 'hodnota → pointer (uloží adresu zdroje)', 'ano a ano': 'pointer → pointer (kopie pointeru)'
};
const pathById = Object.fromEntries(C.paths.map(p => [p.id, p]));
const badge = st => `<span class="badge b-${st}">${ICON[st] || ''} ${WORD[st] || st}</span>`;

$('meta').textContent = `commit ${DATA.meta.commit} · vygenerováno ${DATA.meta.time}`;

// ---------- tiles
const res = C.results;
const cnt = st => res.filter(r => r.status === st).length;
const fuzzRuns = C.fuzz.reduce((a, f) => a + f.runs, 0);
const fuzzFail = C.fuzz.reduce((a, f) => a + f.fail, 0);
const tiles = [
  ['Testů', res.length, 'matice, okrajové případy, velikosti'],
  ['Prošlo', cnt('pass'), ''],
  ['Selhalo', cnt('fail'), cnt('fail') ? 'viz níže' : 'žádné selhání'],
  ['Varování', cnt('warn'), 'otázky návrhu'],
  ['Fuzz běhů', fuzzRuns.toLocaleString('cs'), fuzzFail ? fuzzFail + ' selhalo' : 'všechny prošly'],
];
if (M) {
  const k = M.mutants.filter(m => m.result === 'zabit').length;
  tiles.push(['Mutanti zabiti', `${k} / ${M.mutants.length}`, k === M.mutants.length ? 'testy odhalily každé rozbití' : 'některé přežily']);
}
$('tiles').innerHTML = tiles.map(([l, v, h]) =>
  `<div class="tile"><div class="label">${l}</div><div class="value">${v}</div><div class="hint">${h}</div></div>`).join('');

// ---------- findings
(function () {
  const groups = new Map();
  for (const r of res) if (r.status !== 'pass') {
    const msg = r.errors.length ? r.errors[0].msg : (r.note || (r.diffCount + ' rozdílů v RAM'));
    const key = r.status + '|' + r.group + '|' + msg;
    if (!groups.has(key)) groups.set(key, {st: r.status, group: r.group, msg, n: 0, paths: new Set(), ex: r});
    const g = groups.get(key); g.n++; g.paths.add(r.path);
  }
  for (const f of C.fuzz) if (f.fail) groups.set('fz' + f.path, {st: 'fail', group: 'fuzz', msg: `${f.fail} z ${f.runs} náhodných běhů selhalo`, n: f.fail, paths: new Set([f.path]), ex: f.firstFail});
  const items = [...groups.values()].sort((a, b) => (a.st === 'fail' ? 0 : 1) - (b.st === 'fail' ? 0 : 1));
  $('findings').innerHTML = items.length ? items.map(g => {
    const ps = [...g.paths].map(id => { const p = pathById[id]; return `${p.kind} / ${p.where}`; });
    return `<li>${badge(g.st)}<div><b>${esc(g.msg)}</b><br><span class="meta">${g.group} · ${g.n}× · ${esc(ps.slice(0, 4).join(', '))}${ps.length > 4 ? ' …' : ''}</span></div></li>`;
  }).join('') : `<li>${badge('pass')}<div>Nic. Všechny testy prošly.</div></li>`;
})();

// ---------- heatmap
const matrix = res.filter(r => r.group === 'matice');
const placements = [...new Set(matrix.map(r => r.placement))];
const sizes = [...new Set(matrix.map(r => r.toSize))].sort((a, b) => a - b);
const fuzzBy = Object.fromEntries(C.fuzz.map(f => [f.path, f]));
const cellIdx = new Map(matrix.map((r, i) => [r.path + '|' + r.placement + '|' + r.toSize, i]));
(function () {
  let h = '<tr><th></th>' + placements.map(p => `<th class="gap"></th><th class="grp" colspan="${sizes.length}">${esc(p)}</th>`).join('') + '<th class="gap"></th><th class="grp">fuzz</th></tr>';
  h += '<tr><th></th>' + placements.map(() => '<th class="gap"></th>' + sizes.map(s => `<th>${s}</th>`).join('')).join('') + '<th class="gap"></th><th>300×</th></tr>';
  let lastKind = '';
  for (const p of C.paths) {
    if (p.kind !== lastKind) {
      lastKind = p.kind;
      h += `<tr class="kind"><td colspan="${2 + placements.length * (sizes.length + 1) + 1}">„${p.kind}“ <span>${KIND[p.kind]}</span></td></tr>`;
    }
    h += `<tr><td class="rl">${p.where}</td>`;
    for (const pl of placements) {
      h += '<td class="gap"></td>';
      for (const s of sizes) {
        const i = cellIdx.get(p.id + '|' + pl + '|' + s);
        const r = matrix[i];
        h += `<td><button class="cell c-${r.status}" data-i="${i}" aria-label="${esc(p.kind + ', ' + p.where + ', ' + pl + ', ' + s + ' B: ' + WORD[r.status])}">${ICON[r.status]}</button></td>`;
      }
    }
    const f = fuzzBy[p.id];
    const st = f.fail ? 'fail' : (f.warn ? 'warn' : 'pass');
    h += `<td class="gap"></td><td><button class="cell fz c-${st}" data-fz="${p.id}">${f.fail ? '✗ ' + f.fail : (f.warn ? '! ' + f.warn : '✓ ' + f.pass)}</button></td></tr>`;
  }
  $('heatmap').innerHTML = h;
})();

function renderResult(r, title) {
  const errLines = new Set(r.errors.map(e => e.line));
  let h = `<h2>${esc(title)}</h2><p class="sub">${badge(r.status)} ${esc(r.layout || '')}${r.note ? ' · ' + esc(r.note) : ''}</p>`;
  h += '<div class="cols"><div>';
  h += `<b>Vygenerovaný kód</b> <span class="meta">(${r.code.length} instrukcí)</span><div class="asm mono">`;
  h += r.code.length ? r.code.map((c, i) => `<div class="${errLines.has(i) ? 'err' : ''}"><span class="ln">${i}</span>${esc(c.trim().replace(/\s+/g, ' '))}</div>`).join('') : '<div class="meta">(nic)</div>';
  h += '</div>';
  if (r.errors.length) h += r.errors.map(e => `<div class="err">řádek ${e.line}: ${esc(e.msg)}</div>`).join('');
  h += '</div><div>';
  if (r.diffCount) {
    h += `<b>Rozdíly v RAM</b> <span class="meta">(${r.diffCount} bajtů${r.diffCount > r.diffs.length ? ', zobrazeno ' + r.diffs.length : ''})</span>`;
    h += '<table class="tbl mono"><tr><th>adresa</th><th class="num">čekáno</th><th class="num">je</th><th></th></tr>' +
      r.diffs.map(d => `<tr><td>${sym(d.addr)} <span class="meta">${hex(d.addr)}</span></td><td class="num">${hex(d.exp, 2)}</td><td class="num">${hex(d.act, 2)}</td><td class="meta">${d.inTarget ? 'v cíli' : 'MIMO cíl'}</td></tr>`).join('') + '</table>';
  } else if (r.code.length) {
    h += '<b>RAM</b><p class="meta">Po spuštění přesně odpovídá očekávání (porovnáno všech 65 536 bajtů).</p>';
  }
  h += '</div></div>';
  if (r.trace && r.trace.length) h += traceTable(r);
  return h;
}

$('heatmap').addEventListener('click', e => {
  const b = e.target.closest('button'); if (!b) return;
  document.querySelectorAll('.cell.sel').forEach(x => x.classList.remove('sel'));
  b.classList.add('sel');
  const d = $('detail'); d.classList.add('on');
  if (b.dataset.fz) {
    const f = fuzzBy[b.dataset.fz], p = pathById[f.path];
    d.innerHTML = f.firstFail ? renderResult(f.firstFail, `Fuzz: první selhání, ${p.kind} / ${p.where}`)
      : `<h2>Fuzz: ${p.kind} / ${p.where}</h2><p class="sub">${f.runs} náhodných běhů (velikost 1 až 32 B, náhodné segmenty a offsety): ${f.pass} prošlo, ${f.fail} selhalo, ${f.warn} varování.</p>`;
  } else {
    const r = matrix[+b.dataset.i], p = pathById[r.path];
    d.innerHTML = renderResult(r, `${p.kind} / ${p.where} · ${r.placement} · ${r.toSize} B`);
  }
});

// ---------- tooltip
const tip = $('tip');
function showTip(html, x, y) {
  tip.innerHTML = html; tip.style.display = 'block';
  const w = tip.offsetWidth, hh = tip.offsetHeight;
  tip.style.left = Math.min(x + 14, innerWidth - w - 8) + 'px';
  tip.style.top = Math.max(8, y - hh - 10) + 'px';
}
const hideTip = () => tip.style.display = 'none';
$('heatmap').addEventListener('mousemove', e => {
  const b = e.target.closest('button'); if (!b) return hideTip();
  if (b.dataset.fz) { const f = fuzzBy[b.dataset.fz]; return showTip(`<b>fuzz</b> · ${f.pass} ✓ · ${f.warn} ! · ${f.fail} ✗`, e.clientX, e.clientY); }
  const r = matrix[+b.dataset.i], p = pathById[r.path];
  showTip(`<b>${p.kind} / ${p.where}</b><br>${esc(r.placement)}, ${r.toSize} B · ${r.code.length} instrukcí<br>${badge(r.status)}${r.note ? '<br>' + esc(r.note) : ''}`, e.clientX, e.clientY);
});
$('heatmap').addEventListener('mouseleave', hideTip);

// ---------- small multiples
(function () {
  const kinds = [...new Set(C.paths.map(p => p.kind))];
  const series = [['RAM → RAM', 'var(--s1)', p => !p.fromR1 && !p.toR1], ['zdroj v R1', 'var(--s2)', p => p.fromR1 && !p.toR1], ['cíl v R1', 'var(--s3)', p => !p.fromR1 && p.toR1]];
  const countBy = Object.fromEntries(C.counts.map(c => [c.path, c.counts]));
  const yMax = Math.max(...C.counts.map(c => Math.max(...c.counts)));
  const step = yMax > 40 ? 20 : 10, top = Math.ceil(yMax / step) * step;
  const W = 260, H = 170, L = 30, R = 8, T = 8, B = 22;
  const n = C.counts[0].counts.length;
  const X = i => L + (W - L - R) * i / (n - 1), Y = v => T + (H - T - B) * (1 - v / top);
  let html = '';
  kinds.forEach((k, ki) => {
    let s = `<svg viewBox="0 0 ${W} ${H}" data-k="${ki}" role="img" aria-label="Počet instrukcí pro ${k}">`;
    for (let v = 0; v <= top; v += step) s += `<line x1="${L}" x2="${W - R}" y1="${Y(v)}" y2="${Y(v)}" stroke="var(${v ? '--grid' : '--axis'})" stroke-width="1"/><text x="${L - 5}" y="${Y(v) + 3}" text-anchor="end">${v}</text>`;
    for (const i of [0, 3, 7, 11, 15]) s += `<text x="${X(i)}" y="${H - 6}" text-anchor="middle">${i + 1} B</text>`;
    const lines = [];
    for (const [name, col, pred] of series) {
      const p = C.paths.find(p => p.kind === k && pred(p)); if (!p) continue;
      const c = countBy[p.id]; lines.push([name, col, c]);
      s += `<polyline fill="none" stroke="${col}" stroke-width="2" stroke-linejoin="round" stroke-linecap="round" points="${c.map((v, i) => X(i) + ',' + Y(v)).join(' ')}"/>`;
      s += `<circle cx="${X(n - 1)}" cy="${Y(c[n - 1])}" r="4" fill="${col}" stroke="var(--surface)" stroke-width="2"/>`;
    }
    s += `<line class="xh" x1="0" x2="0" y1="${T}" y2="${H - B}" stroke="var(--axis)" stroke-width="1" visibility="hidden"/>`;
    s += `<rect x="${L}" y="0" width="${W - L - R}" height="${H}" fill="transparent"/></svg>`;
    const max16 = Math.max(...lines.map(l => l[2][n - 1]));
    // stejne rady se v grafu prekryvaji - rict to nahlas, at jedna cara nezmizi beze stopy
    const same = [];
    for (let a = 0; a < lines.length; a++) for (let b = a + 1; b < lines.length; b++)
      if (lines[a][2].join() === lines[b][2].join()) same.push(`${lines[a][0]} = ${lines[b][0]}`);
    const overlap = same.length ? ` · překrývá se: ${same.join(', ')}` : '';
    html += `<div class="chart"><h3>„${k}“</h3><p>${KIND[k]} · při 16 B až ${max16} instrukcí${overlap}</p>${s}</div>`;
    window['_lines' + ki] = lines;
  });
  $('charts').innerHTML = html;
  $('charts').querySelectorAll('svg').forEach(svg => {
    const ki = svg.dataset.k, xh = svg.querySelector('.xh');
    svg.addEventListener('mousemove', e => {
      const pt = svg.getBoundingClientRect(), sx = (e.clientX - pt.left) * W / pt.width;
      const i = Math.max(0, Math.min(n - 1, Math.round((sx - L) / (W - L - R) * (n - 1))));
      xh.setAttribute('x1', X(i)); xh.setAttribute('x2', X(i)); xh.setAttribute('visibility', 'visible');
      showTip(`<b>${i + 1} B</b><br>` + window['_lines' + ki].map(([nm, col, c]) => `<span class="k" style="background:${col}"></span>${nm}: <b>${c[i]}</b>`).join('<br>'), e.clientX, e.clientY);
    });
    svg.addEventListener('mouseleave', () => { xh.setAttribute('visibility', 'hidden'); hideTip(); });
  });
  let t = '<table class="tbl"><tr><th>cesta</th>' + C.counts[0].counts.map((_, i) => `<th class="num">${i + 1}</th>`).join('') + '</tr>';
  for (const c of C.counts) { const p = pathById[c.path]; t += `<tr><td>${p.kind} / ${p.where}</td>${c.counts.map(v => `<td class="num">${v}</td>`).join('')}</tr>`; }
  $('countTable').innerHTML = t + '</table>';
})();

// ---------- sizes
(function () {
  const rs = res.filter(r => r.group === 'velikosti');
  const mixes = [...new Set(rs.map(r => r.placement))];
  const ps = [...new Set(rs.map(r => r.path))];
  let t = '<table class="tbl"><tr><th>cesta</th>' + mixes.map(m => `<th>${m}</th>`).join('') + '</tr>';
  for (const id of ps) {
    const p = pathById[id];
    t += `<tr><td>${p.kind} / ${p.where}</td>` + mixes.map(m => { const r = rs.find(r => r.path === id && r.placement === m); return `<td>${badge(r.status)}</td>`; }).join('') + '</tr>';
  }
  $('sizes').innerHTML = t + '</table>';
})();

// ---------- registers
(function () {
  let t = '<table class="tbl"><tr><th>cesta</th>' + [0, 1, 2, 3, 4, 5].map(i => `<th class="num">R${i}</th>`).join('') + '</tr>';
  for (const p of C.paths) {
    if (p.fromR1 && p.toR1) continue;
    const w = new Set(matrix.filter(r => r.path === p.id).flatMap(r => r.regsWritten));
    t += `<tr><td>${p.kind} / ${p.where}</td>` + [0, 1, 2, 3, 4, 5].map(i => `<td class="num">${w.has(i) ? '<span class="dot" title="zapisuje"></span>' : '<span class="meta">·</span>'}</td>`).join('') + '</tr>';
  }
  $('regs').innerHTML = t + '</table><p class="meta">R0 se mění přes <code>increment</code> (výsledek ALU), přímý zápis do R0 by byl hlášen jako chyba.</p>';
})();

// ---------- mutants
if (!M) $('mutSec').innerHTML += '<p class="meta">Nespuštěno. Pusť <code>./run.sh --mutace</code>.</p>';
else {
  const st = m => m.result === 'zabit' ? 'pass' : (m.result === 'prezil' ? 'fail' : 'none');
  const word = {zabit: 'zabit', prezil: 'přežil', nekompiluje: 'nekompiluje'};
  $('mutants').innerHTML = '<div class="scroll"><table class="tbl"><tr><th class="num">řádek</th><th>smazaný kód</th><th>druh</th><th>výsledek</th></tr>' +
    M.mutants.map(m => `<tr><td class="num">${m.line}</td><td class="mono">${esc(m.code)}</td><td>${m.kind}</td><td><span class="badge b-${st(m)}">${word[m.result]}</span></td></tr>`).join('') + '</table></div>';
}

// ---------- traces
function traceTable(r) {
  let t = '<details open><summary>Trasa</summary><div class="scroll"><table class="tbl mono trace"><tr><th>#</th><th>instrukce</th><th>R0</th><th>R1</th><th>R2</th><th>zápisy do RAM</th></tr>';
  t += `<tr><td class="meta">start</td><td class="meta">${esc(r.layout)}</td>${[0, 1, 2].map(i => `<td>${hex(r.regsBefore[i])}</td>`).join('')}<td></td></tr>`;
  let prev = r.regsBefore;
  for (const row of r.trace) {
    const regs = [0, 1, 2].map(i => `<td${row.regs[i] !== prev[i] ? ' style="font-weight:700"' : ' class="meta"'}>${hex(row.regs[i])}</td>`).join('');
    const wr = row.writes.map(([a, b, v]) => `${sym(a)}: ${hex(b, 2)}→${hex(v, 2)}`).join(', ');
    t += `<tr><td class="meta">${row.line}</td><td>${esc(row.text.trim().replace(/\s+/g, ' '))}</td>${regs}<td>${wr}</td></tr>`;
    prev = row.regs;
  }
  return t + '</table></div></details>';
}
(function () {
  const withTrace = matrix.filter(r => r.trace && r.trace.length);
  $('traces').innerHTML = withTrace.map(r => {
    const p = pathById[r.path];
    return `<details><summary><b>${p.kind}</b> / ${p.where} · ${r.code.length} instrukcí</summary>${traceTable(r).replace('<details open><summary>Trasa</summary>', '<div>').replace(/<\/details>$/, '</div>')}</details>`;
  }).join('');
})();

$('assume').innerHTML = DATA.meta.assumptions.map(a => `<li>${esc(a)}</li>`).join('');
</script>
</body>
</html>
"""

html = TEMPLATE.replace("__DATA__", json.dumps(data, ensure_ascii=False).replace("</", "<\\/"))
path = os.path.join(OUT, "report.html")
with open(path, "w") as f:
    f.write(html)
print(path)
