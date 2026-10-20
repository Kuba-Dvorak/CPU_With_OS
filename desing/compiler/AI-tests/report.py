#!/usr/bin/env python3
"""Z vysledku testu (out/results.json, out/mutants.json) udela jeden HTML report: out/report.html"""
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
    "r": load("results.json"),
    "mutants": load("mutants.json"),
    "meta": {"commit": git_info(), "time": datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), "assumptions": assumptions()},
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
  --seq1: #cde2fb; --seq2: #86b6ef; --seq3: #3987e5; --seq4: #1c5cab;
}
@media (prefers-color-scheme: dark) {
  :root:where(:not([data-theme="light"])) {
    color-scheme: dark;
    --page: #0d0d0d; --surface: #1a1a19; --surface-2: #232321;
    --ink: #ffffff; --ink-2: #c3c2b7; --muted: #898781;
    --grid: #2c2c2a; --axis: #383835; --border: rgba(255,255,255,0.10);
    --s1: #3987e5; --s2: #d95926; --s3: #199e70; --none: #2c2c2a;
    --seq1: #104281; --seq2: #1c5cab; --seq3: #3987e5; --seq4: #86b6ef;
  }
}
:root[data-theme="dark"] {
  color-scheme: dark;
  --page: #0d0d0d; --surface: #1a1a19; --surface-2: #232321;
  --ink: #ffffff; --ink-2: #c3c2b7; --muted: #898781;
  --grid: #2c2c2a; --axis: #383835; --border: rgba(255,255,255,0.10);
  --s1: #3987e5; --s2: #d95926; --s3: #199e70; --none: #2c2c2a;
  --seq1: #104281; --seq2: #1c5cab; --seq3: #3987e5; --seq4: #86b6ef;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--page); color: var(--ink); font: 14px/1.5 system-ui, -apple-system, "Segoe UI", sans-serif; }
main { max-width: 1160px; margin: 0 auto; padding: 24px 16px 64px; }
h1 { font-size: 22px; margin: 0 0 4px; font-weight: 650; }
h2 { font-size: 16px; margin: 0 0 4px; font-weight: 650; }
h3 { font-size: 13px; margin: 14px 0 4px; font-weight: 600; }
.sub { color: var(--ink-2); margin: 0 0 12px; }
.meta { color: var(--muted); font-size: 12px; }
section { background: var(--surface); border: 1px solid var(--border); border-radius: 10px; padding: 18px; margin-top: 16px; }
code, .mono { font-family: ui-monospace, "JetBrains Mono", Menlo, monospace; font-size: 12.5px; }
nav { display: flex; flex-wrap: wrap; gap: 6px 14px; margin-top: 10px; font-size: 13px; }
nav a { color: var(--ink-2); }
.tiles { display: grid; grid-template-columns: repeat(auto-fit, minmax(175px, 1fr)); gap: 12px; margin-top: 16px; }
.tile { background: var(--surface); border: 1px solid var(--border); border-radius: 10px; padding: 14px 16px; }
.tile .label { color: var(--ink-2); font-size: 13px; display: flex; align-items: center; gap: 6px; }
.tile .value { font-size: 22px; font-weight: 650; margin-top: 2px; }
.tile .hint { color: var(--muted); font-size: 12px; }
.badge { display: inline-flex; align-items: center; gap: 4px; padding: 1px 8px; border-radius: 999px; font-size: 12px; font-weight: 600; white-space: nowrap; }
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
.hm { border-collapse: separate; border-spacing: 2px; }
.hm th { font-weight: 500; color: var(--ink-2); font-size: 12px; padding: 0 2px; text-align: center; white-space: nowrap; }
.hm th.grp { color: var(--ink); font-weight: 600; border-bottom: 1px solid var(--axis); padding-bottom: 2px; }
.hm td.rl { text-align: left; color: var(--ink-2); font-size: 12.5px; padding-right: 10px; white-space: nowrap; }
.hm tr.kind td { padding-top: 10px; font-weight: 600; color: var(--ink); font-size: 13px; }
.cell { min-width: 26px; height: 26px; border-radius: 4px; border: 0; text-align: center; font: 700 13px/26px system-ui, sans-serif; cursor: pointer; padding: 0 4px; }
.cell:hover, .cell.sel { outline: 2px solid var(--ink); outline-offset: 1px; }
.cell.wide { min-width: 70px; font-size: 12px; }
.c-pass { background: var(--good); color: var(--on-good); }
.c-warn { background: var(--warn); color: var(--on-warn); }
.c-fail { background: var(--bad); color: var(--on-bad); }
.c-none { background: var(--none); color: var(--muted); cursor: default; }
.gap { width: 10px; }
.legend { display: flex; flex-wrap: wrap; gap: 14px; color: var(--ink-2); font-size: 12.5px; margin: 8px 0 12px; }
.legend i { display: inline-block; width: 12px; height: 12px; border-radius: 3px; vertical-align: -2px; margin-right: 5px; }
.legend i.line { height: 2px; width: 16px; border-radius: 1px; vertical-align: 3px; }
.detail { margin-top: 14px; border-top: 1px solid var(--grid); padding-top: 14px; display: none; }
.detail.on { display: block; }
.asm { background: var(--surface-2); border-radius: 8px; padding: 10px 12px; margin: 8px 0; overflow: auto; max-height: 420px; }
.asm div { white-space: pre; }
.asm .ln { color: var(--muted); display: inline-block; width: 34px; }
.asm .lbl { color: var(--ink-2); font-weight: 600; margin-top: 4px; }
.asm .err { color: var(--bad); font-weight: 600; }
.asm .hit { background: rgba(250,178,25,.18); }
.cols { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 16px; }
.multiples { display: grid; grid-template-columns: repeat(auto-fit, minmax(240px, 1fr)); gap: 18px; }
.chart h3 { margin: 0; }
.chart p { margin: 0 0 4px; color: var(--muted); font-size: 12px; }
.chart svg, svg.frame { width: 100%; height: auto; display: block; overflow: visible; }
svg text { fill: var(--muted); font-size: 10px; font-variant-numeric: tabular-nums; }
svg text.ink { fill: var(--ink); }
#tip { position: fixed; pointer-events: none; background: var(--surface); color: var(--ink); border: 1px solid var(--border); border-radius: 8px;
  padding: 8px 10px; font-size: 12px; box-shadow: 0 4px 16px rgba(0,0,0,.18); display: none; z-index: 10; max-width: 360px; }
#tip .k { display: inline-block; width: 10px; height: 2px; vertical-align: 3px; margin-right: 6px; }
details { margin-top: 10px; }
summary { cursor: pointer; color: var(--ink-2); }
.dot { display: inline-block; width: 10px; height: 10px; border-radius: 50%; background: var(--s1); }
.dot.bad { background: var(--bad); }
ul.findings { margin: 8px 0 0; padding-left: 0; list-style: none; }
ul.findings li { padding: 8px 0; border-bottom: 1px solid var(--grid); display: flex; gap: 10px; align-items: flex-start; }
ul.findings li:last-child { border-bottom: 0; }
ul.assume { margin: 6px 0 0; padding-left: 18px; color: var(--ink-2); }
.trace td { white-space: nowrap; }
.tracebox { max-height: 460px; overflow: auto; }
</style>
</head>
<body>
<main>
  <h1>AI testy kompilátoru</h1>
  <p class="sub">Kód vygenerovaný z <code>memoryManagment/*</code> spuštěný v referenčním simulátoru ISA (<code>AI-tests/sim.hpp</code>). Kontroluje se celá RAM, registry i SP.</p>
  <div class="meta" id="meta"></div>
  <nav><a href="#s-find">Co testy našly</a><a href="#s-calls">Volání funkcí</a><a href="#s-paths">Cesty přes struktury</a><a href="#s-copy">copyFromTo</a><a href="#s-scopes">Scopy</a><a href="#s-cost">Cena kopírování</a><a href="#s-regs">Registry</a><a href="#s-mut">Mutační test</a><a href="#s-assume">Předpoklady</a></nav>
  <div class="tiles" id="tiles"></div>

  <section id="s-find">
    <h2>Co testy našly</h2>
    <p class="sub">Seskupeno podle důvodu. Selhání = kód udělá s pamětí, registry nebo skoky něco jiného, než má.</p>
    <ul class="findings" id="findings"></ul>
  </section>

  <section id="s-calls">
    <h2>Volání funkcí přes zásobník</h2>
    <p class="sub">Malé programy: skok přes těla funkcí, <code>main</code> s proměnnými a voláními, nakonec zápis značky. Každý běží pro tři počáteční SP (to, co nastaví OS). Klikni na buňku.</p>
    <div class="legend"><span><i class="c-pass"></i>✓ prošlo</span><span><i class="c-fail"></i>✗ selhalo</span></div>
    <div class="scroll"><table class="hm" id="callsHm"></table></div>
    <div class="detail" id="callsDetail"></div>
    <h3>SP během běhu (SP − SP na začátku, pro SP 0x1000)</h3>
    <p class="sub">Každé volání posune SP na rámec funkce a návrat ho má vrátit. Správný běh končí na 0.</p>
    <div class="multiples" id="spCharts"></div>
    <h3>Rámce funkcí (offset vůči SP uvnitř funkce)</h3>
    <div id="frames"></div>
  </section>

  <section id="s-paths">
    <h2>Cesty přes struktury a reference</h2>
    <p class="sub">Náhodné cesty typu <code>rb.rn.p.x</code> přes typy <code>Pair</code>, <code>Node</code>, <code>Box</code>, čtení (<code>variable::call</code>) i zápis (<code>variable::set</code>). Řádek = tvar cesty (<code>ref</code> = proměnná je reference, <code>.pole</code> = pole hodnotou, <code>.ref</code> = pole je reference). (R) = druhá strana má adresu v registru. Buňka = prošlo / celkem.</p>
    <div class="scroll"><table class="hm" id="pathsHm"></table></div>
    <div class="detail" id="pathsDetail"></div>
  </section>

  <section id="s-copy">
    <h2>copyFromTo: všechny kombinace</h2>
    <p class="sub">Řádek = co se kopíruje a kde je adresa (zásobník = offset vůči SP, R1/R2 = adresa v registru). Sloupec = SP × velikost. Při SP 0x40F8 leží data přes hranici xxFF → xx00.</p>
    <div class="scroll"><table class="hm" id="copyHm"></table></div>
    <div class="detail" id="copyDetail"></div>
    <p class="meta" id="setLocal"></p>
  </section>

  <section id="s-scopes">
    <h2>Scopy a přidělování paměti v rámci</h2>
    <p class="sub">Offsety, které <code>segmentManager</code> rozdá, a co pak najde <code>returnVar</code>. Nic se nespouští.</p>
    <div class="scroll" id="scopes"></div>
  </section>

  <section id="s-cost">
    <h2>Cena kopírování: počet instrukcí podle velikosti</h2>
    <p class="sub">Jedno přiřazení při SP 0x1000. Stejná osa Y ve všech grafech.</p>
    <div class="legend">
      <span><i class="line" style="background:var(--s1)"></i>zásobník → zásobník</span>
      <span><i class="line" style="background:var(--s2)"></i>R1 → zásobník</span>
      <span><i class="line" style="background:var(--s3)"></i>zásobník → R1</span>
    </div>
    <div class="multiples" id="costCharts"></div>
    <details><summary>Tabulka</summary><div class="scroll" id="costTable"></div></details>
  </section>

  <section id="s-regs">
    <h2>Které registry kód mění</h2>
    <p class="sub">copyFromTo, sjednoceno přes všechny testy. R3 je SP; kopírování ho měnit nesmí (byl by hlášen jako chyba).</p>
    <div class="scroll" id="regs"></div>
  </section>

  <section id="s-mut">
    <h2>Mutační test: jsou testy dost citlivé?</h2>
    <p class="sub">Z <code>memoryManagment/</code> se vždy smaže jeden řádek a testy se pustí znovu. Zabitý = aspoň jeden test dopadl jinak než u původního kódu.</p>
    <div id="mutants"></div>
  </section>

  <section id="s-assume">
    <h2>Předpoklady simulátoru</h2>
    <p class="sub">Načteno z hlavičky <code>AI-tests/sim.hpp</code>. Pokud něco nesedí s CPU, oprav to tam a testy se tím budou řídit.</p>
    <ul class="assume" id="assume"></ul>
  </section>
</main>
<div id="tip"></div>
<script>
const DATA = __DATA__;
const R = DATA.r, M = DATA.mutants;
const $ = id => document.getElementById(id);
const esc = s => String(s).replace(/[&<>"]/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c]));
const hex = (v, w = 4) => '0x' + (v & 0xFFFF).toString(16).toUpperCase().padStart(w, '0');
const ICON = {pass: '✓', warn: '!', fail: '✗'};
const WORD = {pass: 'prošlo', warn: 'varování', fail: 'selhalo'};
const badge = st => `<span class="badge b-${st}">${ICON[st] || ''} ${WORD[st] || st}</span>`;
const clean = t => String(t).trim().replace(/\s+/g, ' ');
$('meta').textContent = `commit ${DATA.meta.commit} · vygenerováno ${DATA.meta.time}`;

// ---------- tooltip
const tip = $('tip');
function showTip(html, x, y) {
  tip.innerHTML = html; tip.style.display = 'block';
  const w = tip.offsetWidth, h = tip.offsetHeight;
  tip.style.left = Math.min(x + 14, innerWidth - w - 8) + 'px';
  tip.style.top = Math.max(8, y - h - 10) + 'px';
}
const hideTip = () => tip.style.display = 'none';

// ---------- spolecny detail jednoho testu
function codeListing(r, labels = {}) {
  const errLines = new Set(r.errors.map(e => e.line));
  if (!r.code.length) return '<div class="asm mono"><div class="meta">(nic se nevygenerovalo)</div></div>';
  return '<div class="asm mono">' + r.code.map((c, i) =>
    (labels[i] ? `<div class="lbl">${esc(labels[i])}:</div>` : '') +
    `<div class="${errLines.has(i) ? 'err' : ''}"><span class="ln">${i}</span>${esc(clean(c))}</div>`).join('') + '</div>';
}
function traceTable(r) {
  if (!r.trace || !r.trace.length) return '';
  let t = `<details${r.trace.length < 40 ? ' open' : ''}><summary>Trasa (${r.trace.length} kroků)</summary><div class="tracebox"><table class="tbl mono trace"><tr><th>krok</th><th>řádek</th><th>instrukce</th>${[1,2,3,4,5].map(i => `<th>R${i}${i === 3 ? ' (SP)' : ''}</th>`).join('')}<th>zápisy do RAM</th></tr>`;
  t += `<tr><td class="meta">start</td><td></td><td></td>${[1,2,3,4,5].map(i => `<td>${hex(r.regsBefore[i])}</td>`).join('')}<td></td></tr>`;
  let prev = r.regsBefore;
  r.trace.forEach((row, k) => {
    const regs = [1,2,3,4,5].map(i => `<td${row.regs[i] !== prev[i] ? ' style="font-weight:700"' : ' class="meta"'}>${hex(row.regs[i])}</td>`).join('');
    const wr = row.writes.map(([a, b, v]) => `${hex(a)}: ${hex(b, 2)}→${hex(v, 2)}`).join(', ');
    t += `<tr><td class="meta">${k}</td><td class="meta">${row.pc}</td><td>${esc(clean(r.code[row.pc] || ''))}</td>${regs}<td>${wr}</td></tr>`;
    prev = row.regs;
  });
  return t + '</table></div></details>';
}
function diffTable(r) {
  if (!r.diffCount) return r.code.length && !r.errors.length && r.status !== 'fail' ? '<p class="meta">RAM po spuštění přesně odpovídá očekávání (porovnáno všech 65 536 bajtů).</p>' : '';
  return `<b>Rozdíly v RAM</b> <span class="meta">(${r.diffCount} B${r.diffCount > r.diffs.length ? ', zobrazeno ' + r.diffs.length : ''})</span>` +
    '<table class="tbl mono"><tr><th>adresa</th><th class="num">čekáno</th><th class="num">je</th><th></th></tr>' +
    r.diffs.map(d => `<tr><td>${hex(d.addr)}</td><td class="num">${hex(d.exp, 2)}</td><td class="num">${hex(d.act, 2)}</td><td class="meta">${d.inTarget ? 'v cíli' : 'MIMO cíl'}</td></tr>`).join('') + '</table>';
}
function renderResult(r, title, opts = {}) {
  let h = `<h2>${esc(title)}</h2><p class="sub">${badge(r.status)} ${esc(r.reason || '')}</p><p class="meta">${esc(r.layout || '')}</p>`;
  h += `<div class="cols"><div><b>Vygenerovaný kód</b> <span class="meta">(${r.code.length} instrukcí)</span>${codeListing(r, opts.labels)}`;
  h += r.errors.map(e => `<div style="color:var(--bad);font-weight:600">řádek ${e.line}: ${esc(e.msg)}</div>`).join('');
  h += `</div><div>${opts.side || ''}${diffTable(r)}</div></div>`;
  return h + traceTable(r);
}

// ---------- obecna heatmapa: rows = [{label, kind?}], cell(rowIdx, colIdx) -> {st, txt, tip, onClick}
function heatmap(table, groups, rows, cell) {
  let h = '<tr><th></th>' + groups.map(g => `<th class="gap"></th><th class="grp" colspan="${g.cols.length}">${esc(g.name)}</th>`).join('') + '</tr>';
  h += '<tr><th></th>' + groups.map(g => '<th class="gap"></th>' + g.cols.map(c => `<th>${esc(c)}</th>`).join('')).join('') + '</tr>';
  const handlers = [];
  const ncols = groups.reduce((a, g) => a + g.cols.length + 1, 1);
  rows.forEach((row, ri) => {
    if (row.kind) { h += `<tr class="kind"><td colspan="${ncols}">${esc(row.kind)}</td></tr>`; return; }
    h += `<tr><td class="rl">${esc(row.label)}</td>`;
    let ci = 0;
    groups.forEach(g => {
      h += '<td class="gap"></td>';
      g.cols.forEach(() => {
        const c = cell(ri, ci);
        if (!c) h += '<td><span class="cell c-none">·</span></td>';
        else { handlers.push(c); h += `<td><button class="cell c-${c.st}${c.wide ? ' wide' : ''}" data-h="${handlers.length - 1}">${c.txt}</button></td>`; }
        ci++;
      });
    });
    h += '</tr>';
  });
  table.innerHTML = h;
  table.addEventListener('click', e => {
    const b = e.target.closest('button'); if (!b) return;
    table.querySelectorAll('.cell.sel').forEach(x => x.classList.remove('sel'));
    b.classList.add('sel'); handlers[+b.dataset.h].onClick();
  });
  table.addEventListener('mousemove', e => {
    const b = e.target.closest('button'); if (!b) return hideTip();
    showTip(handlers[+b.dataset.h].tip, e.clientX, e.clientY);
  });
  table.addEventListener('mouseleave', hideTip);
}
const showDetail = (id, html) => { const d = $(id); d.innerHTML = html; d.classList.add('on'); };

// ---------- data
const copyRes = R.copy.filter(r => r.group === 'copyFromTo');
const setLocal = R.copy.filter(r => r.group === 'setRamLocal');
const cells = R.paths.cells;
const pathTotals = cells.reduce((a, c) => (a.pass += c.pass, a.fail += c.fail, a), {pass: 0, fail: 0});
const cfz = R.copyFuzz.reduce((a, f) => (a.runs += f.runs, a.fail += f.fail, a), {runs: 0, fail: 0});
const cnt = (v, st) => v.filter(r => r.status === st).length;

// ---------- tiles
(function () {
  const t = [
    ['Volání funkcí', `${cnt(R.calls, 'pass')} / ${R.calls.length}`, cnt(R.calls, 'fail') ? `${cnt(R.calls, 'fail')} selhalo` : 'vše prošlo', cnt(R.calls, 'fail') ? 'fail' : 'pass'],
    ['Cesty přes struktury', `${pathTotals.pass.toLocaleString('cs')} / ${(pathTotals.pass + pathTotals.fail).toLocaleString('cs')}`, pathTotals.fail ? `${pathTotals.fail} selhalo` : 'vše prošlo', pathTotals.fail ? 'fail' : 'pass'],
    ['copyFromTo', `${copyRes.length - cnt(copyRes, 'fail')} / ${copyRes.length}`, cnt(copyRes, 'fail') ? `${cnt(copyRes, 'fail')} selhalo` : 'vše prošlo', cnt(copyRes, 'fail') ? 'fail' : 'pass'],
    ['Fuzz copyFromTo', `${(cfz.runs - cfz.fail).toLocaleString('cs')} / ${cfz.runs.toLocaleString('cs')}`, cfz.fail ? `${cfz.fail} selhalo` : 'vše prošlo', cfz.fail ? 'fail' : 'pass'],
    ['Scopy', `${(R.scopes || []).filter(r => r.status !== 'fail').length} / ${(R.scopes || []).length}`, `${(R.scopes || []).filter(r => r.status === 'warn').length} varování`, (R.scopes || []).some(r => r.status === 'fail') ? 'fail' : ((R.scopes || []).some(r => r.status === 'warn') ? 'warn' : 'pass')],
    ['setRamLocal', `${setLocal.length - cnt(setLocal, 'fail')} / ${setLocal.length}`, cnt(setLocal, 'fail') ? 'selhání' : 'vše prošlo', cnt(setLocal, 'fail') ? 'fail' : 'pass'],
  ];
  if (M) {
    const k = M.mutants.filter(m => m.result === 'zabit').length, n = M.mutants.filter(m => m.result !== 'nekompiluje').length;
    t.push(['Mutanti zabiti', `${k} / ${n}`, `${n - k} přežilo`, k === n ? 'pass' : 'warn']);
  }
  $('tiles').innerHTML = t.map(([l, v, h, st]) => `<div class="tile"><div class="label"><span class="badge b-${st}">${ICON[st]}</span>${l}</div><div class="value">${v}</div><div class="hint">${h}</div></div>`).join('');
})();

// ---------- findings
(function () {
  const g = new Map();
  const add = (suite, reason, n, where, st = 'fail') => {
    const k = suite + '|' + reason;
    if (!g.has(k)) g.set(k, {suite, reason, n: 0, where: new Set(), st});
    const x = g.get(k); x.n += n; if (where) x.where.add(where);
  };
  for (const r of R.calls) if (r.status === 'fail') add('volání', r.reason, 1, r.row + ' (' + r.col + ')');
  for (const c of cells) for (const [reason, n] of c.reasons) add('cesty', reason, n, c.shape);
  for (const r of R.copy) if (r.status === 'fail') add(r.group, r.reason, 1, r.row);
  for (const r of R.scopes || []) if (r.status !== 'pass') add('scopy', r.reason, 1, r.layout, r.status);
  for (const f of R.copyFuzz) if (f.fail) add('fuzz copyFromTo', f.firstFail ? f.firstFail.reason : 'selhání', f.fail, f.row);
  const items = [...g.values()].sort((a, b) => (a.st === 'fail' ? 0 : 1) - (b.st === 'fail' ? 0 : 1) || b.n - a.n);
  $('findings').innerHTML = items.length ? items.map(x => {
    const w = [...x.where];
    return `<li>${badge(x.st)}<div><b>${esc(x.reason)}</b><br><span class="meta">${esc(x.suite)} · ${x.n}× · ${esc(w.slice(0, 5).join(', '))}${w.length > 5 ? ' … (' + w.length + ')' : ''}</span></div></li>`;
  }).join('') : `<li>${badge('pass')}<div>Nic. Všechny testy prošly.</div></li>`;
})();

// ---------- volani
(function () {
  const scen = [...new Set(R.calls.map(r => r.row))];
  const sps = [...new Set(R.calls.map(r => r.col))];
  const by = Object.fromEntries(R.calls.map(r => [r.row + '|' + r.col, r]));
  heatmap($('callsHm'), [{name: 'počáteční SP', cols: sps.map(s => s.replace('SP ', ''))}], scen.map(s => ({label: s})), (ri, ci) => {
    const r = by[scen[ri] + '|' + sps[ci]];
    return {st: r.status, txt: ICON[r.status], tip: `<b>${esc(r.row)}</b> · ${esc(r.col)}<br><code>${esc(r.extra.desc)}</code><br>${badge(r.status)} ${esc(r.reason)}`,
      onClick: () => showDetail('callsDetail', renderCall(r))};
  });
  function renderCall(r) {
    const e = r.extra;
    const labels = {0: 'skok přes těla funkcí'};
    for (const s of e.starts) labels[s.line] = s.fn === 'main' ? 'main' : 'funkce ' + s.fn;
    let side = `<b>Kontroly</b><table class="tbl mono"><tr><th>co</th><th class="num">čekáno</th><th class="num">je</th><th></th></tr>` +
      e.checks.map(c => `<tr><td style="font-family:system-ui">${esc(c.what)}</td><td class="num">${hex(c.exp)}</td><td class="num">${hex(c.act)}</td><td>${c.ok ? '✓' : '<b style="color:var(--bad)">✗</b>'}</td></tr>`).join('') +
      `<tr><td style="font-family:system-ui">SP po skončení</td><td class="num">${hex(e.sp0)}</td><td class="num">${hex(e.spEnd)}</td><td>${e.sp0 === e.spEnd ? '✓' : '<b style="color:var(--bad)">✗</b>'}</td></tr>` +
      `<tr><td style="font-family:system-ui">zápisy mimo zásobník</td><td class="num">0</td><td class="num">${e.outside}</td><td>${e.outside ? '<b style="color:var(--bad)">✗</b>' : '✓'}</td></tr></table>` +
      `<p class="meta">${e.steps} kroků, ${e.halted ? 'program doběhl' : 'program NEDOBĚHL'}.</p>`;
    if (!r.trace.length) side += '<p class="meta">Plná trasa je uložená jen pro SP 0x1000.</p>';
    return renderResult(r, `${r.row} · ${r.col}`, {labels, side});
  }
  // SP small multiples (jedna rada na graf, bez legendy)
  const first = scen.map(s => R.calls.find(r => r.row === s));
  const yMin = Math.min(0, ...first.map(r => Math.min(...r.extra.sp))), yMax = Math.max(1, ...first.map(r => Math.max(...r.extra.sp)));
  const top = Math.ceil(yMax / 8) * 8, bot = Math.floor(yMin / 8) * 8;
  const W = 260, H = 150, L = 30, Rr = 40, T = 8, B = 20;
  let html = '';
  first.forEach((r, k) => {
    const sp = r.extra.sp, n = Math.max(2, sp.length);
    const X = i => L + (W - L - Rr) * i / (n - 1), Y = v => T + (H - T - B) * (top - v) / (top - bot || 1);
    let s = `<svg viewBox="0 0 ${W} ${H}" data-k="${k}" role="img" aria-label="SP během ${esc(r.row)}">`;
    for (let v = bot; v <= top; v += 8) s += `<line x1="${L}" x2="${W - Rr}" y1="${Y(v)}" y2="${Y(v)}" stroke="var(${v ? '--grid' : '--axis'})" stroke-width="1"/><text x="${L - 5}" y="${Y(v) + 3}" text-anchor="end">${v > 0 ? '+' : ''}${v}</text>`;
    s += `<text x="${L}" y="${H - 5}">0</text><text x="${W - Rr}" y="${H - 5}" text-anchor="end">${sp.length} kroků</text>`;
    const pts = sp.map((v, i) => `${X(i)},${Y(v)}`).join(' ');
    s += `<polyline fill="none" stroke="var(--s1)" stroke-width="2" stroke-linejoin="round" stroke-linecap="round" points="${pts}"/>`;
    const end = sp[sp.length - 1] || 0, ok = end === 0;
    s += `<circle cx="${X(sp.length - 1)}" cy="${Y(end)}" r="4" fill="var(${ok ? '--s1' : '--bad'})" stroke="var(--surface)" stroke-width="2"/>`;
    s += `<text class="ink" x="${X(sp.length - 1) + 7}" y="${Y(end) + 3}">${ok ? '0 ✓' : (end > 0 ? '+' : '') + end + ' B ✗'}</text>`;
    s += `<line class="xh" x1="0" x2="0" y1="${T}" y2="${H - B}" stroke="var(--axis)" visibility="hidden"/><rect x="${L}" y="0" width="${W - L - Rr}" height="${H}" fill="transparent"/></svg>`;
    html += `<div class="chart"><h3>${esc(r.row)}</h3><p><code>${esc(r.extra.desc)}</code></p>${s}</div>`;
  });
  $('spCharts').innerHTML = html;
  $('spCharts').querySelectorAll('svg').forEach(svg => {
    const r = first[+svg.dataset.k], sp = r.extra.sp, pcs = r.extra.pcs, n = Math.max(2, sp.length), xh = svg.querySelector('.xh');
    svg.addEventListener('mousemove', e => {
      const b = svg.getBoundingClientRect(), sx = (e.clientX - b.left) * W / b.width;
      const i = Math.max(0, Math.min(sp.length - 1, Math.round((sx - L) / (W - L - Rr) * (n - 1))));
      const x = L + (W - L - Rr) * i / (n - 1);
      xh.setAttribute('x1', x); xh.setAttribute('x2', x); xh.setAttribute('visibility', 'visible');
      showTip(`<b>krok ${i}</b> · řádek ${pcs[i]}<br><code>${esc(clean(r.code[pcs[i]] || ''))}</code><br>SP − SP₀ = <b>${sp[i]}</b>`, e.clientX, e.clientY);
    });
    svg.addEventListener('mouseleave', () => { xh.setAttribute('visibility', 'hidden'); hideTip(); });
  });
  // ramce funkci
  const fr = new Map();
  for (const r of R.calls) for (const f of r.extra.frames) if (!fr.has(f.fn)) fr.set(f.fn, f);
  const fill = ['--seq2', '--seq3', '--seq1', '--seq4'];
  $('frames').innerHTML = [...fr.values()].map(f => {
    const total = Math.max(...f.slots.map(s => s.off + s.size)), px = 34, W = total * px + 2;
    let s = `<svg class="frame" viewBox="0 -14 ${W} 50" style="max-width:${W}px" role="img" aria-label="Rámec funkce ${esc(f.fn)}">`;
    f.slots.forEach((sl, i) => {
      const x = sl.off * px + 1, w = sl.size * px - 2;
      s += `<rect x="${x}" y="0" width="${w}" height="22" rx="4" fill="var(${fill[Math.min(i, 2)]})"/>`;
      s += `<text class="ink" x="${x + w / 2}" y="15" text-anchor="middle" style="font-size:11px">${esc(sl.name)}</text>`;
      s += `<text x="${x}" y="-4">SP+${sl.off}</text>`;
    });
    s += `<text x="${W - 1}" y="34" text-anchor="end">${total} B</text></svg>`;
    return `<div style="margin:6px 0 12px"><b class="mono">${esc(f.fn)}</b>${s}</div>`;
  }).join('');
})();

// ---------- cesty
(function () {
  const cols = R.paths.cols;
  const shapes = [...new Set(cells.map(c => c.shape))].sort((a, b) => a.split('.').length - b.split('.').length || a.localeCompare(b));
  const by = new Map(cells.map(c => [c.shape + '|' + c.col, c]));
  const rows = [];
  let depth = -1;
  for (const s of shapes) {
    const d = s.split('.').length - 1;
    if (d !== depth) { depth = d; rows.push({kind: d === 0 ? 'bez polí (celá proměnná)' : `hloubka ${d}`}); }
    rows.push({label: s});
  }
  heatmap($('pathsHm'), [{name: 'normálně', cols: cols.slice(0, 4)}, {name: 'druhá strana v registru', cols: cols.slice(4, 8).map(c => c.replace(' (R)', ''))}, {name: 'pointing', cols: ['zápis', 'v R']}], rows, (ri, ci) => {
    const row = rows[ri]; const c = by.get(row.label + '|' + ci); if (!c) return null;
    const st = c.fail ? 'fail' : 'pass';
    return {st, wide: true, txt: `${c.pass}/${c.pass + c.fail}`, tip: `<b>${esc(row.label)}</b> · ${esc(cols[ci])}<br>${c.pass} prošlo, ${c.fail} selhalo${c.reasons.length ? '<br>' + c.reasons.map(([r, n]) => esc(r) + ' (' + n + '×)').join('<br>') : ''}`,
      onClick: () => showDetail('pathsDetail', renderResult(c.example, `${row.label} · ${cols[ci]} · ${c.fail ? 'první selhání' : 'ukázka'}`))};
  });
})();

// ---------- copyFromTo
(function () {
  const rowsIds = [...new Set(copyRes.map(r => r.row))];
  const colsIds = [...new Set(copyRes.map(r => r.col))];
  const sps = [...new Set(colsIds.map(c => c.split(' | ')[0]))];
  const sizes = [...new Set(colsIds.map(c => c.split(' | ')[1]))];
  const by = new Map(copyRes.map(r => [r.row + '|' + r.col, r]));
  const fz = new Map(R.copyFuzz.map(f => [f.row, f]));
  const rows = [];
  let kind = '';
  for (const id of rowsIds) {
    const [k, loc] = id.split(' | ');
    if (k !== kind) { kind = k; rows.push({kind: k}); }
    rows.push({label: loc, id});
  }
  heatmap($('copyHm'), [...sps.map(s => ({name: s, cols: sizes.map(z => z.replace(' B', ''))})), {name: 'fuzz', cols: ['300×']}], rows, (ri, ci) => {
    const row = rows[ri];
    if (ci === sps.length * sizes.length) {
      const f = fz.get(row.id); const st = f.fail ? 'fail' : 'pass';
      return {st, wide: true, txt: f.fail ? '✗ ' + f.fail : '✓ ' + f.pass, tip: `<b>fuzz</b> ${esc(row.id)}<br>${f.pass} prošlo, ${f.fail} selhalo`,
        onClick: () => showDetail('copyDetail', f.firstFail ? renderResult(f.firstFail, `Fuzz, první selhání: ${row.id}`) : `<h2>Fuzz: ${esc(row.id)}</h2><p class="sub">${f.runs} náhodných běhů (velikost 1 až 32 B, náhodný SP a offsety), všechny prošly.</p>`)};
    }
    const col = sps[Math.floor(ci / sizes.length)] + ' | ' + sizes[ci % sizes.length];
    const r = by.get(row.id + '|' + col);
    return {st: r.status, txt: ICON[r.status], tip: `<b>${esc(row.id)}</b><br>${esc(col)} · ${r.code.length} instrukcí<br>${badge(r.status)} ${esc(r.reason)}`,
      onClick: () => showDetail('copyDetail', renderResult(r, `${row.id} · ${col}`))};
  });
  const bad = setLocal.filter(r => r.status === 'fail');
  $('setLocal').innerHTML = `setRamLocal: ${setLocal.length - bad.length} z ${setLocal.length} testů prošlo (velikosti 1 až 4 B, náhodné SP a offsety, hranice rozsahu).` +
    (bad.length ? ' Selhání: ' + bad.map(r => esc(r.layout + ': ' + r.reason)).join('; ') : '');
})();

// ---------- scopy
(function () {
  const sc = R.scopes || [];
  $('scopes').innerHTML = '<table class="tbl"><tr><th>kontrola</th><th>výsledek</th><th>detail</th></tr>' +
    sc.map(r => `<tr><td>${esc(r.row)}</td><td>${badge(r.status)}</td><td class="meta">${esc(r.layout)}</td></tr>`).join('') + '</table>';
})();

// ---------- cena kopirovani
(function () {
  const kinds = [...new Set(copyRes.map(r => r.row.split(' | ')[0]))];
  const series = [['zásobník → zásobník', 'var(--s1)'], ['R1 → zásobník', 'var(--s2)'], ['zásobník → R1', 'var(--s3)']];
  const base = copyRes.filter(r => r.col.startsWith('SP 0x1000'));
  const sizes = [...new Set(base.map(r => parseInt(r.col.split(' | ')[1])))].sort((a, b) => a - b);
  const val = (k, loc, s) => { const r = base.find(r => r.row === k + ' | ' + loc && parseInt(r.col.split(' | ')[1]) === s); return r && r.code.length && r.status !== 'fail' ? r.code.length : null; };
  const all = kinds.flatMap(k => series.flatMap(([loc]) => sizes.map(s => val(k, loc, s)))).filter(v => v !== null);
  const yMax = Math.max(1, ...all), step = yMax > 40 ? 20 : 10, top = Math.ceil(yMax / step) * step;
  const W = 260, H = 160, L = 30, Rr = 8, T = 8, B = 22, xMax = sizes[sizes.length - 1];
  const X = s => L + (W - L - Rr) * (s - 1) / (xMax - 1), Y = v => T + (H - T - B) * (1 - v / top);
  let html = '', table = '<table class="tbl"><tr><th>kopírování</th><th>umístění</th>' + sizes.map(s => `<th class="num">${s} B</th>`).join('') + '</tr>';
  kinds.forEach((k, ki) => {
    let s = `<svg viewBox="0 0 ${W} ${H}" data-k="${ki}" role="img" aria-label="Počet instrukcí: ${esc(k)}">`;
    for (let v = 0; v <= top; v += step) s += `<line x1="${L}" x2="${W - Rr}" y1="${Y(v)}" y2="${Y(v)}" stroke="var(${v ? '--grid' : '--axis'})"/><text x="${L - 5}" y="${Y(v) + 3}" text-anchor="end">${v}</text>`;
    for (const z of sizes) s += `<text x="${X(z)}" y="${H - 6}" text-anchor="middle">${z}</text>`;
    const missing = [];
    for (const [loc, col] of series) {
      const pts = sizes.map(z => [z, val(k, loc, z)]).filter(p => p[1] !== null);
      table += `<tr><td>${esc(k)}</td><td>${esc(loc)}</td>${sizes.map(z => `<td class="num">${val(k, loc, z) ?? '—'}</td>`).join('')}</tr>`;
      if (!pts.length) { missing.push(loc); continue; }
      s += `<polyline fill="none" stroke="${col}" stroke-width="2" stroke-linejoin="round" stroke-linecap="round" points="${pts.map(([z, v]) => X(z) + ',' + Y(v)).join(' ')}"/>`;
      const [lz, lv] = pts[pts.length - 1];
      s += `<circle cx="${X(lz)}" cy="${Y(lv)}" r="4" fill="${col}" stroke="var(--surface)" stroke-width="2"/>`;
    }
    s += '</svg>';
    const lines = series.map(([loc]) => sizes.map(z => val(k, loc, z)).join());
    const same = [];
    for (let a = 0; a < 3; a++) for (let b = a + 1; b < 3; b++) if (lines[a] === lines[b] && !missing.includes(series[a][0])) same.push(series[a][0] + ' = ' + series[b][0]);
    const max = Math.max(0, ...sizes.map(z => Math.max(...series.map(([loc]) => val(k, loc, z) || 0))));
    html += `<div class="chart"><h3>${esc(k)}</h3><p>při 16 B až ${max} instrukcí${same.length ? ' · překrývá se: ' + same.join(', ') : ''}${missing.length ? ' · nevygenerováno: ' + missing.join(', ') : ''}</p>${s}</div>`;
  });
  $('costCharts').innerHTML = html;
  $('costTable').innerHTML = table + '</table>';
})();

// ---------- registry
(function () {
  const rowsIds = [...new Set(copyRes.map(r => r.row))];
  let t = '<table class="tbl"><tr><th>kopírování</th>' + [1, 2, 3, 4, 5].map(i => `<th class="num">R${i}${i === 3 ? ' (SP)' : ''}</th>`).join('') + '</tr>';
  for (const id of rowsIds) {
    const rs = copyRes.filter(r => r.row === id && r.code.length);
    if (!rs.length) continue;
    const w = new Set(rs.flatMap(r => r.regsWritten));
    t += `<tr><td>${esc(id)}</td>` + [1, 2, 3, 4, 5].map(i => `<td class="num">${w.has(i) ? `<span class="dot${i === 3 ? ' bad' : ''}" title="zapisuje"></span>` : '<span class="meta">·</span>'}</td>`).join('') + '</tr>';
  }
  $('regs').innerHTML = t + '</table>';
})();

// ---------- mutanti
(function () {
  if (!M) { $('mutants').innerHTML = '<p class="meta">Nespuštěno. Pusť <code>./run.sh --mutace</code>.</p>'; return; }
  const st = m => m.result === 'zabit' ? 'pass' : (m.result === 'prezil' ? 'fail' : 'none');
  const word = m => m.result === 'zabit' ? (m.crash ? 'zabit (testy spadly)' : 'zabit') : (m.result === 'prezil' ? 'přežil' : 'nekompiluje');
  const order = {prezil: 0, zabit: 1, nekompiluje: 2};
  const ms = [...M.mutants].sort((a, b) => order[a.result] - order[b.result] || a.file.localeCompare(b.file) || a.line - b.line);
  const per = M.files.map(f => { const x = M.mutants.filter(m => m.file === f && m.result !== 'nekompiluje'); return `${f}: ${x.filter(m => m.result === 'zabit').length}/${x.length}`; });
  $('mutants').innerHTML = `<p class="meta">Zabito podle souboru: ${per.join(' · ')}</p><div class="scroll"><table class="tbl"><tr><th>soubor</th><th class="num">řádek</th><th>smazaný kód</th><th>druh</th><th>výsledek</th></tr>` +
    ms.map(m => `<tr><td class="mono">${m.file}</td><td class="num">${m.line}</td><td class="mono">${esc(m.code)}</td><td>${m.kind}</td><td><span class="badge b-${st(m)}">${word(m)}</span></td></tr>`).join('') + '</table></div>';
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
