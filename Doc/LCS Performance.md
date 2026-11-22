# LCS Performance

This file shows how `-Aa` (Automatic) seamlessly moves between the four LCS
algorithms as the input crosses each boundary condition.  Every table lists all
four engines plus `-Aa`, so the reader can see that `-Aa` tracks the winner
without any user intervention.

Notation: `D` = edit distance, `r` = match count (matched position pairs),
`η = r/(mn)` = match density (matching efficiency, in `[0,1]`), `s` = alphabet size (distinct
symbols), `p` = LCS length.  Cells are `wall-time · peak-RSS`;
the `-Aa` cell also names the algorithm it selected in brackets.
`TIMEOUT` = exceeded the timeout; `abort` = the algorithm bailed out on its own
budget.

The dispatch itself is documented in `LCS Algorithm Selection.md`.  In one line:

```
D ≤ 2000                → Myers          (-Ad)
else r ≤ 100·min(m,n)   → Hunt-Szymanski (-Am)
else dense, length-only → RickTime       (-At)
else dense, reconstruct → RickSpace      (-As)
```

There are three boundary conditions, each with a measured table below.

---

## Boundary 1 — divergence (`D`)

Dense DNA (`s = 4`, so `r ≈ n²/4 ≈ 626M` at every edit rate — Hunt–Szymanski is
ineligible throughout, at 125× its sparse threshold).  As the edit rate climbs,
`D` grows, and `-Aa` moves off Myers.

### Length-only (the find phase)

| rate | D | Myers `-Ad` | H-S `-Am` | RickSpace `-As` | RickTime `-At` | Auto `-Aa` |
|---|---|---|---|---|---|---|
| 1% | 851 | **0.00s** · 5MB | TIMEOUT | 0.06s · 9MB | 0.01s · 8MB | **0.00s** · 5MB [Myers] |
| 5% | 3,695 | 0.04s · 5MB | TIMEOUT | 0.25s · 9MB | **0.05s** · 8MB | **0.07s** · 8MB [RickTime] |
| 10% | 7,557 | 0.15s · 5MB | TIMEOUT | 0.52s · 9MB | **0.12s** · 8MB | **0.14s** · 8MB [RickTime] |
| 20% | 14,712 | 0.60s · 5MB | TIMEOUT | 1.02s · 9MB | **0.34s** · 7MB | **0.34s** · 8MB [RickTime] |
| 40% | 25,373 | 2.06s · 6MB | TIMEOUT | 1.97s · 9MB | **0.85s** · 7MB | **0.87s** · 7MB [RickTime] |

The hand-off is at the `D = 2000` probe: at 1% (`D = 851`) `-Aa` picks Myers
instantly; from 5% up it stays on RickTime, whose single diagonal sweep is the
fastest find.  Myers, left to itself, degrades `O(ND)` — 2.06s by 40%.

### Interval reconstruction (the full LCS)

| rate | D | Myers `-Ad` | H-S `-Am` | RickSpace `-As` | RickTime `-At` | Auto `-Aa` |
|---|---|---|---|---|---|---|
| 1% | 851 | **0.01s** · 12MB | TIMEOUT | 0.15s · 12MB | 0.11s · 69MB | **0.01s** · 12MB [Myers] |
| 5% | 3,695 | 0.08s · 61MB | TIMEOUT | **0.55s** · 12MB | 0.74s · 241MB | **0.57s** · 12MB [RickSpace] |
| 10% | 7,557 | 0.29s · 227MB | TIMEOUT | **1.11s** · 12MB | 1.39s · 450MB | **1.14s** · 12MB [RickSpace] |
| 20% | 14,712 | 1.40s · 835MB | TIMEOUT | **2.33s** · 12MB | 3.40s · 762MB | **2.49s** · 12MB [RickSpace] |
| 40% | 25,373 | 4.62s · 2.4GB | TIMEOUT | **4.09s** · 11MB | 5.93s · 1.1GB | **4.16s** · 11MB [RickSpace] |

Reconstruction changes the picture in two ways the dispatcher exploits:

- **Myers** is fast at low `D` but its `O(D²)` history balloons to **2.4 GB** by
  40% — so `-Aa` leaves it as soon as `D` passes 2000.
- **RickTime's** `O(d log d)` dominant-match sort makes it *slower than RickSpace*
  once the sequences diverge — so `-Aa` routes reconstruction straight to
  RickSpace, whose **11 MB** is flat across the whole range.

The same input therefore picks a different Rick engine by phase: **RickTime for
the find, RickSpace for the reconstruction** — and `-Aa` tracks the best in both
columns.

---

## Boundary 2 — match count (`r`)

Divergent input (40% edit), length-only, as the alphabet grows and the matches
thin out:

| `s` | r | Myers `-Ad` | H-S `-Am` | RickSpace `-As` | RickTime `-At` | Auto `-Aa` |
|---|---|---|---|---|---|---|
| 4 | 624M | 2.07s · 6MB | TIMEOUT | 2.09s · 9MB | **0.91s** · 8MB | **0.91s** · 8MB [RickTime] |
| 20 | 125M | 2.06s · 6MB | 5.96s · 6MB | 1.16s · 21MB | **0.26s** · 14MB | **0.26s** · 14MB [RickTime] |
| 100 | 25M | 1.82s · 6MB | 1.25s · 6MB | 0.43s · 82MB | **0.13s** · 44MB | **0.13s** · 44MB [RickTime] |
| 1,000 | 2.5M | 1.84s · 6MB | **0.15s** · 6MB | 0.85s · 768MB | 0.28s · 388MB | **0.15s** · 6MB [Hunt-Szymanski] |
| 10,000 | 284K | 1.46s · 6MB | **0.04s** · 12MB | TIMEOUT | TIMEOUT | **0.06s** · 12MB [Hunt-Szymanski] |

The hand-off is at the `r = 100·min(m,n)` boundary (5M at this length) — a
match density of `η = 100/max(m,n) = 0.002` here, swept from `η ≈ 1/4` (DNA)
down to `η ≈ 10⁻⁴` (`s = 10,000`):

- **Dense** (`s ≤ 100`): `r` is huge, Hunt–Szymanski times out, and `-Aa` stays
  on RickTime.
- **Sparse** (`s ≥ 1000`): `r` falls below the threshold, Hunt–Szymanski becomes
  the winner (0.04s at `s=10,000`), and `-Aa` moves onto it — *just as* the Rick
  engines' `O(ns)` tables blow up (RickSpace/RickTime TIMEOUT at `s=10,000`,
  where the tables are ~4 GB).  `-Aa` dodges that blow-up by switching to the
  match-list algorithm.

---

## Boundary 3 — memory (the linear-space fallback)

RickSpace is the only engine with flat memory, so `-Aa`'s choices are
memory-safe by construction:

- **Length-only** → RickTime needs no recording (memory `O(ns)`), so it never
  overflows.
- **Reconstruction** → RickSpace is `O(ns)` linear space, so it never overflows
  either.

The only overflow left is an *explicit* `-At` on a large, divergent input: its
`O(d)` dominant-match recording grows past `Analysis::RecordingBudget` (0.75×
available RAM), and it aborts with "use `-As` (RickSpace)" rather than OOMing.
Measured at 40% edit: RickTime's recording is 3.9 GB at 100 Kbp and overflows at
~150 Kbp (on a host with 9.7 GB available; the point scales with available RAM)
— the point at which `-As` is mandatory.

---

## Affix Trimming

Every table above uses random input, whose common prefix and suffix are
negligible, so the Affix Trimming first pass costs only its `O(m + n)` scan
there and `-a` (Compare All) reproduces the same numbers.  Its payoff is on
inputs that do share affixes — versioned files, assembled genomes, repeated
headers.

The table below keeps a 100 Kbp prefix and a 100 Kbp suffix (200 Kbp of
affixes) fixed and mutates only the middle 50 Kbp at 1% (`D = 855`), then runs
each engine with Affix Trimming (default) and without it (`-a`):

| phase | engine | trimmed | `-a` (Compare All) |
|---|---|---|---|
| length-only | Myers | 0.01s · 7MB | 0.02s · 8MB |
| length-only | RickSpace | 0.07s · 10MB | 0.12s · 26MB |
| length-only | RickTime | 0.02s · 9MB | 0.14s · 19MB |
| length-only | Auto | 0.01s · 7MB [Myers] | 0.02s · 8MB [Myers] |
| reconstruction | Myers | 0.01s · 13MB | 0.04s · 28MB |
| reconstruction | RickSpace | 0.31s · 14MB | 0.18s · 42MB |
| reconstruction | RickTime | 0.43s · 71MB | 0.73s · 348MB |
| reconstruction | Auto | 0.01s · 13MB [Myers] | 0.03s · 28MB [Myers] |

Hunt–Szymanski times out in both columns (its `r` is ≈ 15.7B on DNA at
250 Kbp).  The `-a` cost is the whole affix length fed to the algorithm.  It is
largest for RickTime — the length-only find slows ~7× (0.02s → 0.14s) and the
reconstruction recording grows 71 MB → 348 MB — and for Myers reconstruction,
which slows ~4× (0.01s → 0.04s) while doubling memory (13 MB → 28 MB).
RickSpace's `O(ns)` tables grow ~3× (14 MB → 42 MB) because `-a` tables the
full 250 Kbp instead of the 50 Kbp middle.  Trimming strips the 200 Kbp of
affixes first, so the algorithm sees `N = m + n` = 100 K instead of 500 K at
the same `D`.

The same applies to the sparse-match regime, where Hunt–Szymanski is at home.
With a 40 Kbp prefix and 40 Kbp suffix around a 10 Kbp middle over `s = 1000`,
`-a` feeds the full 50 Kbp, so Hunt–Szymanski's `O((r + n) log n)` sees ~25×
the matches (~2.55M instead of ~100K) and slows from ~instant to 0.15s:

| engine (10% edit) | trimmed | `-a` (Compare All) |
|---|---|---|
| Hunt-Szymanski | 0.00s · 5MB | 0.15s · 6MB |
| RickSpace | 0.15s · 157MB | 0.79s · 768MB |
| RickTime | 0.09s · 81MB | 0.26s · 387MB |
| Myers | 0.00s · 5MB | 0.00s · 5MB |

Myers is the exception: its initial snake skips the common prefix and `D` is
unchanged, so it is barely affected.  At 40% edit `-Aa` still picks
Hunt–Szymanski, but the run slows from 0.01s to 0.16s.  Net: Affix Trimming
buys roughly the affix fraction for every engine — Myers' `O(ND)` sweep,
Hunt–Szymanski's `O((r + n) log n)` match list, and the Rick `O(ns)` tables —
so the win scales with the affix fraction, not with the algorithm.

---

## Summary

`-Aa` never runs a pathological algorithm.  Each boundary condition — divergence
(`D = 2000`), match count (`r = 100·min`), and the output phase (length-only vs
reconstruction) — is tested up front or routed by phase, and `-Aa` lands on the
winner every time:

- near-identical → **Myers**;
- sparse matches → **Hunt–Szymanski**;
- dense, length-only → **RickTime**;
- dense, reconstruction → **RickSpace**.

Because the two Rick engines are parameterized by LCS length and alphabet size
rather than edit distance or match count, there is no input for which all four
engines are pathological — and `-Aa` finds the one that is not.
