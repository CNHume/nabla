// Copyright (C) 2017-2026, Christopher N. Hume.  All rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// 2026-10-21 CNHume  Created reconstruction regression tests.
//
// The reconstruction oracle asserts two independent properties:
//
//   1. The algorithm's reported LCS length matches an independent O(n*m)
//      dynamic-programming length for small inputs.
//   2. The reconstructed match trace is a genuine LCS: its runs are ordered,
//      match equal symbols, and sum to the reported length.  Together these
//      prove the trace is a valid common subsequence of maximum length.
//
// Property 2 is the stronger check: it is what catches a trace whose length
// is right but whose reconstruction is incomplete (the Pair destructor bug).
//
#include "doctest/doctest.h"
#include "modes/Text.h"
#include "files/Fasta.h"
#include "lcs/Delta.h"
#include "algorithms/Myers.h"
#include "modes/Record.h"

#include <algorithm>                    // for equal, max
#include <cstdint>                      // for uint32_t
#include <random>                       // for mt19937
#include <stdexcept>                    // for runtime_error
#include <string>
#include <vector>

using namespace lcs;

using namespace std;

namespace {
  const Algorithm algos[4] = {
    Algorithm::EditDistance,
    Algorithm::Matches,
    Algorithm::RickSpace,
    Algorithm::RickTime,
  };

  u32string toU32(const string& s) {
    return u32string(s.begin(), s.end());
  }

  // Reference O(n*m) DP LCS length: independent ground truth for small inputs.
  uint32_t dpLcsLength(const u32string& a, const u32string& b) {
    auto n = a.size();
    auto m = b.size();
    vector<vector<uint32_t>> d(n + 1, vector<uint32_t>(m + 1, 0));
    for (size_t i = 1; i <= n; ++i)
      for (size_t j = 1; j <= m; ++j)
        d[i][j] = a[i - 1] == b[j - 1] ?
          d[i - 1][j - 1] + 1 : max(d[i - 1][j], d[i][j - 1]);
    return d[n][m];
  }

  // Assert the coalesced match Deltas form a valid LCS trace of `length`.
  void checkTrace(const shared_ptr<Delta> deltas, uint32_t length,
                  const u32string& s1, const u32string& s2) {
    uint32_t sum = 0;
    uint32_t priorEnd1 = 0;
    uint32_t priorEnd2 = 0;
    for (auto d = deltas; d != nullptr; d = dynamic_pointer_cast<Delta>(d->next)) {
      auto len1 = d->end1 - d->begin1;
      auto len2 = d->end2 - d->begin2;
      REQUIRE(d->begin1 >= priorEnd1);
      REQUIRE(d->begin2 >= priorEnd2);
      REQUIRE(len1 == len2);
      REQUIRE(d->end1 <= s1.size());
      REQUIRE(d->end2 <= s2.size());
      REQUIRE(equal(
        s1.begin() + d->begin1, s1.begin() + d->end1,
        s2.begin() + d->begin2));
      sum += len1;
      priorEnd1 = d->end1;
      priorEnd2 = d->end2;
    }
    CHECK(sum == length);
  }

  // Run one (algorithm, s1, s2) case through the reconstruction oracle.
  void checkAlgo(Algorithm algo,
                 const u32string& s1, const u32string& s2,
                 uint32_t expected) {
    shared_ptr<Delta> deltas;
    auto length = modes::Text::Correspondence(
      &deltas, nullptr, nullptr, s1, s2, algo, false, false, false, false, 0, 0, 0);
    CAPTURE((int)algo);
    CHECK(length == expected);
    checkTrace(deltas, length, s1, s2);
  }
}

TEST_CASE("reconstruction is a valid LCS (fixed cases, DP cross-check)") {
  const vector<pair<string, string>> cases = {
    { "ACGTACGTACGT", "ACGTAGGTACGT" },  // the Pair destructor bug case
    { "ABCDEFGHIJ", "ABXDEFGHIJ" },      // clean substitution
    { "AAAAAA", "AAAAAA" },              // identical, maximal density
    { "AAAAAA", "BBBBBB" },              // disjoint
    { "AABB", "ABAB" },                  // dense
    { "A", "A" },
    { "A", "B" },
    { "", "" },
    { "", "A" },
    { "A", "" },
    { "ACGT", "ACGTACGT" },              // prefix
    { "TACGTACGT", "ACGTACGT" },         // shifted
    { "demanoeuvre", "reendeavour" },    // classic LCS example
    { "thisisatest", "testing123testing" },
  };
  for (auto& [s1, s2] : cases) {
    auto u1 = toU32(s1);
    auto u2 = toU32(s2);
    auto expected = dpLcsLength(u1, u2);
    for (auto algo : algos)
      checkAlgo(algo, u1, u2, expected);
  }
}

TEST_CASE("reconstruction valid on random low-alphabet strings") {
  mt19937 rng(12345);
  const string alphabets[] = { "ACGT", "AC", "AB" };
  for (int iter = 0; iter < 200; ++iter) {
    const string& ab = alphabets[rng() % 3];
    auto n1 = rng() % 25;
    auto n2 = rng() % 25;
    string s1, s2;
    for (auto i = 0u; i < n1; ++i) s1.push_back(ab[rng() % ab.size()]);
    for (auto i = 0u; i < n2; ++i) s2.push_back(ab[rng() % ab.size()]);
    auto u1 = toU32(s1);
    auto u2 = toU32(s2);
    auto expected = dpLcsLength(u1, u2);
    for (auto algo : algos)
      checkAlgo(algo, u1, u2, expected);
  }
}

TEST_CASE("algorithms agree on larger dense inputs") {
  mt19937 rng(999);
  const string ab = "ACGT";
  for (int iter = 0; iter < 30; ++iter) {
    auto n1 = 100 + rng() % 200;
    auto n2 = 100 + rng() % 200;
    string s1, s2;
    for (auto i = 0u; i < n1; ++i) s1.push_back(ab[rng() % 4]);
    for (auto i = 0u; i < n2; ++i) s2.push_back(ab[rng() % 4]);
    auto u1 = toU32(s1);
    auto u2 = toU32(s2);
    uint32_t reference = 0;
    for (int k = 0; k < 4; ++k) {
      shared_ptr<Delta> deltas;
      auto length = modes::Text::Correspondence(
        &deltas, nullptr, nullptr, u1, u2, algos[k], false, false, false, false, 0, 0, 0);
      if (k == 0)
        reference = length;
      else
        CHECK(length == reference);
      checkTrace(deltas, length, u1, u2);
    }
  }
}

TEST_CASE("string fixtures load and reconstruct validly") {
  auto u1 = modes::Text::Read(
    string(TEST_DATA_DIR) + "/Test/String/demanoeuvre.txt");
  auto u2 = modes::Text::Read(
    string(TEST_DATA_DIR) + "/Test/String/reendeavour.txt");
  CHECK(u1.size() == 11);
  CHECK(u2.size() == 11);
  auto expected = dpLcsLength(u1, u2);
  for (auto algo : algos)
    checkAlgo(algo, u1, u2, expected);
}

TEST_CASE("Myers aborts when the edit distance exceeds its budget") {
  // Disjoint alphabets drive the composition pre-filter's lower bound past
  // the memory-derived D budget, so Find() must throw instead of running the
  // O(D^2) sweep on a dissimilar input.
  u32string s1(1000000, U'A');
  u32string s2(1000000, U'B');
  CHECK_THROWS_AS(
    algorithms::Myers::Find(nullptr, nullptr, s1, s2),
    std::runtime_error);
}

TEST_CASE("Fasta keeps the first record and reports name/description") {
  // Single record with description: name and description split on whitespace.
  {
    files::Fasta fasta("chr21.fa", ">chr21 chromosome 21\nACGT\nACGT\n");
    REQUIRE(fasta.Records().size() == 1);
    CHECK(fasta.Records()[0].name == "chr21");
    CHECK(fasta.Records()[0].description == "chromosome 21");
    CHECK(fasta.Sequence() == toU32("ACGTACGT"));
  }
  // Pipe-delimited identifier: kept whole, since no delimiter is presupposed.
  {
    files::Fasta fasta(
      "ecoli.fa", ">gi|123|ref|NC_000913.3| Escherichia coli\nAC\n");
    REQUIRE(fasta.Records().size() == 1);
    CHECK(fasta.Records()[0].name == "gi|123|ref|NC_000913.3|");
    CHECK(fasta.Records()[0].description == "Escherichia coli");
    CHECK(fasta.Sequence() == toU32("AC"));
  }
  // Multi-record FASTA: only the first record is used; the second is ignored.
  {
    files::Fasta fasta("multi.fa", ">a\nAC\n>b desc\nGT\n");
    REQUIRE(fasta.Records().size() == 2);
    CHECK(fasta.Records()[0].name == "a");
    CHECK(fasta.Records()[0].description.empty());
    CHECK(fasta.Records()[1].name == "b");
    CHECK(fasta.Records()[1].description == "desc");
    CHECK(fasta.Sequence() == toU32("AC"));
  }
  // Three records: the vector holds all of them; the last two are ignored.
  {
    files::Fasta fasta("three.fa", ">a\nAC\n>b\nGT\n>c\nTT\n");
    REQUIRE(fasta.Records().size() == 3);
    CHECK(fasta.Records()[0].name == "a");
    CHECK(fasta.Records()[1].name == "b");
    CHECK(fasta.Records()[2].name == "c");
    CHECK(fasta.Sequence() == toU32("AC"));
  }
  // CRLF header: trailing CR is trimmed from the captured defline.
  {
    files::Fasta fasta("crlf.fa", ">chr21 desc\r\nACGT\n");
    REQUIRE(fasta.Records().size() == 1);
    CHECK(fasta.Records()[0].name == "chr21");
    CHECK(fasta.Records()[0].description == "desc");
    CHECK(fasta.Sequence() == toU32("ACGT"));
  }
  // No header: empty records vector, sequence passes through.
  {
    files::Fasta fasta("plain.fa", "ACGT\n");
    CHECK(fasta.Records().empty());
    CHECK(fasta.Sequence() == toU32("ACGT"));
  }
}

TEST_CASE("Fasta selects a record by index or name") {
  const string text = ">a\nAC\n>b desc\nGTAC\n>c\nTT\n";
  // 1-based index selects the third record.
  {
    files::Fasta fasta("multi.fa", text, {}, 3);
    REQUIRE(fasta.Records().size() == 3);
    CHECK(fasta.Sequence() == toU32("TT"));
  }
  // Name selects the matching record (exact identifier match).
  {
    files::Fasta fasta("multi.fa", text, "b");
    CHECK(fasta.Sequence() == toU32("GTAC"));
  }
  // Pipe-delimited name selects its record whole.
  {
    files::Fasta fasta(
      "ecoli.fa", ">a\nAC\n>gi|1|ref|NC|\nGT\n", "gi|1|ref|NC|");
    CHECK(fasta.Sequence() == toU32("GT"));
  }
  // Index out of range throws.
  {
    CHECK_THROWS_AS(
      (files::Fasta("multi.fa", text, {}, 4)), std::runtime_error);
  }
  // Name not found throws.
  {
    CHECK_THROWS_AS(
      (files::Fasta("multi.fa", text, "zzz")), std::runtime_error);
  }
  // Index zero is treated as "no index": the first record is used.
  {
    files::Fasta fasta("multi.fa", text, {}, 0);
    CHECK(fasta.Sequence() == toU32("AC"));
  }
}

TEST_CASE("record_id parses as a 1-based index or a record name") {
  //
  // A record_id is a numeric string -> 1-based index, otherwise a record name.
  // A leading '=' forces the name interpretation: "=42" names the record "42",
  // while "42" means the 42nd record.
  //
  auto parse = [](const char* id) {
    string name = "<unset>";
    uint32_t index = 0xDEADBEEFu;
    files::ParseRecordId(id, name, index);
    return pair<string, uint32_t>(name, index);
  };

  // Numeric strings are 1-based indices and leave the name untouched.
  CHECK(parse("1").second == 1);
  CHECK(parse("1").first == "<unset>");
  CHECK(parse("42").second == 42);
  CHECK(parse("4294967295").second == 4294967295u);

  // A leading '=' forces the name interpretation.
  CHECK(parse("=42").first == "42");
  CHECK(parse("=42").second == 0xDEADBEEFu);
  CHECK(parse("=100").first == "100");

  // Non-numeric strings are names and leave the index untouched.
  CHECK(parse("abc").first == "abc");
  CHECK(parse("abc").second == 0xDEADBEEFu);
  CHECK(parse("=abc").first == "abc");

  // An empty id is treated as a name (empty), not as index 0.
  CHECK(parse("").first == "");
  CHECK(parse("").second == 0xDEADBEEFu);
}

TEST_CASE("automatic dispatch returns a correct LCS") {
  // -Aa should select a working algorithm: the length and trace must match the
  // O(m*n) DP reference regardless of which algorithm it picks.
  const vector<pair<string, string>> cases = {
    { "ACGTACGTACGT", "ACGTAGGTACGT" },
    { "ABCDEFGHIJ", "ABXDEFGHIJ" },
    { "AAAAAA", "AAAAAA" },
    { "AAAAAA", "BBBBBB" },
    { "demanoeuvre", "reendeavour" },
  };
  for (auto& [s1, s2] : cases) {
    auto u1 = toU32(s1);
    auto u2 = toU32(s2);
    auto expected = dpLcsLength(u1, u2);
    shared_ptr<Delta> deltas;
    auto length = modes::Text::Correspondence(
      &deltas, nullptr, nullptr, u1, u2, Algorithm::Automatic,
      false, false, false, false, 0, 0, 0);
    CHECK(length == expected);
    checkTrace(deltas, length, u1, u2);
  }
}

TEST_CASE("Myers agrees with Hunt-Szymanski on the record path") {
  const vector<vector<string>> records1 = {
    { "alpha", "beta", "gamma", "delta" },
    { "a", "b", "c" },
    { "x", "x", "x" },
  };
  const vector<vector<string>> records2 = {
    { "alpha", "beta", "epsilon", "delta" },
    { "a", "b" },
    { "x", "y", "x" },
  };
  for (size_t i = 0; i < records1.size(); ++i) {
    const auto& r1 = records1[i];
    const auto& r2 = records2[i];
    // Reference length from Hunt-Szymanski (Matches), independent of loop order.
    shared_ptr<Delta> refDeltas;
    auto reference = modes::Record::Correspondence(
      &refDeltas, nullptr, nullptr, r1, r2, Algorithm::Matches,
      false, false, false, false, 0, 0, 0);
    for (auto algo : { Algorithm::EditDistance, Algorithm::Matches }) {
      shared_ptr<Delta> deltas;
      auto length = modes::Record::Correspondence(
        &deltas, nullptr, nullptr, r1, r2, algo, false, false, false, false, 0, 0, 0);
      CAPTURE((int)algo);
      CHECK(length == reference);
      // The trace must be a valid common subsequence summing to `length`.
      uint32_t sum = 0;
      for (auto d = deltas; d != nullptr; d = dynamic_pointer_cast<Delta>(d->next)) {
        auto len = d->end1 - d->begin1;
        REQUIRE(len == d->end2 - d->begin2);
        for (uint32_t k = 0; k < len; ++k)
          REQUIRE(r1[d->begin1 + k] == r2[d->begin2 + k]);
        sum += len;
      }
      CHECK(sum == length);
    }
  }
}

TEST_CASE("32-bit position boundary distinguishes nabla from nabla64") {
  //
  // The 32-bit build addresses positions [0, 2^32); a sequence of more than
  // 2^32 positions overflows Index, which is the crossover where nabla (32-bit)
  // would silently wrap and nabla64 becomes necessary.  This verifies the
  // boundary constant and ExceedsIndex(), the guard that makes the 32-bit build
  // fail loudly instead.
  //
#ifdef LENGTH_SIZE_64
  static_assert(sizeof(Index) == 8);
  CHECK(Index(-1) > 4'294'967'295ull);          // 64-bit index exceeds the 32-bit limit
#else
  static_assert(sizeof(Index) == 4);
  CHECK(Index(-1) == 4'294'967'295ull);          // 32-bit index tops out at 2^32 - 1
#endif

  CHECK(ExceedsIndex(0) == false);
#ifdef LENGTH_SIZE_64
  // No reachable boundary in the 64-bit build: these lengths all fit.
  CHECK(ExceedsIndex(4'294'967'296ull) == false);
  CHECK(ExceedsIndex(4'294'967'297ull) == false);
#else
  // 2^32 positions is the largest representable length; 2^32 + 1 overflows.
  CHECK(ExceedsIndex(4'294'967'296ull) == false);
  CHECK(ExceedsIndex(4'294'967'297ull) == true);
#endif
}
