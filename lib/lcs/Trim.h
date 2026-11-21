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
// 2026-11-21 CNHume  Created file
//
#pragma once

#include "lcs/Types.h"

#include <cstddef>                      // for size_t

namespace lcs {

  //
  // Affix Trimming strips the maximal common prefix and common suffix (the
  // "affixes") of two sequences before the core LCS algorithm.  Affix symbols
  // are forced LCS members, so the algorithm runs on the remaining middle
  // only, and the affixes are restored to the result afterward.  This matters
  // most for Myers, whose O(ND) work is proportional to m + n.
  //
  // Trim() computes the lengths of the two affixes.  equal reports whether a
  // pair of symbols matches under the same case/space normalization the LCS
  // algorithms use.  The affixes never overlap: prefix + suffix <= min(|a|, |b|).
  //
  template <typename Sequence, typename Equal>
  void Trim(const Sequence& a, const Sequence& b, Equal equal,
    Index& prefix, Index& suffix) {
    const auto m = a.size();
    const auto n = b.size();
    const auto bound = m < n ? m : n;
    std::size_t p = 0;
    while (p < bound && equal(a[p], b[p]))
      p++;
    std::size_t s = 0;
    while (s < bound - p && equal(a[m - 1 - s], b[n - 1 - s]))
      s++;
    prefix = (Index)p;
    suffix = (Index)s;
  }

} // namespace lcs
