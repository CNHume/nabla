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
// 2018-05-11 CNHume  Added word switch
// 2017-07-09 CNHume  Created File class
//
#include "files/File.h"
#include "files/Encoding.h"
#include "formatters/Edit.h"
#include "formatters/RecordDriver.h"
#include "formatters/TextDriver.h"
#include "modes/Record.h"
#include "modes/Text.h"

#include <format>                       // for format
#include <iostream>                     // for cerr

using namespace std;
using namespace lcs;
using namespace lcs::files;

namespace {
  //
  // Report one file argument's FASTA records.  The first (used) defline is
  // listed under -v, as are the deflines of any additional records, which are
  // ignored.  Whenever -F is given and a file holds extra records, a warning
  // is emitted.
  //
  void ReportFastaHeader(const string& filename, const modes::Text::FastaInfo& info, bool isverbose) {
    if (info.recordCount == 0) {
      if (isverbose)
        cerr << format("{}: (no FASTA header)\n", filename);
      return;
    }
    if (isverbose)
      cerr << format("{}: >{}\n", filename, Encoding::ToUtf8(info.header));
    if (info.recordCount > 1) {
      cerr << format("Warning: Additional FASTA record ignored\n");
      if (isverbose)
        for (const auto& defline : info.ignored)
          cerr << format("{}: >{}\n", filename, Encoding::ToUtf8(defline));
    }
  }

  //
  // Strip FASTA headers from one text file, keeping only the first record.
  // The record count is needed for the warning even without -v, so info is
  // always collected.
  //
  u32string PrepareFasta(const string& filename, const u32string& raw, bool isverbose) {
    modes::Text::FastaInfo info;
    auto text = modes::Text::StripFasta(raw, &info);
    ReportFastaHeader(filename, info, isverbose);
    return text;
  }
}

Length File::Correspondence(
  shared_ptr<Delta>* intervals, metrics::Statistics* statistics,
  metrics::Progress* progress,
  const Options& options) {
  Length length(0);
  auto format = formatters::Formatter::Make(
    options.format, options.width, options.tabsize);
  if (options.istext) {
    u32string s1, s2;
    if (options.isquote) {
      // Treat filename arguments as literal strings.
      s1 = Encoding::DecodeUtf8(options.file1);
      s2 = Encoding::DecodeUtf8(options.file2);
    }
    else {
      s1 = modes::Text::Read(options.file1);
      s2 = modes::Text::Read(options.file2);
      if (options.isfasta) {
        s1 = PrepareFasta(options.file1, s1, options.isverbose);
        s2 = PrepareFasta(options.file2, s2, options.isverbose);
      }
    }
    length = modes::Text::Correspondence(
      intervals, statistics, progress, s1, s2,
      options.algorithm, options.ignorecase, options.ignorespace,
      options.isjoin, options.isverbose,
      options.join, options.prefix, options.suffix);
    if (intervals != nullptr)
      formatters::TextDriver::Show(
        *format, *intervals, s1, s2, options.file1, options.file2);
  }
  else {
    auto r1 = modes::Record::Read(options.file1, options.isword);
    auto r2 = modes::Record::Read(options.file2, options.isword);
    length = modes::Record::Correspondence(
      intervals, statistics, progress, r1, r2,
      options.algorithm, options.ignorecase, options.ignorespace,
      options.isjoin, options.isverbose,
      options.join, options.prefix, options.suffix);
    if (intervals != nullptr)
      formatters::RecordDriver::Show(
        *format, *intervals, r1, r2, options.file1, options.file2);
  }
  return length;
}

Length File::Difference(
  shared_ptr<Delta>* intervals, metrics::Statistics* statistics,
  metrics::Progress* progress,
  const Options& options) {
  Length length(0);
  if (options.istext) {
    auto format = formatters::Formatter::Make(
      options.format, options.width, options.tabsize);
    u32string s1, s2;
    if (options.isquote) {
      // Treat filename arguments as literal strings.
      s1 = Encoding::DecodeUtf8(options.file1);
      s2 = Encoding::DecodeUtf8(options.file2);
    }
    else {
      s1 = modes::Text::Read(options.file1);
      s2 = modes::Text::Read(options.file2);
      if (options.isfasta) {
        s1 = PrepareFasta(options.file1, s1, options.isverbose);
        s2 = PrepareFasta(options.file2, s2, options.isverbose);
      }
    }
    length = modes::Text::Difference(
      intervals, statistics, progress, s1, s2,
      options.algorithm, options.ignorecase, options.ignorespace,
      options.isjoin, options.isverbose,
      options.join, options.prefix, options.suffix);
    if (intervals != nullptr)
      formatters::TextDriver::Show(
        *format, *intervals, s1, s2, options.file1, options.file2);
  }
  else {
    auto r1 = modes::Record::Read(options.file1, options.isword);
    auto r2 = modes::Record::Read(options.file2, options.isword);
    if (options.format == formatters::Format::Edit) {
      auto format = formatters::Formatter::Make(
        options.format, options.width, options.tabsize);
      formatters::EDIT_INTERVALS editIntervals;
      length = formatters::Edit::Difference(
        intervals != nullptr ? &editIntervals : nullptr,
        statistics, progress, r1, r2,
        options.algorithm, options.ignorecase, options.ignorespace,
        options.isjoin, options.isverbose,
        options.join, options.prefix, options.suffix);
      if (!editIntervals.empty()) {     // identical files: emit nothing, like diff -u
        formatters::RecordDriver::Show(
          *format, editIntervals, r1, r2, options.file1, options.file2);
      }
    }
    else {
      auto format = formatters::Formatter::Make(
        options.format, options.width, options.tabsize);
      length = modes::Record::Difference(
        intervals, statistics, progress, r1, r2,
        options.algorithm, options.ignorecase, options.ignorespace,
        options.isjoin, options.isverbose,
        options.join, options.prefix, options.suffix);
      if (intervals != nullptr)
        formatters::RecordDriver::Show(
          *format, *intervals, r1, r2, options.file1, options.file2);
    }
  }
  return length;
}
