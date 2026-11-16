// makedata.cpp — Generate LCS benchmark data from a FASTA (or plain) sequence.
//
// Usage:
//   makedata <input.fa> <output-prefix> <K1> [K2 ...]
//
// Reads <input.fa>, extracts the sequence (strips '>' header lines, drops
// whitespace, uppercases) and writes it as <output-prefix>.seq — a header-free
// plain sequence (the .seq convention).  It also writes <output-prefix>_variants.fna,
// a multi-record FASTA file: an unmutated "0" record, then one record per K.
// Each mutant applies K distinct random substitutions — each changed to a
// different symbol drawn from the sequence's own alphabet — and is named by its
// K (an all-digit name, so -q =K selects it while -q K means the Kth record);
// its defline records the source file and the mutation parameters (the
// substitution count and the xorshift32 seed).
//
// The generator is reseeded with the same fixed value before every K, so each
// output is reproducible on its own from the parameters in its defline, and a
// given input reproduces identical mutants on any platform.  Because the
// position shuffle is likewise repeated, each larger K reuses the smaller K's
// positions and adds more: the mutants nest.
//
// Build (Linux):   g++ -std=c++20 -O2 -Wall -Wextra makedata.cpp -o makedata
// Build (Windows): cl /std:c++20 /O2 /EHsc makedata.cpp
//
#include <algorithm>                    // for sort, unique
#include <cctype>                       // for isspace, toupper
#include <cstdint>                      // for uint32_t
#include <cstdio>                       // for fprintf, snprintf
#include <cstdlib>                      // for exit, strtoull
#include <fstream>                      // for ifstream, ofstream
#include <ostream>                      // for ostream
#include <string>                       // for string, to_string
#include <vector>                       // for vector

using namespace std;

// Fixed xorshift32 seed.  Every mutant is generated from this same seed, so a
// defline's "seed=" and "subs=" values reproduce that exact file.
static const uint32_t kSeed = 0x12345678u;
static uint32_t rngState = kSeed;
static uint32_t NextRand() {
  uint32_t x = rngState;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return rngState = x;
}

//
// Extract the sequence from a FASTA (or plain) file: skip '>' header lines,
// drop whitespace, and uppercase.
//
static string ReadSequence(const string& path) {
  ifstream in(path, ios::binary);
  if (!in) {
    fprintf(stderr, "makedata: cannot open %s\n", path.c_str());
    exit(1);
  }
  string seq, line;
  while (getline(in, line)) {
    if (!line.empty() && line[0] == '>')
      continue;
    for (unsigned char c : line)
      if (!isspace(c))
        seq.push_back((char)toupper(c));
  }
  return seq;
}

// Write a header-free plain sequence on one line (the .seq convention).
static void WriteSequence(const string& path, const string& seq) {
  ofstream out(path, ios::binary);
  out << seq << '\n';
}

// Write one FASTA record to out: a '>' defline carrying the name and, when
// present, a space-separated description, followed by the sequence on one line.
static void WriteFastaRecord(ostream& out, const string& name,
                             const string& description, const string& seq) {
  out << '>' << name;
  if (!description.empty())
    out << ' ' << description;
  out << '\n' << seq << '\n';
}

int main(int argc, char** argv) {
  if (argc < 4) {
    fprintf(stderr, "usage: makedata <input.fa> <output-prefix> <K1> [K2 ...]\n");
    return 2;
  }

  const string source = argv[1];
  const string prefix = argv[2];
  auto seq = ReadSequence(source);

  // The base sequence, header-free: a plain copy of the source.
  WriteSequence(prefix + ".seq", seq);

  // The variants file: an unmutated "0" record, then one record per K.
  ofstream variants(prefix + "_variants.fna", ios::binary);
  if (!variants) {
    fprintf(stderr, "makedata: cannot create %s\n",
            (prefix + "_variants.fna").c_str());
    return 1;
  }
  WriteFastaRecord(variants, "0", "source=" + source, seq);

  // The sequence's own distinct symbols, used for substitutions.
  vector<char> symbols(seq.begin(), seq.end());
  sort(symbols.begin(), symbols.end());
  symbols.erase(unique(symbols.begin(), symbols.end()), symbols.end());

  auto n = seq.size();
  vector<size_t> pos(n);

  for (int a = 3; a < argc; a++) {
    auto K = (size_t)strtoull(argv[a], nullptr, 10);
    if (K > n)
      K = n;

    // Regenerate each mutant independently: the same seed and the same initial
    // permutation, so the defline's "seed=" and "subs=" reproduce it alone and
    // larger K values nest over smaller ones.
    rngState = kSeed;
    for (size_t i = 0; i < n; i++)
      pos[i] = i;

    // Fisher-Yates shuffle to select K distinct positions.
    for (size_t i = n - 1; i > 0; i--) {
      auto r = NextRand() % (i + 1);
      swap(pos[i], pos[r]);
    }

    auto mut = seq;
    for (size_t k = 0; k < K; k++) {
      auto p = pos[k];
      char c;
      do {
        c = symbols[NextRand() % symbols.size()];
      } while (c == mut[p]);
      mut[p] = c;
    }

    char seedBuf[16];
    snprintf(seedBuf, sizeof seedBuf, "0x%08lx", (unsigned long)kSeed);
    string description = "source=" + source + " subs=" + to_string(K) +
                         " seed=" + seedBuf;
    WriteFastaRecord(variants, to_string(K), description, mut);
  }
  return 0;
}
