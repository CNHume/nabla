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
#include <string_view>                  // for u32string_view
#include <utility>                      // for move

using namespace std;
using namespace lcs;

namespace lcs::files {

Fasta::Fasta(const string& filename, u32string text) {
  filename_ = filename;
  sequence_.reserve(text.size());
  auto n = text.size();
  size_t i = 0;
  size_t records = 0;                   // '>' header lines seen
  while (i < n) {
    auto j = text.find(U'\n', i);
    if (j == u32string::npos)
      j = n;
    if (i < j && text[i] != U'>') {     // sequence line
      if (records < 2) {                // first record only
        for (auto k = i; k < j; k++) {
          auto c = text[k];
          if (U" \t\r"sv.find(c) == std::u32string_view::npos)
            sequence_.push_back(c);
        }
      }
    }
    else if (i < j) {                   // header line (text[i] == U'>')
      records++;
      //
      // Split the defline at its first whitespace run.  The identifier is
      // the first token; the description is the remainder.  Whitespace is
      // the only delimiter we may presuppose, so a pipe-delimited id such
      // as gi|123|ref|NC_000913.3| stays in name whole.
      //
      auto begin = i + 1;
      auto end = j;
      while (end > begin && text[end - 1] == U'\r')
        end--;
      auto defline = u32string(text.begin() + begin, text.begin() + end);
      Record record;
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
      records_.push_back(std::move(record));
    }

    // advance past the line terminator
    i = j < n ?
      j + 1 : n;
  }
}

void Fasta::Show() const {
  if (records_.empty()) {
    cerr << format("{}: No FASTA Header\n", filename_);
    return;
  }
  const auto& used = records_[0];
  if (used.description.empty())
    cerr << format(
      std::locale(), "{}: {:L} >{}\n",
      filename_, sequence_.size(), used.name);
  else
    cerr << format(
      std::locale(), "{}: {:L} >{} {}\n",
      filename_, sequence_.size(), used.name, used.description);
}

void Fasta::WarnExtra() const {
  for (size_t k = 1; k < records_.size(); ++k) {
    const auto& extra = records_[k];
    if (extra.description.empty())
      cerr << format(
        "Warning: Extra FASTA record in {}: {}\n",
        filename_, extra.name);
    else
      cerr << format(
        "Warning: Extra FASTA record in {}: {} {}\n",
        filename_, extra.name, extra.description);
  }
}

} // namespace lcs::files
