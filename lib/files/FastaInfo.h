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
// 2026-11-15 CNHume  Created FastaInfo class
//
#pragma once

#include <string>                       // for string, u32string
#include <vector>                       // for vector

namespace lcs::files {

  //
  // FastaInfo parses one FASTA file's text.  Only the first record's sequence
  // is used; every additional record is ignored.  Each record's '>' defline is
  // split at its first whitespace run into a name (the identifier) and a
  // description (the remainder); the name is not subdivided further, so
  // pipe-delimited identifiers such as gi|...|ref|... are kept intact.
  //
  class FastaInfo {
  public:
    //
    // A FASTA record's '>' defline.
    //
    struct Record {
      std::string name;                  // first token of the '>' defline
      std::string description;           // remainder of the defline after the name
    };

    //
    // Parse text, moving it in; filename names the source for reporting.
    //
    FastaInfo(const std::string& filename, std::u32string text);

    const std::string& Filename() const { return filename_; }
    const std::vector<Record>& Records() const { return records_; }

    //
    // The first record's sequence, with '>' headers and whitespace removed.
    //
    std::u32string TakeSequence() { return std::move(sequence_); }

    //
    // Report the processed record under -v and warn for every ignored record.
    //
    void Show(bool isverbose) const;

  private:
    std::string filename_;
    std::u32string sequence_;
    std::vector<Record> records_;
  };

} // namespace lcs::files
