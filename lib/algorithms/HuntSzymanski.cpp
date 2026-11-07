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
// 2026-11-07 CNHume  Replaced Pair chains with a MatchNode arena trace
// 2017-07-05 CNHume  Applied prev() and next() in skip criteria.
// 2017-07-03 CNHume  Refresh limit iterator after prefixEnd.push_back()
// 2017-06-29 CNHume  Added Command class
// 2015-01-19 CNHume  Refactored Show() and Delta::Complement()
// 2015-01-17 CNHume  Added reclamation via shared_ptr<>
// 2015-01-16 CNHume  Avoid construction of superfluous Pairs
// 2015-01-15 CNHume  Settled on LCS as the name of the class
// 2015-01-06 CNHume  Added prefix and suffix support to Complement()
// 2015-01-02 CNHume  Implemented the Normal() method
// 2014-12-19 CNHume  Created file
//
// Purpose:
//
// This program implements a solution to the Longest Common Subsequence
// (LCS) Problem based on the Hunt and Szymanski algorithm.  Please see
// the overview provided in "Doc/LCS Overview.md"
//
#include "algorithms/HuntSzymanski.h"
#include "lcs/join.h"

#include <algorithm>                    // for lower_bound()
#include <cstddef>                      // for ptrdiff_t
#include <cstdint>                      // for uint32_t, UINT32_MAX
#include <format>                       // for format
#include <iostream>                     // for cout
#include <iterator>                     // for next() and prev()
#include <locale>                       // for locale
#include <memory>                       // for make_shared
#include <vector>                       // for vector

using namespace std;
using namespace lcs;

namespace lcs::algorithms {

namespace {
  //
  // MatchNode is an arena record for the trace.  It combines a match (row,
  // column) with a back-pointer next to its predecessor in the chain, i.e. it
  // is the paper's "list node" (newnode).  next indexes the arena
  // (UINT32_MAX = none), matching Pair::next's "toward the chain head"
  // direction without per-node shared_ptr allocation.
  //
  struct MatchNode {
    uint32_t row = 0;
    uint32_t column = 0;
    uint32_t next = UINT32_MAX;
  };

  //
  // pushNode() is Hunt & Szymanski (1977) Algorithm 2's trace, verbatim:
  //
  //   LINK[k] := newnode(i, j, LINK[k-1])
  //
  // where row = i, column = j, and prefix = the predecessor node
  // links[rank - 1] = LINK[k-1].  links[] is the paper's LINK array and each
  // MatchNode is a newnode list node.  The predecessor is snapshotted here at
  // creation time: a later update to links[rank - 1] must not disturb nodes
  // already built on it (see Doc/References/HuntSzymanski.pdf, "The Algorithm",
  // Theorem 1, which bounds this trace at O(r) space).  Returns the new node's
  // arena index.
  //
  uint32_t pushNode(
    vector<MatchNode>& nodes, const vector<uint32_t>& links,
    const ptrdiff_t& rank, uint32_t row, uint32_t column) {
    auto prefix = rank > 0 ? links[(size_t)(rank - 1)] : UINT32_MAX;
    nodes.push_back({ row, column, prefix });
    return (uint32_t)(nodes.size() - 1);
  }

  //
  // Reconstruct() returns the LCS as a Pair chain in ascending order by
  // walking the arena from the head of the longest chain.  next links toward
  // the chain head, so each match is prepended as it is visited.  This
  // embodies the work formerly done by Pair::Reverse(chains.back()) when the
  // trace was stored as a chain of Pair nodes.
  //
  shared_ptr<Pair> Reconstruct(
    const vector<MatchNode>& nodes, const vector<uint32_t>& links) {
    shared_ptr<Pair> head = nullptr;
    auto node = links.empty() ? UINT32_MAX : links.back();
    while (node != UINT32_MAX) {
      const auto& match = nodes[(size_t)node];
      head = make_shared<Pair>(match.row, match.column, head);
      node = match.next;
    }
    return head;
  }
}

uint32_t HuntSzymanski::Find(
  shared_ptr<Pair>* pairs, MATCHES& columnsMatchedByRow) {
  auto traceLCS = pairs != nullptr;
  vector<MatchNode> nodes;              // arena: one node per THRESH change
  vector<uint32_t> links;               // LINK array: arena index per LCS length
  INDEXES prefixEnd;                    // THRESH array: minimal column per length

  //
  //[Assert]After each row iteration prefixEnd[rank] is the least column
  // such that the LCS of s1[0:row] and s2[0:column] has length rank + 1
  //
  uint32_t row = 0;
  for (const auto& it1 : columnsMatchedByRow) {
    const auto& dq2 = *it1;
#ifdef SHOW_MATCHES
    cout << format("L1[{}] = {}\n", row, join(dq2, " "));
#endif
#ifdef SHOW_PREFIXENDS
    auto updated = false;
#endif
    auto limit = prefixEnd.end();
    for (auto it2 = dq2.rbegin(); it2 != dq2.rend(); it2++) {
      // Each row, column pair corresponds to a match
      auto column = *it2;

      //
      // Note: The reverse iterator it2 visits column values in descending order,
      // allowing in-place update of prefixEnd[].  std::lower_bound() is used to
      // perform a binary search.
      //
      limit = lower_bound(prefixEnd.begin(), limit, column);
#ifdef FILTER_PAIRS
      //
      // Look ahead to the next column value to optimize the MatchNodes used by
      // the Hunt and Szymanski algorithm.  If the next column is also an
      // improvement on the value currently held in prefixEnd[rank], a new
      // MatchNode will only be superseded on the next column iteration.
      //
      // Verify that a next column value exists; and that this value is greater
      // than the final column value of the LCS prefix at prev(limit):
      //
      auto preferNextColumn = next(it2) != dq2.rend() &&
        (limit == prefixEnd.begin() || *prev(limit) < *next(it2));

      //
      // Depending on the match count, this optimization reduces the number of
      // MatchNode arena entries by up to (r/n + 1)/2, where r/n is the average
      // number of matches per position.  At r/n = 1 there is no reduction.  At
      // maximal density the factor approaches n/2.
      //
      // The wall-clock benefit is modest and decays with n: ~1.40x faster at
      // n = 10,000 vs. ~1.26x at n = 100,000, because the O(log n) lower_bound
      // dominates the saved entry as the input grows.
      //
      if (preferNextColumn) continue;
#endif
      auto rank = distance(prefixEnd.begin(), limit);
#ifdef SHOW_PREFIXENDS
      auto len = rank + 1;
#endif
      if (limit == prefixEnd.end()) {
        // Insert Case
#ifdef SHOW_PREFIXENDS
        updated = true;
        cout << format(
          "inserting {} at {} for length = {}\n",
          column, row, len);
#endif
        prefixEnd.push_back(column);
        // Refresh limit iterator:
        limit = prev(prefixEnd.end());
        if (traceLCS) {
          links.push_back(pushNode(nodes, links, rank, row, column));
        }
      }
      else if (column < *limit) {
        // Update Case
#ifdef SHOW_PREFIXENDS
        updated = true;
        cout << format(
          "replacing {} with {} at {} for length = {}\n",
          *limit, column, row, len);
#endif
        // Update limit value:
        * limit = column;
        if (traceLCS) {
          links[rank] = pushNode(nodes, links, rank, row, column);
        }
      }
    }                                   // next column
#ifdef SHOW_PREFIXENDS
    if (updated) {
      uint32_t index = 0;
      for (const auto& it3 : prefixEnd) {
        cout << format("end[{}] = {}\n", index++, it3);
      }
    }
#endif
    row++;
  }                                     // next row

  if (traceLCS) {
    *pairs = Reconstruct(nodes, links);
#ifdef SHOW_PAIRS
    Pair::List(*pairs);
#endif
  }

  auto length = prefixEnd.size();
#ifdef _DEBUG
  cout << format(
    std::locale(), "Pair Count = {:L}\n", Pair::Count);
#endif
  return length;
}

} // namespace lcs::algorithms
