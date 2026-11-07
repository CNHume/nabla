# nabla

`nabla` finds the **longest common subsequence (LCS)** of two sequences or
files. It ships four LCS algorithms plus an automatic dispatcher that selects
the fastest one for each input.

| Algorithm | Switch | Regime |
|---|---|---|
| Myers (1986) | `-Ad` | near-identical input (small edit distance `D`) |
| Hunt-Szymanski (1977) | `-Am` | sparse matches (small match count `r`) |
| RickSpace (2000) | `-As` | linear-space fallback, never exhausts memory |
| RickTime (1995) | `-At` | dense match count, fastest find |
| Automatic | `-Aa` | the default; probes and routes to the winner |

Automatic dispatch: `D ≤ 2000 → Myers`; else `r ≤ 100·min(m,n) →
Hunt-Szymanski`; else dense and length-only → RickTime; else dense and
reconstruction → RickSpace.

`nabla` reads FASTA (`-F`) and emits SAM CIGAR alignments (`-G`), so it works
directly on nucleotide/protein sequences as well as text and records.

## Build

GNU make and a C++20 compiler:

```
make -C cli
```

The executable is `cli/build/nabla`.

## Usage

```
nabla old.txt new.txt                  # line-by-line diff
nabla -yp -W 40 -t 2 old.txt new.txt   # side-by-side (parallel) output
nabla -As -C -F a.fa b.fa              # FASTA genomes, linear space, matches
```

Run `nabla -h` for the full option list, or read `man/nabla.1`. Algorithm and
performance details are in `Doc/`.

## License

LGPL-3.0-or-later; see `LICENSE`.

## History

Developed in [CNHume/Samples](https://github.com/CNHume/Samples) under
`C++/LCS/`; this repository is the promoted public home starting at v1.0.0.
