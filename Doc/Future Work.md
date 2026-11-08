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

## Build

- **Ship both binaries in releases and packaging.**  The two-binary build is in
  place (`nabla` 32-bit default, `nabla64` via `LENGTH_SIZE=64`), but
  `.github/workflows/release.yml` and the Bioconda recipe still ship only the
  32-bit `nabla`.  Attach `nabla64` release assets (and, if desired, a `nabla64`
  Bioconda recipe) as a follow-up.

- **Drop the Bioconda recipe's `BUILD=` workaround at the next tag.**  The
  Makefile output-directory knob was renamed `BUILD` → `BUILD_DIR` in
  `cli/Makefile`, `Test/Makefile`, and the root `Makefile`, because
  conda-forge's compiler activation exports `BUILD` as the build-platform
  triplet (`x86_64-conda-linux-gnu`), which collided with `BUILD ?= build`.
  `conda/build.sh` still passes `BUILD=build` because `conda/meta.yaml` points
  at the v1.1.1 tarball, whose Makefile uses the old name — the two must change
  together.  When v1.1.2 is tagged, bump `conda/meta.yaml` (version + sha256)
  and simplify `build.sh` to `make -C cli` (32-bit) and
  `make -C cli LENGTH_SIZE=64 BUILD_DIR=build64` (64-bit), then re-sync
  `build.sh`/`meta.yaml` to the Bioconda fork.

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
