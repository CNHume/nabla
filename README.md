# nabla

`nabla` finds the **longest common subsequence (LCS)** of two input files.
It supports explicit choice among four LCS algorithms; and will select the fastest
algorithm automatically, based on analysis of the inputs.

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

`nabla` reads FASTA (`-F`) sequences, and emits SAM CIGAR alignments (`-G`);
so it works on nucleotide/protein sequences, in addition to text or records.

## Build

GNU make and a C++20 compiler:

```
make -C cli
```

The executable is `cli/build/nabla`.  A root-level `make` (or `make all`) builds
both variants: `cli/build/nabla` and `cli/build/nabla64`.

For inputs longer than 4,294,967,296 positions, a 64-bit variant is built, as follows:

```
make -C cli LENGTH_SIZE=64 BUILD=build64
```

which produces `cli/build64/nabla64`.  The 32-bit default handles up to 4,294,967,296 positions
(the human genome is ~3.1 billion); `nabla64` supports 64-bit positions, using twice the memory.

## Usage

```
nabla old.txt new.txt                  # line-by-line sequential output
nabla -t2 -W40 -y old.txt new.txt      # side-by-side parallel output
nabla -As -C -F a.fa b.fa              # FASTA genomes, linear space, matches
```

Run `nabla -h` for a usage synopsis, or read `man/nabla.1`.
Algorithm and performance details are presented in `Doc/`.

## License

LGPL-3.0-or-later.  See `LICENSE`.

## History

Originally maintained within [CNHume/Samples](https://github.com/CNHume/Samples) under `C++/LCS/`.
Beginning with v1.0.0 this repository is now the public home.
