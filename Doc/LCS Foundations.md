# Theoretical Foundations

The design of nabla's automatic dispatch (`-Aa`) is motivated by the lower
bounds of Aho, Hirschberg, and Ullman (1976).  This note states those bounds
precisely and their relationship to nabla.

## The model

Aho, Hirschberg, and Ullman study the longest-common-subsequence problem in
the *decision-tree* model: an algorithm may ask only questions of the form
"is the symbol at position X equal to the symbol at position Y?" — an
*equal-unequal* comparison.  No ordering, arithmetic, or hashing is allowed.

They write T(n, s) for the worst-case number of such comparisons required to
find an LCS of two length-n strings whose alphabet has s distinct symbols.

## The bounds

The paper's principal results are:

| Alphabet size s | T(n, s) |
|---|---|
| s = 2 | exactly 2n − 1 (linear) |
| 2 ≤ s ≤ n | (s/2)(n+s) ≤ T(n,s) ≤ min(n², (s−1)(2n−s)) |
| n ≤ s ≤ 4n/3 | 3ns/4 ≤ T(n,s) ≤ n² |
| s ≥ 4n/3 | exactly n² (quadratic) |

So T(n, s) rises from ~2n on a binary alphabet, through Θ(s·n), to n² — and it
saturates at n² once s reaches 4n/3.  The quadratic regime begins when the
alphabet is a constant fraction of the input length.

### Where 4n/3 comes from

The lower bound balances two competing costs.  The proof exhibits, in every
decision tree, a path on which either many *cross* comparisons (between the two
strings) are made, or many *side* comparisons (within one string) are made to
group equal positions into classes.

The crux is a *fundamental assignment*: at most s/2 distinct values per string
and none in common, so the LCS is empty.  Merging positions into a group
therefore costs ~s/2 side comparisons, while cross comparisons are at least
gA·gB for gA, gB groups.  Optimizing this trade-off gives a lower bound of

    min( n², 3ns/4, (s/2)(n+s) ).

The threshold 4n/3 is exactly where 3ns/4 = n² — i.e. s = 4n/3.  Beyond that,
merging no longer repays its side-comparison cost and the algorithm is stuck
at n².  The qualitative message is what matters: comparison-based LCS becomes
quadratic once the alphabet is a constant fraction of n.

## Unequal lengths

For two strings of lengths n₁ and n₂, with n₁ ≥ n₂, the same lower bounds hold
with the length parameter generalized.  The paper gives:

| Alphabet size s | Lower bound on T(n₁, n₂, s) |
|---|---|
| 0 ≤ s ≤ n₂ | (s/2)(n₁ + n₂ + s) |
| n₂ ≤ s ≤ 4n₁n₂/(2n₂ + n₁) | (s/4)(2n₂ + n₁) |
| 4n₁n₂/(2n₂ + n₁) ≤ s ≤ n₁ + n₂ | n₁n₂ |

Setting n₁ = n₂ = n recovers the equal-length results: the quadratic term
n₁n₂ becomes n², the middle term (s/4)(2n₂+n₁) becomes 3ns/4, and the
threshold 4n₁n₂/(2n₂+n₁) becomes 4n/3.  The threshold therefore interpolates
from 4n/3 for equal-length strings down toward 4n₂ as one string becomes much
shorter — the shorter string caps the number of distinct symbols that can
matter.

## The two models

The paper also considers a restricted model in which comparisons between
positions of the *same* string are forbidden (cross comparisons only).  There
the problem is 2n − 1 for s = 2 and n² for s ≥ 3.  In other words, side
comparisons are exactly what permits subquadratic behavior on small alphabets.

## Relationship to nabla

The consequence nabla takes from AHU is that the *alphabet size* — and with it
the match count r and the edit distance D — is a first-class parameter of LCS
difficulty, so no single fixed algorithm can be best on every input.  The -Aa
dispatcher measures structure and routes accordingly:

- small edit distance D → Myers (O(ND)),
- sparse matches r → Hunt-Szymanski (O((r + n) log n)),
- dense matches on a small alphabet → RickTime, with RickSpace as the
  linear-space fallback; their occurrence tables are O(s·(m + n)).

Two qualifications keep the claim honest.  First, AHU's lower bound is in the
comparison model; nabla's algorithms sort and hash to build match lists and
occurrence tables, so they are not literally bound by it.  Second, the
quadratic bound applies to *large* alphabets (s ≥ 4n/3).  A small alphabet
such as DNA (s = 4) is the regime where AHU shows subquadratic is achievable —
and RickTime/RickSpace realize exactly that O(s·n) behavior.

## Memory parameterization

RickTime and RickSpace build dense occurrence tables of sizeof(Index) · s ·
(m + n) bytes.  The dispatcher reserves 0.75 of available RAM for those tables
plus the dominant-match recording, and selects RickTime only when the
recording fits, falling back to RickSpace otherwise:

    room = 0.75 · availableRAM − sizeof(Index) · s · (m + n + 4) − overhead

## References

1. A. V. Aho, D. S. Hirschberg, and J. D. Ullman, "Bounds on the Complexity of
   the Longest Common Subsequence Problem", J. ACM 23(1):1-12, 1976.
2. J. W. Hunt and T. G. Szymanski, "A Fast Algorithm for Computing Longest
   Common Subsequences", CACM 20(5):350-353, 1977.
3. E. W. Myers, "An O(ND) Difference Algorithm and Its Variations",
   Algorithmica 1(2):251-266, 1986.
4. C. Rick, "A new flexible algorithm for the longest common subsequence
   problem", Proc. CPM 1995.
5. C. Rick, "Simple and fast linear space computation of longest common
   subsequences", Information Processing Letters 75:275-281, 2000.
