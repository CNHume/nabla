# Bioinformatics Test Data

This directory holds genome-scale FASTA inputs used to exercise nabla's
`-F` (FASTA) path and to benchmark the algorithms.  The large data files are
generated locally and are **not** tracked by git (see `.gitignore`); the small
fixtures under `Test/` and the `makedata.cpp` generator are tracked.

## Sources

Two real, public-domain reference genomes:

| Local file | Sequence | FASTA header | Download |
|---|---|---|---|
| `chr21.fa.gz` | human chromosome 21, GRCh38 (≈ 46.7 Mb) | `>chr21` | <https://hgdownload.soe.ucsc.edu/goldenPath/hg38/chromosomes/chr21.fa.gz> |
| `ecoli_mg1655.fna` | *E. coli* str. K-12 MG1655, complete genome (≈ 4.64 Mb) | `>NC_000913.3 …` | <https://ftp.ncbi.nlm.nih.gov/genomes/all/GCF/000/005/845/GCF_000005845.2_ASM584v2/GCF_000005845.2_ASM584v2_genomic.fna.gz> |

If a URL moves, the accessions (`chr21` / `NC_000913.3`) are the authoritative
identifiers for locating the same sequence.

## Generating the derived files

`makedata.cpp` extracts a sequence from a FASTA (or plain) file and applies
reproducible (seeded) random substitutions:

```
g++ -std=c++20 -O2 -Wall -Wextra makedata.cpp -o makedata
./makedata ecoli_mg1655.fna ecoli_mg1655 100 1000 10000 999permil
```

This writes `ecoli_mg1655.seq` — the extracted sequence as a header-free plain
`.seq` (one line, uppercased) — and `ecoli_mg1655_variants.fna`, a multi-record
FASTA file: an unmutated `0` record followed by one record per argument.  Each
argument is a substitution count `K`, or a `Npermil` rate (N per mille identity,
e.g. `999permil` ≈ 99.9% identity).  Count records are named by their K; a
permil record is named by its rate.  Each mutant applies K distinct
substitutions — each changed to a *different* symbol drawn from the sequence's
own alphabet — and its defline records the source and the mutation parameters
(the substitution count and the xorshift32 seed).  The generator is reseeded
with the same fixed value for each argument, so every record is reproducible on
its own, a given input reproduces identical mutants on any platform, and each
larger K nests over the smaller K values.

## Naming convention

A `.seq` file is a header-free plain sequence; a `.fna` file is FASTA — a `>`
defline followed by the one-line sequence.  Files follow
`<name>[_<prefix>][_<mutant>]` with the matching extension:

- `<name>.seq` — the extracted sequence (one line, no defline).
- `<name>_16M.seq`, `_4M.seq`, `_100K.seq` — the first 16M / 4M / 100K symbols
  (a truncation of the `.seq`).
- `<name>_variants.fna` — the unmutated `0` record, one record per substitution
  count, and a `999permil` record (≈ 99.9% identity, about one symbol in 1,000
  changed), each defline recording the source and the mutation parameters.

`makedata.cpp` produces the base `.seq` and the `_variants.fna` file directly;
the prefix truncations are further simple steps over the base files.

## Example

```
nabla -As -C -F -q =100 ecoli_mg1655.seq ecoli_mg1655_variants.fna
```
