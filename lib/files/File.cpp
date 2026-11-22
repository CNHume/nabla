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
#include "files/Fasta.h"
#include "formatters/Edit.h"
#include "formatters/RecordDriver.h"
#include "formatters/TextDriver.h"
#include "modes/Record.h"
#include "modes/Text.h"

#include <utility>                      // for move

using namespace std;
using namespace lcs;
using namespace lcs::files;
using modes::Record;
using modes::Text;
using formatters::Edit;
using formatters::EDIT_INTERVALS;
using formatters::Format;
using formatters::Formatter;
using formatters::RecordDriver;
using formatters::TextDriver;
using metrics::Progress;
using metrics::Statistics;

Length File::Correspondence(
  shared_ptr<Delta>* intervals, Statistics* statistics,
  Progress* progress,
  const Options& options) {
  Length length(0);
  //
  // Labels to be used by the Show() methods:
  //
  auto label1 = options.filename1;
  auto label2 = options.filename2;
  if (statistics) {
    statistics->label1 = label1;
    statistics->label2 = label2;
  }
  auto format = Formatter::Make(
    options.format, options.width, options.tabsize);
  if (options.istext) {
    u32string s1, s2;
    if (options.isquote) {
      // Treat filename arguments as literal strings.
      s1 = Encoding::DecodeUtf8(options.filename1);
      s2 = Encoding::DecodeUtf8(options.filename2);
    }
    else if (options.isfasta) {
      Fasta fasta1 = Fasta::Read(
        options.filename1, options.referenceName, options.referenceIndex);
      Fasta fasta2 = Fasta::Read(
        options.filename2, options.queryName, options.queryIndex);
      label1 = fasta1.Label();
      label2 = fasta2.Label();
      if (statistics) {
        statistics->label1 = label1;
        statistics->label2 = label2;
      }
      if (options.isverbose) {
        fasta1.Show();
        fasta2.Show();
      }
      if (!options.hasReferenceId)
        fasta1.WarnIgnored();
      if (!options.hasQueryId)
        fasta2.WarnIgnored();
      s1 = fasta1.Sequence();
      s2 = fasta2.Sequence();
    }
    else {
      s1 = Text::Read(options.filename1);
      s2 = Text::Read(options.filename2);
    }
    length = Text::Correspondence(
      intervals, statistics, progress, s1, s2,
      options.algorithm, options.ignorecase, options.ignorespace,
      options.isjoin, options.isverbose, options.isaffix,
      options.join, options.prefix, options.suffix);
    if (intervals != nullptr)
      TextDriver::Show(
        *format, *intervals, s1, s2, label1, label2);
  }
  else {
    auto r1 = Record::Read(options.filename1, options.isword);
    auto r2 = Record::Read(options.filename2, options.isword);
    length = Record::Correspondence(
      intervals, statistics, progress, r1, r2,
      options.algorithm, options.ignorecase, options.ignorespace,
      options.isjoin, options.isverbose, options.isaffix,
      options.join, options.prefix, options.suffix);
    if (intervals != nullptr)
      RecordDriver::Show(
        *format, *intervals, r1, r2, label1, label2);
  }
  return length;
}

Length File::Difference(
  shared_ptr<Delta>* intervals, Statistics* statistics,
  Progress* progress,
  const Options& options) {
  Length length(0);
  //
  // Labels to be used by the Show() methods:
  //
  auto label1 = options.filename1;
  auto label2 = options.filename2;
  if (statistics) {
    statistics->label1 = label1;
    statistics->label2 = label2;
  }
  if (options.istext) {
    auto format = Formatter::Make(
      options.format, options.width, options.tabsize);
    u32string s1, s2;
    if (options.isquote) {
      // Treat filename arguments as literal strings.
      s1 = Encoding::DecodeUtf8(options.filename1);
      s2 = Encoding::DecodeUtf8(options.filename2);
    }
    else if (options.isfasta) {
      Fasta fasta1 = Fasta::Read(
        options.filename1, options.referenceName, options.referenceIndex);
      Fasta fasta2 = Fasta::Read(
        options.filename2, options.queryName, options.queryIndex);
      label1 = fasta1.Label();
      label2 = fasta2.Label();
      if (statistics) {
        statistics->label1 = label1;
        statistics->label2 = label2;
      }
      if (options.isverbose) {
        fasta1.Show();
        fasta2.Show();
      }
      if (!options.hasReferenceId)
        fasta1.WarnIgnored();
      if (!options.hasQueryId)
        fasta2.WarnIgnored();
      s1 = fasta1.Sequence();
      s2 = fasta2.Sequence();
    }
    else {
      s1 = Text::Read(options.filename1);
      s2 = Text::Read(options.filename2);
    }
    length = Text::Difference(
      intervals, statistics, progress, s1, s2,
      options.algorithm, options.ignorecase, options.ignorespace,
      options.isjoin, options.isverbose, options.isaffix,
      options.join, options.prefix, options.suffix);
    if (intervals != nullptr)
      TextDriver::Show(
        *format, *intervals, s1, s2, label1, label2);
  }
  else {
    auto r1 = Record::Read(options.filename1, options.isword);
    auto r2 = Record::Read(options.filename2, options.isword);
    if (options.format == Format::Edit) {
      auto format = Formatter::Make(
        options.format, options.width, options.tabsize);
      EDIT_INTERVALS editIntervals;
      length = Edit::Difference(
        intervals != nullptr ? &editIntervals : nullptr,
        statistics, progress, r1, r2,
        options.algorithm, options.ignorecase, options.ignorespace,
        options.isjoin, options.isverbose, options.isaffix,
        options.join, options.prefix, options.suffix);
      // Identical files do not show any intervals
      if (!editIntervals.empty())
        RecordDriver::Show(
          *format, editIntervals, r1, r2, label1, label2);
    }
    else {
      auto format = Formatter::Make(
        options.format, options.width, options.tabsize);
      length = Record::Difference(
        intervals, statistics, progress, r1, r2,
        options.algorithm, options.ignorecase, options.ignorespace,
        options.isjoin, options.isverbose, options.isaffix,
        options.join, options.prefix, options.suffix);
      if (intervals != nullptr)
        RecordDriver::Show(
          *format, *intervals, r1, r2, label1, label2);
    }
  }
  return length;
}
