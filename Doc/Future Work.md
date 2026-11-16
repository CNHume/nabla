# Future Work

Deferred points and open questions, kept so they can be revisited later.
Each entry records the current decision and enough context to resume the
thread.

## Memory

- **Flatten the RickTime/RickSpace dense occurrence tables.**  The tables are
  `std::vector<std::vector<Index>>`; a single contiguous `std::vector<Index>`
  with stride indexing would remove ~48·s bytes of per-row vector/malloc
  overhead per table and collapse s allocations into one.  The byte savings are
  negligible relative to the table data (about 12/(m+n) of the footprint), so
  this is allocator hygiene, not a real memory lever.  Safe for long sequences.
  Low priority.

- **RickSpace `prev`/`prev1` tables — deliberately kept.**  Dropping the backward
  occurrence tables and deriving backward queries by binary-searching the sorted
  match lists would halve RickSpace's table memory at O(log n) instead of O(1)
  probes.  The time/memory tradeoff was made in favor of the O(1) LeftPos/TopPos
  tables.  Revisit only if table memory becomes the hard limit.

- **Myers linear-space reconstruction (intriguing).**  A middle-snake
  divide-and-conquer would reduce the full-reconstruction path from its O(D²)
  frontier history to O(D), at about 2× time.  The V history is already stored
  as `Index` (half the original `int64_t` cost); only the full path keeps every
  frontier, while the length-only probe is already O(D).

## Naming

- **Promote a shared `Match { row, column }` base struct.**  Myers and RickSpace
  each define `struct Match { row, column }` privately.  Hoist a shared `Match`,
  then reconcile `Pair`/`Delta`'s `begin1`/`begin2` (and `end1`/`end2`) naming
  toward `row`/`column`, as the CONTRIBUTING.md "Naming" section already
  anticipates.

## Analysis

- **Reconcile `Analysis::tableByteBudget` (fixed 1 GB) with `RecordingBudget`
  (RAM-derived, × 0.75).**  Intriguing.  Decide whether the dense-table warning
  should also follow available RAM, or document why it stays a fixed threshold.

## Algorithms

- **Bit-vector (word-level) LCS length engine (CIPR) — candidate.**  The
  Crochemore–Iliopoulos–Pinzon–Reid algorithm computes the LCS length in
  O(n·m/w) time and O(m/w) space (n ≥ m) by delta-encoding the DP columns as
  bit-vectors and updating each with a four-operation bit-parallel formula.
  It works over any finite alphabet and is most efficient when the shorter
  string fits in a machine word (m ≤ w), giving O(n) time.  O(n²/log n) is the
  word-RAM idealization; on real hardware (w = 64) it is a ~64× constant-factor
  speedup over the O(n·m) DP, not an asymptotic one.  Length-only; the trace is
  recovered via Hirschberg divide-and-conquer, still O(n·m/w).  It complements
  RickTime: RickTime is O(p·n) and wins when p is small, while CIPR wins when
  p ≈ n/2.  Reference: Crochemore, Iliopoulos, Pinzon, and Reid, "A fast and
  practical bit-vector algorithm for the longest common subsequence problem",
  Information Processing Letters, 80(6):279–285, 2001 (PDF in Doc/References/).
  Recommended by D. Hirschberg.  Provenance: the delta-encoding of a DP column
  plus the carry-by-addition trick descend from Myers' bit-vector edit-distance
  algorithm (reference [10] in the CIPR paper) — Myers, "A fast bit-vector
  algorithm for approximate string matching based on dynamic programming",
  J. ACM 46(3):395–415, 1999 (PDF in Doc/References/).  Myers encodes edit
  deltas Dv ∈ {-1, 0, +1} in two vectors and computes Xh = (((E&P) + P) ^ P) | E;
  CIPR specializes this to the LCS delta ΔL ∈ {0, 1} (one vector, four ops).
  CIPR notes that porting Myers to LCS "only adds extra overheads", hence the
  direct derivation.  Not a standalone candidate for nabla: it solves approximate
  substring matching, not exact global LCS.

- **Batch candidate-scoring (query vs. reference shortlist) — likely a separate
  tool over `lib/`, not a `nabla` flag.**  The contract differs from pairwise
  LCS — N references in, a ranked table plus top-k CIGAR out — and the new
  machinery (record cursor, top-k heap, directory handling, ranking output) is
  orchestration, not LCS.  So this should be a second thin executable (e.g.
  `nabla-score`) that loops `lib/`'s per-pair `Find` over candidates, leaving the
  single-pair `nabla` CLI untouched; `StripFasta` stays first-record-only and
  the cursor lives in the new tool.  The scoring is exact, not search: no database
  index, no seeding, no heuristics; file1 = reference shortlist, file2 = query,
  preserving the -G roles.  The query (file2) side is fixed, so its preprocessing
  is amortized across candidates: Hunt-Szymanski's `columnsMatchedByChar` (keyed
  on s2) reuses cleanly, Rick's `next2`/`id2` less so (shared union symbol-id map
  plus the shorter-first swap).  Score all candidates length-first, then
  reconstruct only the top-k; CIPR is the natural ranker (length only).  Input
  is one multi-FASTA file whose '>' headers name each reference, then the folder
  form (concatenate with injected per-file headers, sorted listing, extension
  filter).  Caveats: Rick's shorter-first swap must not disturb reference/query
  roles in CIGAR; and O(N · pairwise) bounds N to a shortlist, not a database.
  Build gate: a native scorer only pays when N (hundreds to thousands) makes
  rebuilt query indices cost more than the loop; for tens of candidates a shell
  loop over `nabla` already covers it.  Alternative worth considering before
  committing to a new binary: have `nabla` optionally emit its query-side
  preprocessed index (normalized query plus symbol map/occurrence tables) as an
  intermediate artifact that later invocations reload instead of rebuild, so the
  outer shell loop amortizes the same preprocessing without a long-lived scorer
  process — trading a serialize/reload round-trip for in-memory reuse.

## Build

- **Ship `nabla64` in the release workflow.**  The two-binary build is in place
  (`nabla` 32-bit default, `nabla64` via `LENGTH_SIZE=64`), and the Bioconda
  recipe now installs both, but `.github/workflows/release.yml` still attaches
  only the 32-bit `nabla`.  Attach the `nabla64` asset as a follow-up.

## Testing / Benchmarking

- **Boundary test: where `nabla` (32-bit) breaks and `nabla64` is required.**  A
  test demonstrating the ~4.29 G-position crossover.  A full >4.29 G input is
  impractical (~17 GB per string), so this likely means first adding a guard so
  the 32-bit build rejects (or clearly errors on) inputs exceeding `Index::max()`
  positions rather than silently wrapping, plus a test exercising that guard and
  a 64-bit-only check.

- **RSS comparison of `nabla` vs `nabla64`.**  Measure peak resident set size of
  both binaries on representative inputs, to quantify the 2× memory cost of
  64-bit positions and confirm the 32-bit default's memory advantage.
