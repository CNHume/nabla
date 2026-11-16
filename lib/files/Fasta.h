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
#pragma once

#include <cstdint>                      // for uint32_t
#include <string>                       // for string, u32string
#include <vector>                       // for vector

namespace lcs::files {

  //
  // Fasta parses one FASTA file's text.  One record's sequence is used — the
  // first, or the one named by recordName / numbered by recordIndex — and
  // every other record is ignored.  Each record's '>' defline is split at its
  // first whitespace run into a name (the identifier) and a description (the
  // remainder); the name is not subdivided further, so pipe-delimited
  // identifiers such as gi|...|ref|... are kept intact.
  //
  class Fasta {
  public:
    //
    // A FASTA record's '>' defline.
    //
    struct Record {
      std::string name;                  // first token of the '>' defline
      std::string description;           // remainder of the defline after the name
    };

    //
    // Parse text, moving it in; filename names the source for reporting.  When
    // recordIndex is non-zero it selects a 1-based record; otherwise a non-empty
    // recordName selects the first record with that exact name.  With neither,
    // the first record is used.
    //
    Fasta(
      const std::string& filename, std::u32string text,
      const std::string& recordName = {}, std::uint32_t recordIndex = 0);

    const std::string& Filename() const { return filename_; }
    const std::vector<Record>& Records() const { return records_; }

    //
    // The selected record's sequence, with '>' headers and whitespace removed.
    //
    std::u32string Sequence() { return std::move(sequence_); }

    //
    // Report the selected record; call only under -v.
    //
    void Show() const;

    //
    // Warn for every record that is not selected; call unconditionally.
    //
    void WarnIgnored() const;

  private:
    std::string filename_;
    std::u32string sequence_;
    std::vector<Record> records_;
    std::size_t selected_ = 0;           // index of the record actually used
  };

  //
  // Parse a record_id as a 1-based index or a record name.  A leading '='
  // forces the name interpretation: "=42" names the record "42", while "42"
  // means the 42nd record.
  //
  void ParseRecordId(
    const std::string& id, std::string& name, std::uint32_t& index);

} // namespace lcs::files
