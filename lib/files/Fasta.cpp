// Copyright (C) 2017-2026, Christopher N. Hume.  All rights reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 3 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, see <https://www.gnu.org/licenses/>.
//
// 2026-11-15 CNHume  Created Fasta class
//
#include "files/Fasta.h"
#include "files/Encoding.h"

#include <format>                       // for format
#include <iostream>                     // for cerr
#include <locale>                       // for locale
#include <stdexcept>                    // for runtime_error
#include <string_view>                  // for u32string_view
#include <utility>                      // for move

using namespace std;
using namespace lcs;

namespace lcs::files {

namespace {
  //
  // Parse one '>' defline at [begin, end) into its name (the first
  // whitespace-delimited token) and description (the remainder).  The name is
  // not subdivided further, so a pipe-delimited id such as gi|...|ref|... is
  // kept whole.
  //
  Fasta::Record ParseDefline(const u32string& text, size_t begin, size_t end) {
    while (end > begin && text[end - 1] == U'\r')
      end--;
    auto defline = u32string(text.begin() + begin, text.begin() + end);
    Fasta::Record record;
    auto ws = defline.find_first_of(U" \t");
    u32string name, description;
    if (ws == u32string::npos) {
      name = defline;
    }
    else {
      name = defline.substr(0, ws);
      auto rest = ws;
      while (
        rest < defline.size() &&
        (defline[rest] == U' ' || defline[rest] == U'\t'))
        rest++;
      description = defline.substr(rest);
    }
    record.name = Encoding::ToUtf8(name);
    record.description = Encoding::ToUtf8(description);
    return record;
  }

  //
  // Collect the sequence lines in [begin, end), skipping '>' header lines and
  // stripping whitespace, into one unbroken sequence.
  //
  u32string CollectSequence(const u32string& text, size_t begin, size_t end) {
    u32string sequence;
    sequence.reserve(end - begin);
    size_t i = begin;
    while (i < end) {
      auto j = text.find(U'\n', i);
      if (j == u32string::npos || j > end)
        j = end;
      if (i < j && text[i] != U'>') {   // sequence line
        for (auto k = i; k < j; k++) {
          auto c = text[k];
          if (U" \t\r"sv.find(c) == std::u32string_view::npos)
            sequence.push_back(c);
        }
      }
      i = j < end ? j + 1 : end;
    }
    return sequence;
  }
}

Fasta::Fasta(
  const string& filename, u32string text,
  const string& recordName, uint32_t recordIndex) {
  filename_ = filename;
  auto n = text.size();

  //
  // First pass: collect every '>' defline and record its sequence span
  // [begin, end) in the text.  A record's sequence runs from just after its
  // header line to the start of the next header line (or the end of text).
  //
  struct Span { size_t begin = 0; size_t end = 0; };
  vector<Span> spans;
  size_t i = 0;
  while (i < n) {
    auto j = text.find(U'\n', i);
    if (j == u32string::npos)
      j = n;
    if (i < j && text[i] != U'>') {     // sequence line
      if (!spans.empty())
        spans.back().end = j < n ? j + 1 : n;
    }
    else if (i < j) {                   // header line (text[i] == U'>')
      // Close the previous record's span at the start of this header line.
      if (!spans.empty())
        spans.back().end = i;
      records_.push_back(ParseDefline(text, i + 1, j));
      spans.push_back({ j < n ? j + 1 : n, n });
    }
    i = j < n ? j + 1 : n;
  }

  //
  // Select the record to use.
  //
  if (recordIndex != 0) {
    if (recordIndex > records_.size())
      throw runtime_error(format(
        "{}: FASTA record index {} is out of range ({} record{})",
        filename_, recordIndex, records_.size(),
        records_.size() == 1 ? "" : "s"));
    selected_ = recordIndex - 1;
  }
  else if (!recordName.empty()) {
    auto found = false;
    for (size_t k = 0; k < records_.size(); ++k) {
      if (records_[k].name == recordName) {
        selected_ = k;
        found = true;
        break;
      }
    }
    if (!found) {
      auto names = string();
      for (size_t k = 0; k < records_.size(); ++k) {
        if (k > 0)
          names += ", ";
        names += records_[k].name;
      }
      throw runtime_error(format(
        "{}: FASTA record '{}' not found; available: {}",
        filename_, recordName,
        names.empty() ? string("(none)") : names));
    }
  }

  //
  // Collect the selected record's sequence.  A file with no '>' header has no
  // records and is used whole when no selection was requested.
  //
  if (records_.empty())
    sequence_ = CollectSequence(text, 0, n);
  else
    sequence_ = CollectSequence(text, spans[selected_].begin, spans[selected_].end);
}

void Fasta::Show() const {
  if (records_.empty()) {
    cerr << format(
      std::locale(), "{}: {:L} No FASTA Header\n",
      filename_, sequence_.size());
    return;
  }
  const auto& used = records_[selected_];
  if (used.description.empty())
    cerr << format(
      std::locale(), "{}: {:L} >{}\n",
      filename_, sequence_.size(), used.name);
  else
    cerr << format(
      std::locale(), "{}: {:L} >{} {}\n",
      filename_, sequence_.size(), used.name, used.description);
}

void Fasta::WarnIgnored() const {
  for (size_t k = 0; k < records_.size(); ++k) {
    if (k == selected_)
      continue;
    const auto& ignored = records_[k];
    if (ignored.description.empty())
      cerr << format(
        "Warning: Ignored FASTA record in {}: {}\n",
        filename_, ignored.name);
    else
      cerr << format(
        "Warning: Ignored FASTA record in {}: {} {}\n",
        filename_, ignored.name, ignored.description);
  }
}

} // namespace lcs::files
