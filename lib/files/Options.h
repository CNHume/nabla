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
// 2026-10-24 CNHume  Created Options class
//
#pragma once

#include "lcs/LCS.h"
#include "formatters/Format.h"

#include <cstdint>                      // for uint32_t
#include <string>

namespace lcs::files {

  //
  // Options carries the comparison-relevant command-line parameters, so
  // File can accept options without depending on any tool's switch set.
  // Command-line tools derive their Command class from it and add only
  // the switches that are tool concerns.
  //
  class Options {
  public:
    Algorithm algorithm = Algorithm::Automatic;
    formatters::Format format = formatters::Format::Sequential;
    std::string filename1;              // a.k.a., reference file
    std::string filename2;              // a.k.a., query file
    bool ignorecase = false;
    bool ignorespace = false;
    bool isfasta = false;
    bool isjoin = false;
    bool isquote = false;
    bool istext = false;
    bool isverbose = false;
    bool isword = false;

    bool hasQueryId = false;
    std::string queryName;               // -q record name for query file (filename2)
    std::uint32_t queryIndex = 0;        // -q record index for query file (1-based)

    bool hasReferenceId = false;
    std::string referenceName;           // -r record name for reference file (filename1)
    std::uint32_t referenceIndex = 0;    // -r record index for reference file (1-based)

    std::uint32_t join = 0;
    std::uint32_t prefix = 0;
    std::uint32_t suffix = 0;
    //
    // The narrowest parallel interval "[1:2]" requires five columns.
    // To accommodate 2-digit line numbers the minimum width is 7.
    // Widths below this minimum are not allowed.
    //
    static constexpr std::uint32_t MIN_PARALLEL_WIDTH = 7;
    std::uint32_t width = 80;
    std::uint32_t tabsize = 8;
  };

} // namespace lcs::files
