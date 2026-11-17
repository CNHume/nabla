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

#include <cerrno>                       // for errno
#include <cstdlib>                      // for strtoul
#include <format>                       // for format
#include <iostream>                     // for cerr
#include <locale>                       // for locale
#include <stdexcept>                    // for runtime_error
#include <string_view>                  // for string_view
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
  Fasta::Record ParseDefline(const string& bytes, size_t begin, size_t end) {
    while (end > begin && bytes[end - 1] == '\r')
      end--;
    Fasta::Record record;
    auto ws = begin;
    while (ws < end && bytes[ws] != ' ' && bytes[ws] != '\t')
      ws++;
    record.name.assign(bytes, begin, ws - begin);
    auto rest = ws;
    while (rest < end && (bytes[rest] == ' ' || bytes[rest] == '\t'))
      rest++;
    record.description.assign(bytes, rest, end - rest);
    return record;
  }

  //
  // Decode the selected record's byte span [begin, end) and strip whitespace,
  // so a sequence may span lines or contain spaces without affecting the
  // comparison.
  //
  u32string CollectSequence(string_view bytes, size_t begin, size_t end) {
    auto codePoints = Encoding::DecodeUtf8(bytes.substr(begin, end - begin));
    u32string sequence;
    sequence.reserve(codePoints.size());
    for (auto c : codePoints)
      if (c != U' ' && c != U'\t' && c != U'\r' && c != U'\n')
        sequence.push_back(c);
    return sequence;
  }
}

//
// A FASTA record_id identifies a record in one of two ways:
//
// If it is a numeric string, it is interpreted as a 1-based record index.
// Otherwise it will be interpreted as a record name.
//
// Record names may be preceded by an equal sign (=) to indicate that the
// subsequent string is to be interpreted as a record name, even if it may
// appear to be a numeric string.
//
// A record whose name is all digits is written -q =42 or -r =42, distinct
// from the 42nd record, -q 42 or -r 42.
//
void ParseRecordId(
  const std::string& id, std::string& name, std::uint32_t& index) {
  auto s = id.c_str();
  if (*s == '=') {
    name = id.substr(1);
    return;
  }
  //
  // Interpret an unsigned integer as a 1-based index; otherwise id is a record
  // name.  Reset errno before strtoul(): it sets errno only on failure and
  // leaves it unchanged on success, so a stale non-zero errno left by an
  // earlier call would otherwise make a valid numeric argument look like an
  // overflow error.
  //
  errno = 0;
  char* pEnd = nullptr;
  auto result = strtoul(s, &pEnd, 10);
  if (errno != 0 || pEnd == s || *pEnd != '\0')
    name = id;
  else
    index = (uint32_t)result;
}

Fasta::Fasta(
  const string& filename, string bytes,
  const string& recordName, uint32_t recordIndex) {
  filename_ = filename;
  auto n = bytes.size();

  //
  // First pass: collect every '>' defline and record its sequence span
  // [begin, end) in the byte buffer.  A record's sequence runs from just after
  // its header line to the start of the next header line (or the end of input).
  //
  struct Span { size_t begin = 0; size_t end = 0; };
  vector<Span> spans;
  size_t i = 0;
  while (i < n) {
    auto j = bytes.find('\n', i);
    if (j == string::npos)
      j = n;
    if (i < j && bytes[i] != '>') {     // sequence line
      if (!spans.empty())
        spans.back().end = j < n ? j + 1 : n;
    }
    else if (i < j) {                   // header line (bytes[i] == '>')
      if (!spans.empty())
        spans.back().end = i;
      records_.push_back(ParseDefline(bytes, i + 1, j));
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
    sequence_ = CollectSequence(bytes, 0, n);
  else
    sequence_ = CollectSequence(bytes, spans[selected_].begin, spans[selected_].end);
}

Fasta Fasta::Read(
  const string& filename, const string& recordName, uint32_t recordIndex) {
  auto bytes = Encoding::ReadBytes(filename);
  auto [encoding, bomLength] = Encoding::GetEncoding(bytes);
  if (Encoding::IsWide(encoding)) {
    // Wide encodings (UTF-16/32) cannot be scanned as UTF-8 bytes: decode the
    // whole file and re-encode to UTF-8 first.  Wide FASTA is rare, so the
    // whole-file expansion is acceptable there.
    auto codePoints = Encoding::DecodeCodePoints(bytes, encoding);
    bytes = Encoding::ToUtf8(codePoints);
  }
  else if (bomLength > 0) {
    bytes.erase(0, (size_t)bomLength);
  }
  return Fasta(filename, std::move(bytes), recordName, recordIndex);
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
