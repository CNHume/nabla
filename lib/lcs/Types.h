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
// 2026-11-07 CNHume  Created file
//
// Position and length types for nabla.
//
// Index is the type of a position index into a sequence; its width sets the
// maximum sequence length nabla can address.  The default 32-bit build covers
// up to 4,294,967,296 positions (position indexes 0 through 4,294,967,295; the
// human genome is ~3.1 billion) at half the memory of 64-bit indexes.  Define
// LENGTH_SIZE_64 to build a specialized release whose maximum sequence length
// is 64-bit.
//
#pragma once

#include <cstddef>                      // for size_t
#include <cstdint>                      // for uint32_t, uint64_t
#include <limits>                       // for numeric_limits

namespace lcs {

#ifdef LENGTH_SIZE_64
  using Index = std::uint64_t;          // specialized release: 64-bit positions
#else
  using Index = std::uint32_t;          // default: 32-bit positions
#endif

  //
  // Length counts the elements of a sequence and the sums of two lengths
  // (m + n) or the LCS length, which can exceed the range of a single Index.
  // It is always 64-bit so the 32-bit build cannot overflow it.
  //
  using Length = std::uint64_t;

  // Sentinel Index value meaning "none" (no predecessor, no match, no symbol).
  inline constexpr Index NoIndex = std::numeric_limits<Index>::max();

  //
  // ExceedsIndex() reports whether a sequence of `size` positions cannot be
  // addressed by Index: its largest position, size - 1, would not fit.  The
  // 32-bit build therefore supports up to 2^32 positions (4,294,967,296); the
  // 64-bit limit is unreachable in practice.  The 32-bit build guards its match
  // list construction with this so it fails loudly instead of silently wrapping.
  //
  inline bool ExceedsIndex(std::size_t size) {
    return size > 0 && size - 1 > (std::size_t)std::numeric_limits<Index>::max();
  }

} // namespace lcs
