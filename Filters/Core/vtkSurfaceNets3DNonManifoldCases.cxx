// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkSurfaceNets3DNonManifoldCases.h"

#include <algorithm>
#include <bitset>
#include <cassert>
#include <numeric>

// enable when creating the metadata table
// #define VTK_SURFACE_NETS_3D_NON_MANIFOLD_CASES_CREATE_TABLE
#ifdef VTK_SURFACE_NETS_3D_NON_MANIFOLD_CASES_CREATE_TABLE
#include <iostream>
#endif

namespace
{
VTK_ABI_NAMESPACE_BEGIN
//--------------------------Constexpr Array Functions---------------------------
/**
 * Sorts an array of size_t values in ascending order in-place using insertion sort.
 * Implemented as a constexpr-friendly alternative to std::sort, which is not
 * constexpr before C++20.
 *
 * @tparam T Element type
 * @tparam N Size of the array.
 * @param arr The array to sort in-place.
 */
template <class T, size_t N>
constexpr void SortArray(std::array<T, N>& arr)
{
  for (size_t i = 1; i < N; ++i)
  {
    T key = arr[i];
    ptrdiff_t j = static_cast<ptrdiff_t>(i) - 1;
    while (j >= 0 && arr[j] > key)
    {
      arr[j + 1] = arr[j];
      --j;
    }
    arr[j + 1] = key;
  }
}

/**
 * Compares two arrays element-wise for equality.
 * Implemented as a constexpr-friendly alternative to std::operator==, which is not
 * constexpr before C++20.
 *
 * @tparam T Element type.
 * @tparam N Size of the arrays.
 * @param a First array.
 * @param b Second array.
 * @return True if all elements are equal, false otherwise.
 */
template <class T, size_t N>
constexpr auto ArrayEqual(const std::array<T, N>& a, const std::array<T, N>& b) -> bool
{
  for (size_t i = 0; i < N; ++i)
  {
    if (a[i] != b[i])
    {
      return false;
    }
  }
  return true;
}
//------------------------------------------------------------------------------

//-------------------------------Combinatorics----------------------------------
/**
 * Computes the binomial coefficient C(N, M) — the number of ways to choose M items from N.
 * Used to reserve vector capacity in Combinations, avoiding reallocations during push_back.
 *
 * @tparam N Total number of items.
 * @tparam M Number of items to choose.
 * @return C(N, M).
 */
template <size_t N, size_t M>
constexpr size_t BinomialCoefficient()
{
  size_t num = 1, den = 1;
  for (size_t i = 0; i < M; ++i)
  {
    num *= (N - i);
    den *= (i + 1);
  }
  return num / den;
}

/**
 * The number of unique pairs in a set of N elements, equal to C(N, 2).
 *
 * @tparam N Number of elements.
 */
template <size_t N>
inline constexpr size_t NumPairs = BinomialCoefficient<N, 2>();

/**
 * Generates all C(N, M) combinations of elements from a collection of elements.
 * The return array is sized exactly to BinomialCoefficient<N, M>(), which is the exact
 * number of combinations produced.
 *
 * @tparam M  Number of elements per combination.
 * @tparam N  Number of source elements.
 * @tparam T  Type of each value within an element (e.g., uint8_t).
 * @tparam K  Size of each element (e.g., 3 for 3D coordinates).
 * @param elements The source elements to combine.
 * @return An array of all C(N, M) combinations, each as an array of M elements of size K.
 */
template <size_t M, size_t N, class T, size_t K>
constexpr auto Combinations(const std::array<std::array<T, K>, N>& elements)
  -> std::array<std::array<std::array<T, K>, M>, BinomialCoefficient<N, M>()>
{
  std::array<std::array<std::array<T, K>, M>, BinomialCoefficient<N, M>()> result{};
  size_t count = 0;

  std::array<size_t, M> chosen{};
  for (size_t i = 0; i < M; ++i)
  {
    chosen[i] = i;
  }

  while (true)
  {
    std::array<std::array<T, K>, M> sub{};
    for (size_t i = 0; i < M; ++i)
    {
      sub[i] = elements[chosen[i]];
    }
    result[count++] = sub;

    ptrdiff_t i = static_cast<ptrdiff_t>(M) - 1;
    while (i >= 0 && chosen[i] == N - M + static_cast<size_t>(i))
    {
      --i;
    }
    if (i < 0)
    {
      break;
    }
    ++chosen[static_cast<size_t>(i)];
    for (size_t j = static_cast<size_t>(i + 1); j < M; ++j)
    {
      chosen[j] = chosen[j - 1] + 1;
    }
  }

  return result;
}

#ifdef VTK_SURFACE_NETS_3D_NON_MANIFOLD_CASES_CREATE_TABLE
/**
 * Computes the Bell number B(N), which counts the number of distinct ways to partition
 * a set of N elements into non-empty subsets.
 *
 * For example, B(3) = 5 because the set {A, B, C} can be partitioned as:
 *   {ABC}, {A}{BC}, {B}{AC}, {C}{AB}, {A}{B}{C}
 *
 * The first few Bell numbers are:
 *   B(0)=1, B(1)=1, B(2)=2, B(3)=5, B(4)=15, B(5)=52, B(6)=203
 *
 * Computed via the Bell triangle, where each row i starts with the last element of
 * the previous row, and each subsequent element is the sum of the previous element
 * in the current row and the element above it in the previous row. B(N) is the first
 * element of row N.
 *
 * In the context of material-variation enumeration, B(N) gives the total number of
 * colorings of N voxels produced by ColoredVariations(), including the trivial
 * all-same-color partition.
 *
 * @tparam N Number of elements to partition.
 * @return  The Bell number B(N).
 */
template <size_t N>
constexpr size_t BellNumber()
{
  std::array<std::array<size_t, N + 1>, N + 1> bell{};
  bell[0][0] = 1;
  for (size_t i = 1; i <= N; ++i)
  {
    bell[i][0] = bell[i - 1][i - 1];
    for (size_t j = 1; j <= i; ++j)
    {
      bell[i][j] = bell[i][j - 1] + bell[i - 1][j - 1];
    }
  }
  return bell[N][0];
}

/**
 * Enumerates all colorings of N elements using restricted growth strings (RGS).
 *
 * A restricted growth string is a sequence a[0...n-1] where:
 *   - a[0] = 0 always (first element is always in group 0)
 *   - a[i] <= max(a[0...i-1]) + 1 (a new group index can only be one more than
 *     the current maximum, ensuring canonical enumeration without duplicates)
 *
 * Note: the RGS is defined and computed using 0-based group indices. The output
 * is shifted by 1 to produce 1-based color indices, reserving 0 as a sentinel
 * for inactive corners in ComputeColorEncodedVoxelCase() and
 * ExtractColorEncodedVoxelCase().
 *
 * For example with n=3, internal RGS → 1-based output:
 *   [0, 0, 0] → [1, 1, 1] → all same group     (trivial, all same material)
 *   [0, 0, 1] → [1, 1, 2] → groups {0,1} and {2}
 *   [0, 1, 0] → [1, 2, 1] → groups {0,2} and {1}
 *   [0, 1, 1] → [1, 2, 2] → groups {0}   and {1,2}
 *   [0, 1, 2] → [1, 2, 3] → groups {0},  {1},  and {2}
 *
 * The auxiliary array b[] tracks the running maximum of a[], i.e. b[i] = max(a[0...i]).
 * This allows O(1) checking of the RGS constraint at each position, and O(1) detection
 * of non-trivial colorings via b[n-1] > 0.
 *
 * The increment step finds the rightmost position i that can be incremented (i.e.
 * a[i] < b[i-1] + 1), increments it, updates b[i], then resets all positions to the
 * right back to 0 (the smallest valid value), recomputing b[] accordingly.
 *
 * The trivial all-same-color partition is explicitly prepended since the RGS iteration
 * skips it via the b[n-1] > 0 guard. The total number of returned colorings is exactly
 * BellNumber(N) = B(N).
 *
 * @tparam N Number of elements to color.
 * @tparam T The unsigned type to use for color indices (e.g., uint8_t). Must be large
 *           enough to hold BellNumber(N).
 * @return  All 1-based color assignments, each of length N, starting with the trivial
 *          all-same-color partition [1,1,...,1]. The number of returned colorings equals
 *          BellNumber(N).
 */
template <size_t N, class T = uint8_t>
constexpr auto ColoredVariations() -> std::array<std::array<T, N>, BellNumber<N>()>
{
  static_assert(
    N <= std::numeric_limits<T>::max(), "N is too large for type T — use a larger type.");
  std::array<std::array<T, N>, BellNumber<N>()> result{};
  size_t index = 0;

  // trivial coloring: all elements in group 0
  result[index++] = std::array<T, N>{};

  std::array<T, N> a{}; // color assignment (RGS), 0-based
  std::array<T, N> b{}; // b[i] = max(a[0..i])

  while (true)
  {
    if (b[N - 1] > 0) // non-trivial: at least 2 groups
    {
      result[index++] = a;
    }
    ptrdiff_t i = static_cast<ptrdiff_t>(N) - 1;
    while (i > 0 && a[i] == static_cast<T>(b[i - 1] + 1))
    {
      --i;
    }
    if (i == 0)
    {
      break;
    }
    ++a[i];
    b[i] = std::max(b[i - 1], a[i]);
    for (size_t j = static_cast<size_t>(i + 1); j < N; ++j)
    {
      a[j] = 0;
      b[j] = b[j - 1];
    }
  }

  // shift all color indices by 1 to produce 1-based output
  for (auto& coloring : result)
  {
    for (auto& color : coloring)
    {
      color = static_cast<T>(color + 1);
    }
  }
  return result;
}
#endif
//------------------------------------------------------------------------------

//-------------------------------Distances--------------------------------------
/**
 * Computes the Hamming distance between two coordinates.
 * The Hamming distance is the sum of absolute differences between corresponding elements.
 *
 * @tparam T Type of the coordinate elements.
 * @tparam N Size of the coordinate arrays.
 * @param a First coordinate.
 * @param b Second coordinate.
 * @return Sum of absolute differences between corresponding elements.
 *         Returns double for floating-point T, size_t for integral T.
 */
template <class T, size_t N,
  class TDist = std::conditional_t<std::is_floating_point_v<T>, double, size_t>>
constexpr auto HammingDistance(const std::array<T, N>& a, const std::array<T, N>& b) -> TDist
{
  TDist dist = 0;
  for (size_t i = 0; i < N; ++i)
  {
    dist += static_cast<TDist>((a[i] > b[i]) ? (a[i] - b[i]) : (b[i] - a[i]));
  }
  return dist;
}

/**
 * Computes the sorted list of pairwise Hamming distances between all elements of a
 * collection of coordinates.
 *
 * @tparam T Type of the elements within each coordinate (e.g., uint8_t).
 * @tparam N Size of each coordinate (e.g., 3 for 3D coordinates).
 * @tparam M Number of coordinates.
 * @param c  The coordinates to compute distances for, as an array of M coordinates of size N.
 * @return   Sorted array of NumPairs<M> pairwise Hamming distances, in ascending order.
 */
template <class T, size_t N, size_t M,
  class TDist = std::conditional_t<std::is_floating_point_v<T>, double, size_t>>
constexpr auto PairwiseDistances(const std::array<std::array<T, N>, M>& c)
  -> std::array<TDist, NumPairs<M>>
{
  std::array<size_t, NumPairs<M>> dists{};
  size_t k = 0;
  for (size_t i = 0; i < M; ++i)
  {
    for (size_t j = i + 1; j < M; ++j)
    {
      dists[k++] = HammingDistance(c[i], c[j]);
    }
  }
  SortArray(dists);
  return dists;
}
//------------------------------------------------------------------------------

//----------------------------Voxel Case Coordinates----------------------------
using Coord = std::array<uint8_t, 3>;

template <size_t N>
using Case = std::array<Coord, N>;

constexpr Case<8> Coords = { {
  { 0, 0, 0 }, // bit 0
  { 1, 0, 0 }, // bit 1
  { 0, 1, 0 }, // bit 2
  { 1, 1, 0 }, // bit 3
  { 0, 0, 1 }, // bit 4
  { 1, 0, 1 }, // bit 5
  { 0, 1, 1 }, // bit 6
  { 1, 1, 1 }, // bit 7
} };

/**
 * Computes the voxel case bitmask for a given the coordinates of voxel case.
 * The bitmask is an 8-bit integer where each bit corresponds to one of the 8 corners of the
 * voxel, and is set to 1 if that corner's coordinate is present in the input case.
 *
 * @tparam TArray The type of the input array (e.g., std::array<Coord, N>).
 * @param voxelCaseCoords The voxel case's coordinates for which to compute the bitmask.
 * @return An 8-bit integer bitmask representing which corners are present in the case.
 */
template <class TArray>
constexpr auto ComputeVoxelCase(const TArray& voxelCaseCoords) -> uint8_t
{
  uint8_t mask = 0;
  for (const Coord& coord : voxelCaseCoords)
  {
    for (uint8_t pos = 0; pos < static_cast<uint8_t>(Coords.size()); ++pos)
    {
      if (ArrayEqual(Coords[pos], coord))
      {
        mask |= 1 << pos;
        break;
      }
    }
  }
  return mask;
}

/**
 * Represents the coordinates of a voxel case as an ordered subset of the 8 unit cube corners,
 * extending Case<N> with domain-specific operations.
 *
 * @tparam N The number of active corners in this voxel case.
 */
template <size_t N>
class VoxelCaseCoords : public Case<N>
{
public:
  /**
   * Computes the coordinates of the complementary voxel case. The complementary voxel case
   * consists of all unit cube corners not present in this voxel case. Since the unit cube
   * has 8 corners, the complementary voxel case always has 8-N corners.
   *
   * @return The coordinates of the complementary voxel case, containing the 8-N corners
   *         not present in this voxel case, ordered according to the Coords ordering.
   */
  constexpr auto GetComplementaryCoords() const -> VoxelCaseCoords<8 - N>
  {
    const uint8_t voxelCase = ComputeVoxelCase(*this);
    const uint8_t complementaryVoxelCase = ~voxelCase;

    VoxelCaseCoords<8 - N> result{};
    size_t index = 0;
    for (size_t i = 0; i < 8; ++i)
    {
      if (complementaryVoxelCase & (1 << i))
      {
        result[index++] = Coords[i];
      }
    }
    return result;
  }

  auto Print(std::ostream& os) const -> std::ostream&
  {
    os << "Voxel Case " << static_cast<int>(ComputeVoxelCase(*this)) << ": { ";
    for (const auto& coord : *this)
    {
      os << "{" << static_cast<int>(coord[0]) << "," << static_cast<int>(coord[1]) << ","
         << static_cast<int>(coord[2]) << "} ";
    }
    os << "}" << std::endl;
    return os;
  }
};
//------------------------------------------------------------------------------

//----------------------Non-Manifold & Manifold Cases---------------------------
template <size_t N>
struct CaseEntry
{
  VoxelCaseCoords<N> Coords;
  vtkSurfaceNets3DNonManifoldCases::VoxelCaseType VoxelCase;
};

template <size_t M, size_t N>
using CaseTable = std::array<CaseEntry<M>, N>;

/**
 * Counts the number of C(N, M) combinations of coordinates whose sorted pairwise
 * Hamming distance signature matches the given target.
 * Intended for use with static_assert to verify the expected number of matching
 * cases K passed to CreateCaseTable at compile time.
 *
 * @tparam M                     Number of coordinates per case.
 * @tparam N                     Number of source coordinates.
 * @param coords                 Source coordinates to combine.
 * @param hammingDistancesTarget Sorted pairwise Hamming distance signature to match against.
 * @return Number of combinations whose pairwise distance signature matches the target.
 */
template <size_t M, size_t N>
constexpr auto CountMatchingCases(const std::array<Coord, N>& coords,
  const std::array<size_t, NumPairs<M>>& hammingDistancesTarget) -> size_t
{
  size_t count = 0;
  for (const Case<M>& sub : Combinations<M, N>(coords))
  {
    if (ArrayEqual(PairwiseDistances(sub), hammingDistancesTarget))
    {
      count++;
    }
  }
  return count;
}

/**
 * Filters all C(N, M) combinations of coordinates, retaining only those whose
 * sorted pairwise Hamming distance signature matches the given target.
 * The expected number of matching cases K must be known at compile time and can
 * be verified against CountMatchingCases via static_assert before calling this function.
 *
 * @tparam M                     Number of coordinates per case.
 * @tparam K                     Number of results expected to match the target signature.
 *                               Must equal CountMatchingCases<M>(coords, hammingDistancesTarget).
 * @tparam N                     Number of source coordinates.
 * @param coords                 Source coordinates to combine.
 * @param hammingDistancesTarget Sorted pairwise Hamming distance signature to match against.
 * @return A CaseTable containing exactly the K matching cases.
 */
template <size_t M, size_t K, size_t N>
constexpr auto CreateCaseTable(const std::array<Coord, N>& coords,
  const std::array<size_t, NumPairs<M>>& hammingDistancesTarget) -> CaseTable<M, K>
{
  CaseTable<M, K> table{};
  size_t index = 0;
  for (const Case<M>& sub : Combinations<M, N>(coords))
  {
    if (ArrayEqual(PairwiseDistances(sub), hammingDistancesTarget))
    {
      table[index++] = { { sub }, ComputeVoxelCase(sub) };
    }
  }
  return table;
}

constexpr std::array<size_t, NumPairs<2>> HammingDistancesNonManifoldCase1 = { { 3 } };
constexpr std::array<size_t, NumPairs<2>> HammingDistancesNonManifoldCase2 = { { 2 } };
constexpr std::array<size_t, NumPairs<3>> HammingDistancesNonManifoldCase3 = { { 1, 2, 3 } };
constexpr std::array<size_t, NumPairs<3>> HammingDistancesNonManifoldCase4 = { { 2, 2, 2 } };
constexpr std::array<size_t, NumPairs<4>> HammingDistancesNonManifoldCase5 = { { 1, 1, 2, 2, 3,
  3 } };
constexpr std::array<size_t, NumPairs<4>> HammingDistancesNonManifoldCase6 = { { 1, 1, 2, 2, 2,
  3 } };
constexpr std::array<size_t, NumPairs<4>> HammingDistancesNonManifoldCase7 = { { 2, 2, 2, 2, 2,
  2 } };
constexpr std::array<size_t, NumPairs<5>> HammingDistancesNonManifoldCase8 = { { 1, 1, 1, 2, 2, 2,
  2, 2, 2, 3 } };
constexpr std::array<size_t, NumPairs<5>> HammingDistancesNonManifoldCase9 = { { 1, 1, 1, 1, 2, 2,
  2, 2, 3, 3 } };
constexpr std::array<size_t, NumPairs<6>> HammingDistancesNonManifoldCase10 = { { 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 3, 3 } };
constexpr std::array<size_t, NumPairs<6>> HammingDistancesNonManifoldCase11 = { { 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 3, 3, 3 } };

static_assert(CountMatchingCases<2>(Coords, HammingDistancesNonManifoldCase1) == 4);
constexpr auto NonManifoldCase1 = CreateCaseTable<2, 4>(Coords, HammingDistancesNonManifoldCase1);
static_assert(CountMatchingCases<2>(Coords, HammingDistancesNonManifoldCase2) == 12);
constexpr auto NonManifoldCase2 = CreateCaseTable<2, 12>(Coords, HammingDistancesNonManifoldCase2);
static_assert(CountMatchingCases<3>(Coords, HammingDistancesNonManifoldCase3) == 24);
constexpr auto NonManifoldCase3 = CreateCaseTable<3, 24>(Coords, HammingDistancesNonManifoldCase3);
static_assert(CountMatchingCases<3>(Coords, HammingDistancesNonManifoldCase4) == 8);
constexpr auto NonManifoldCase4 = CreateCaseTable<3, 8>(Coords, HammingDistancesNonManifoldCase4);
static_assert(CountMatchingCases<4>(Coords, HammingDistancesNonManifoldCase5) == 6);
constexpr auto NonManifoldCase5 = CreateCaseTable<4, 6>(Coords, HammingDistancesNonManifoldCase5);
static_assert(CountMatchingCases<4>(Coords, HammingDistancesNonManifoldCase6) == 24);
constexpr auto NonManifoldCase6 = CreateCaseTable<4, 24>(Coords, HammingDistancesNonManifoldCase6);
static_assert(CountMatchingCases<4>(Coords, HammingDistancesNonManifoldCase7) == 2);
constexpr auto NonManifoldCase7 = CreateCaseTable<4, 2>(Coords, HammingDistancesNonManifoldCase7);
static_assert(CountMatchingCases<5>(Coords, HammingDistancesNonManifoldCase8) == 8);
constexpr auto NonManifoldCase8 = CreateCaseTable<5, 8>(Coords, HammingDistancesNonManifoldCase8);
static_assert(CountMatchingCases<5>(Coords, HammingDistancesNonManifoldCase9) == 24);
constexpr auto NonManifoldCase9 = CreateCaseTable<5, 24>(Coords, HammingDistancesNonManifoldCase9);
static_assert(CountMatchingCases<6>(Coords, HammingDistancesNonManifoldCase10) == 12);
constexpr auto NonManifoldCase10 =
  CreateCaseTable<6, 12>(Coords, HammingDistancesNonManifoldCase10);
static_assert(CountMatchingCases<6>(Coords, HammingDistancesNonManifoldCase11) == 4);
constexpr auto NonManifoldCase11 = CreateCaseTable<6, 4>(Coords, HammingDistancesNonManifoldCase11);
constexpr auto NonManifoldCases = std::make_tuple(NonManifoldCase1, NonManifoldCase2,
  NonManifoldCase3, NonManifoldCase4, NonManifoldCase5, NonManifoldCase6, NonManifoldCase7,
  NonManifoldCase8, NonManifoldCase9, NonManifoldCase10, NonManifoldCase11);

#ifdef VTK_SURFACE_NETS_3D_NON_MANIFOLD_CASES_CREATE_TABLE
constexpr std::array<size_t, NumPairs<1>> HammingDistancesManifoldCase12 = { {} };
constexpr std::array<size_t, NumPairs<2>> HammingDistancesManifoldCase13 = { { 1 } };
constexpr std::array<size_t, NumPairs<3>> HammingDistancesManifoldCase14 = { { 1, 1, 2 } };
constexpr std::array<size_t, NumPairs<4>> HammingDistancesManifoldCase15 = { { 1, 1, 1, 1, 2, 2 } };
constexpr std::array<size_t, NumPairs<4>> HammingDistancesManifoldCase16 = { { 1, 1, 1, 2, 2, 2 } };
constexpr std::array<size_t, NumPairs<4>> HammingDistancesManifoldCase17 = { { 1, 1, 1, 2, 2, 3 } };
constexpr std::array<size_t, NumPairs<5>> HammingDistancesManifoldCase18 = { { 1, 1, 1, 1, 1, 2, 2,
  2, 2, 3 } };
constexpr std::array<size_t, NumPairs<6>> HammingDistancesManifoldCase19 = { { 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 3, 3 } };
constexpr std::array<size_t, NumPairs<7>> HammingDistancesManifoldCase20 = { { 1, 1, 1, 1, 1, 1, 1,
  1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3 } };
constexpr std::array<size_t, NumPairs<8>> HammingDistancesManifoldCase21 = { { 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3 } };

static_assert(CountMatchingCases<1>(Coords, HammingDistancesManifoldCase12) == 8);
constexpr auto ManifoldCase12 = CreateCaseTable<1, 8>(Coords, HammingDistancesManifoldCase12);
static_assert(CountMatchingCases<2>(Coords, HammingDistancesManifoldCase13) == 12);
constexpr auto ManifoldCase13 = CreateCaseTable<2, 12>(Coords, HammingDistancesManifoldCase13);
static_assert(CountMatchingCases<3>(Coords, HammingDistancesManifoldCase14) == 24);
constexpr auto ManifoldCase14 = CreateCaseTable<3, 24>(Coords, HammingDistancesManifoldCase14);
static_assert(CountMatchingCases<4>(Coords, HammingDistancesManifoldCase15) == 6);
constexpr auto ManifoldCase15 = CreateCaseTable<4, 6>(Coords, HammingDistancesManifoldCase15);
static_assert(CountMatchingCases<4>(Coords, HammingDistancesManifoldCase16) == 8);
constexpr auto ManifoldCase16 = CreateCaseTable<4, 8>(Coords, HammingDistancesManifoldCase16);
static_assert(CountMatchingCases<4>(Coords, HammingDistancesManifoldCase17) == 24);
constexpr auto ManifoldCase17 = CreateCaseTable<4, 24>(Coords, HammingDistancesManifoldCase17);
static_assert(CountMatchingCases<5>(Coords, HammingDistancesManifoldCase18) == 24);
constexpr auto ManifoldCase18 = CreateCaseTable<5, 24>(Coords, HammingDistancesManifoldCase18);
static_assert(CountMatchingCases<6>(Coords, HammingDistancesManifoldCase19) == 12);
constexpr auto ManifoldCase19 = CreateCaseTable<6, 12>(Coords, HammingDistancesManifoldCase19);
static_assert(CountMatchingCases<7>(Coords, HammingDistancesManifoldCase20) == 8);
constexpr auto ManifoldCase20 = CreateCaseTable<7, 8>(Coords, HammingDistancesManifoldCase20);
static_assert(CountMatchingCases<8>(Coords, HammingDistancesManifoldCase21) == 1);
constexpr auto ManifoldCase21 = CreateCaseTable<8, 1>(Coords, HammingDistancesManifoldCase21);
[[maybe_unused]] constexpr auto ManifoldCases =
  std::make_tuple(ManifoldCase12, ManifoldCase13, ManifoldCase14, ManifoldCase15, ManifoldCase16,
    ManifoldCase17, ManifoldCase18, ManifoldCase19, ManifoldCase20, ManifoldCase21);

constexpr auto AllCases = std::make_tuple(NonManifoldCase1, NonManifoldCase2, NonManifoldCase3,
  NonManifoldCase4, NonManifoldCase5, NonManifoldCase6, NonManifoldCase7, NonManifoldCase8,
  NonManifoldCase9, NonManifoldCase10, NonManifoldCase11, ManifoldCase12, ManifoldCase13,
  ManifoldCase14, ManifoldCase15, ManifoldCase16, ManifoldCase17, ManifoldCase18, ManifoldCase19,
  ManifoldCase20, ManifoldCase21);
#endif

/**
 * Maps a VoxelCase bitmask to its non-manifold case classification (1-11),
 * or 0 if no match. Used as a precomputed 256-entry lookup table to avoid
 * runtime iteration over all non-manifold case tables.
 */
struct NonManifoldCaseEntry
{
  // 0 = no match, 1-11 = case
  vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseType NonManifoldCaseIndex;
  vtkSurfaceNets3DNonManifoldCases::VoxelCaseType VoxelCase;
};

// Build a 256-entry lookup table mapping each VoxelCase value to the first
// (lowest-numbered) non-manifold case it appears in. First-match-wins semantics
// preserve the original loop's behavior.
constexpr auto BuildVoxelCaseToNonManifoldLookup() -> std::array<NonManifoldCaseEntry, 256>
{
  std::array<NonManifoldCaseEntry, 256> table{}; // zero-initialized → CaseIndex = 0 = "no match"

  // Helper: insert all entries from one case table, only if voxelCase has no entry yet.
  auto insertCase = [&table](auto nonManifoldCaseIndex, const auto& nonManifoldCases) constexpr
  {
    for (const auto& [_, voxelCase] : nonManifoldCases)
    {
      if (table[voxelCase].NonManifoldCaseIndex == 0)
      { // first-match-wins
        table[voxelCase] = { nonManifoldCaseIndex, voxelCase };
      }
    }
  };
  // Iterate the NonManifoldCases tuple in order (case 1 first, then 2, ...).
  std::apply(
    [&](const auto&... cases)
    {
      vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseType nonManifoldCaseIndex = 1;
      ((insertCase(nonManifoldCaseIndex++, cases)), ...);
    },
    NonManifoldCases);

  return table;
}

constexpr auto VoxelCaseToNonManifold = BuildVoxelCaseToNonManifoldLookup();
//------------------------------------------------------------------------------

//---------------------------Color Encoded Voxel Case---------------------------
#ifdef VTK_SURFACE_NETS_3D_NON_MANIFOLD_CASES_CREATE_TABLE
/**
 * Computes a color-encoded voxel case by expanding a compact color assignment of N active
 * voxels into a full 8-element array aligned with the 8 cube corners. Each element of the
 * output corresponds to one corner of the cube (following the Coords ordering), and holds
 * the color index of that corner if it is part of the voxel case's coordinates, or 0 if it is
 * not.
 *
 * @tparam N                     The number of voxel case's coordinates.
 * @tparam T                     The unsigned type to use for color indices (e.g., uint8_t).
 * @param voxelCaseCoords        The coordinates of the N active voxels.
 * @param colorEncodedVoxelCase  A color assignment of length N, where elements with the same
 *                               color index share a material label, as produced by
 *                               ColoredVariations().
 * @return An 8-element array aligned with the cube corners, where each element holds
 *         the color index of that corner, or 0 if the corner is not active.
 */
template <size_t N, class T = uint8_t>
constexpr auto ComputeColorEncodedVoxelCase(const VoxelCaseCoords<N>& voxelCaseCoords,
  const std::array<T, N>& colorEncodedVoxelCase) -> std::array<T, 8>
{
  std::array<T, 8> expanded{};
  for (size_t i = 0; i < N; ++i)
  {
    // find the index of this coordinate in the global Coords array to determine
    // which of the 8 cube corners it corresponds to
    for (size_t j = 0; j < Coords.size(); ++j)
    {
      if (Coords[j] == voxelCaseCoords[i])
      {
        // assign the color index of this active voxel to its corresponding corner
        expanded[j] = colorEncodedVoxelCase[i];
        break;
      }
    }
  }
  return expanded;
}
#endif

/**
 * Extracts a color-encoded voxel case from the material labels at the 8 corners of a voxel.
 * Active corners (those set in voxelCase) are assigned color indices 1, 2, 3, ... in order of first
 * appearance of their material label. Inactive corners are assigned 0.
 *
 * The resulting color-encoded voxel case matches the convention used by
 * ComputeColorEncodedVoxelCase() and GetNonManifoldCaseForColorEncodedVoxelCase(), allowing
 * direct comparison against colorings stored in NonManifoldCaseMetadata.
 *
 * @tparam MT            The type of the material labels.
 * @tparam CT            The unsigned type to use for color indices (e.g., uint8_t).
 * @param labels         The material labels at all 8 corners of the voxel.
 * @param voxelCase The basic voxel case bitmask identifying active corners.
 * @return A color-encoded voxel case of size 8, where inactive corners are 0
 *         and active corners carry a color index 1-8.
 */
template <class MT, class CT = uint8_t>
constexpr auto ExtractColorEncodedVoxelCase(const std::array<MT, 8>& labels,
  const vtkSurfaceNets3DNonManifoldCases::VoxelCaseType& voxelCase) -> std::array<CT, 8>
{
  std::array<CT, 8> coloring{};
  // tracks distinct labels seen so far, in order of first appearance
  std::array<MT, 8> seenLabels{};
  CT nextColor = 1; // 1-based, 0 is reserved for inactive corners

  for (size_t i = 0; i < 8; ++i)
  {
    // skip inactive corners
    if (!(voxelCase & (1 << i)))
    {
      continue;
    }
    // find the label in the seen labels, or register it as a new color
    CT foundColor = 0;
    for (CT j = 0; j < static_cast<CT>(nextColor - 1); ++j)
    {
      if (seenLabels[j] == labels[i])
      {
        foundColor = static_cast<CT>(j + 1); // 1-based
        break;
      }
    }
    // if not found, register a new color for this label
    if (foundColor == 0)
    {
      seenLabels[nextColor - 1] = labels[i];
      foundColor = nextColor++;
    }
    // assign the found color to the output coloring
    coloring[i] = foundColor;
  }
  return coloring;
}

/**
 * Checks whether all active corners of a voxel case share the same material label.
 *
 * @tparam T         The type of the material labels (e.g., uint8_t).
 * @param voxelCase  The voxel case bitmask identifying the active corners.
 * @param labels     The material labels at all 8 corners of the voxel.
 * @return True if all active corners share the same material label, false otherwise.
 */
template <class T>
auto AreAllMaterialsTheSame(
  vtkSurfaceNets3DNonManifoldCases::VoxelCaseType voxelCase, const std::array<T, 8>& labels) -> bool
{
  T representativeLabel = 0;
  bool firstFound = false;
  bool allPinchVoxelsIdentical = true;

  for (int i = 0; i < 8; ++i)
  {
    if (voxelCase & (1 << i)) // if this corner is active in the voxel case
    {
      if (!firstFound)
      {
        representativeLabel = labels[i];
        firstFound = true;
      }
      else if (labels[i] != representativeLabel)
      {
        allPinchVoxelsIdentical = false;
        break;
      }
    }
  }
  return allPinchVoxelsIdentical;
}
//------------------------------------------------------------------------------

//-----------------------Detect Non-Manifold Voxel Cases------------------------
/**
 * Represents a single non-manifold case match found within a color-encoded voxel case.
 * Stores both the non-manifold case index and the voxel case bitmask of the color group
 * that triggered the match, allowing the caller to identify which voxels are involved
 * in the non-manifold configuration.
 */
struct NonManifoldCaseMatch
{
  vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseType NonManifoldCase;
  vtkSurfaceNets3DNonManifoldCases::VoxelCaseType VoxelCase;
};

/**
 * Stores all non-manifold case matches found across all color groups in a color-encoded
 * voxel case. Each match corresponds to one color group whose voxel configuration matches
 * one of the 11 non-manifold cases. At most 4 matches can be found since there are 8 corners
 * and each non-manifold group requires at least 2 corners.
 */
class NonManifoldCaseMatches
{
public:
  /**
   * Adds a non-manifold case match, storing both the non-manifold case index and the
   * voxel case bitmask of the color group that triggered the match.
   *
   * @param nonManifoldCase The 1-based non-manifold case index.
   * @param voxelCase       The voxel case bitmask of the matched color group.
   */
  auto Add(vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseType nonManifoldCase,
    vtkSurfaceNets3DNonManifoldCases::VoxelCaseType voxelCase) -> void
  {
    assert(this->Size < 4);
    this->Matches[this->Size++] = { nonManifoldCase, voxelCase };
  }

  /**
   * Returns the number of non-manifold case matches found.
   */
  auto GetSize() const -> uint8_t { return this->Size; }

  /**
   * Returns the non-manifold case match at the given index.
   */
  auto operator[](size_t i) const -> const NonManifoldCaseMatch& { return this->Matches[i]; }

  /**
   * Returns the maximum non-manifold case match found among all matches, or 0 if no matches are
   * found.
   */
  auto GetMaxNonManifoldCaseMatch() const -> NonManifoldCaseMatch
  {
    NonManifoldCaseMatch maxMatch = { 0, 0 };
    for (size_t i = 0; i < this->Size; ++i)
    {
      if (this->Matches[i].NonManifoldCase > maxMatch.NonManifoldCase)
      {
        maxMatch = this->Matches[i];
      }
    }
    return maxMatch;
  }

private:
  std::array<NonManifoldCaseMatch, 4> Matches{};
  uint8_t Size = 0;
};

/**
 * Returns all non-manifold cases found for each color group in a color-encoded voxel case.
 * For each color group, the voxel case bitmask is computed and checked against all non-manifold
 * case tables. Groups of size less than 2 are skipped as they cannot match any non-manifold case.
 *
 * @tparam T                     The unsigned type of the color indices (e.g., uint8_t).
 * @param colorEncodedVoxelCase  A color-encoded voxel case of length 8, aligned with the
 *                               cube corners, as produced by ComputeColorEncodedVoxelCase()
 *                               or ExtractColorEncodedVoxelCase(). Color indices are 1-based;
 *                               0 indicates an inactive corner.
 * @return A NonManifoldCaseMatches object containing all non-manifold cases found, one per
 *         non-manifold color group
 */
template <class T = uint8_t>
auto GetNonManifoldCasesForColorEncodedVoxelCase(const std::array<T, 8>& colorEncodedVoxelCase)
  -> NonManifoldCaseMatches
{
  // Build per-group voxel case bitmasks in a single pass. Arrays are sized 9 (not 8) so we
  // can index directly by 1-based color value; slot 0 is unused.
  std::array<std::bitset<8>, 9> groupVoxelCases{};
  uint8_t numGroups = 0;

  for (uint8_t i = 0; i < 8; ++i)
  {
    const T color = colorEncodedVoxelCase[i];
    if (color > 0)
    {
      groupVoxelCases[color].set(i);
      if (color > numGroups)
      {
        numGroups = color;
      }
    }
  }
  assert(numGroups <= 8); // at most 8 groups since there are only 8 corners

  NonManifoldCaseMatches matches;

  // For each color group of size >= 2, look up its non-manifold case.
  for (uint8_t group = 1; group <= numGroups; ++group)
  {
    const auto& groupVoxelCase = groupVoxelCases[group];
    if (groupVoxelCase.count() < 2)
    {
      continue;
    }
    const NonManifoldCaseEntry& entry = VoxelCaseToNonManifold[groupVoxelCase.to_ulong()];
    if (entry.NonManifoldCaseIndex != 0)
    {
      matches.Add(entry.NonManifoldCaseIndex, entry.VoxelCase);
    }
  }
  return matches;
}

#ifdef VTK_SURFACE_NETS_3D_NON_MANIFOLD_CASES_CREATE_TABLE
/**
 * Checks whether a color-encoded voxel case contains any non-manifold color group.
 * This is a convenience wrapper around GetNonManifoldCasesForColorEncodedVoxelCase()
 * that returns true if at least one non-manifold group is found.
 *
 * @tparam T                     The unsigned type of the color indices (e.g., uint8_t).
 * @param colorEncodedVoxelCase  A color-encoded voxel case of length 8, aligned with the
 *                               cube corners, as produced by ComputeColorEncodedVoxelCase()
 *                               or ExtractColorEncodedVoxelCase(). Color indices are 1-based;
 *                               0 indicates an inactive corner.
 * @return True if at least one non-manifold group is found, false otherwise.
 */
template <class T = uint8_t>
auto IsNonManifoldCaseForColorEncodedVoxelCase(const std::array<T, 8>& colorEncodedVoxelCase)
  -> bool
{
  return GetNonManifoldCasesForColorEncodedVoxelCase(colorEncodedVoxelCase).GetSize() > 0;
}
#endif

/**
 * Checks whether a split (represented by its sub-voxel cases) is sufficient for a given
 * color-encoded voxel case. A split is sufficient if no face group contains an unsolvable
 * sub-non-manifold configuration based on the actual material assignment.
 *
 * A sub-non-manifold configuration within a face group is unsolvable if it matches
 * non-manifold cases 1-3, since these cases have no complementary split available.
 * Sub-non-manifold configurations matching cases 4-8 are still resolvable via their
 * complementary split and do not invalidate the split.
 *
 * This function is used at runtime to determine whether a candidate split stored in
 * the non-manifold metadata table is compatible with the actual material configuration
 * of the voxel neighborhood. It applies to both primary splits (cases 1-8) and splits
 * derived from complementary coords (cases 9-11).
 *
 * @tparam T                     The unsigned type of the color indices (e.g., uint8_t).
 * @param subVoxelCases          The voxel case bitmask for each face group in the split,
 *                               as stored in NonManifoldCaseMetadata. A value of 0
 *                               indicates an unused group slot.
 * @param colorEncodedVoxelCase  A color-encoded voxel case of length 8, aligned with the
 *                               cube corners, as produced by ExtractColorEncodedVoxelCase().
 *                               Color indices are 1-based; 0 indicates an inactive corner.
 * @return True if the split is valid for the given material configuration, false if any
 *         face group contains an unsolvable sub-non-manifold configuration of cases 1-3.
 */
template <class T>
auto IsSplitSufficientForColorEncodedVoxelCase(const std::array<uint8_t, 4>& subVoxelCases,
  const std::array<T, 8>& colorEncodedVoxelCase) -> bool
{
  // check each face group for unsolvable sub-non-manifold configurations
  for (uint8_t g = 0; g < 4; ++g)
  {
    if (subVoxelCases[g] == 0)
    {
      break; // unused group slot
    }

    // extract the color-encoded voxel case for this group's voxels only
    const auto subColorEncoded =
      ExtractColorEncodedVoxelCase(colorEncodedVoxelCase, subVoxelCases[g]);

    // check if any unsolvable sub-non-manifold (cases 1-3) exists within this group.
    // cases 1-3 have no complementary split, so their presence within a group makes
    // the primary split invalid. cases 4-8 can still be handled via their complementary
    // split and do not invalidate the primary split.
    const auto matches = GetNonManifoldCasesForColorEncodedVoxelCase(subColorEncoded);
    if (matches.GetSize() > 0 && matches.GetMaxNonManifoldCaseMatch().NonManifoldCase <= 3)
    {
      return false; // unsolvable sub-non-manifold found, split is invalid
    }
  }

  return true;
}
//------------------------------------------------------------------------------

//--------------------------Non-Manifold Edge Cases-----------------------------
/**
 * Computes the homogeneous edge case for a given voxel case bitmask. The homogeneous
 * edge case is the edge case where all active voxels share the same material — only
 * faces between active and inactive voxels are generated, and no faces between two
 * active voxels are generated.
 *
 * @param voxelCase The 8-bit voxel case bitmask identifying active corners.
 * @return The 12-bit homogeneous edge case for this voxel configuration.
 */
auto ComputeHomogeneousEdgeCase(uint8_t voxelCase) -> uint16_t
{
  uint16_t edgeCase = 0;
  uint8_t bit = 0;

  // precomputed voxel case bitmasks for each face's two voxels
  // yz-plane faces (bits 0-3): outer=z, inner=y, A=(0,y,z), B=(1,y,z)
  static constexpr uint8_t yzBitA[2][2] = { { 1, 4 }, { 16, 64 } };
  static constexpr uint8_t yzBitB[2][2] = { { 2, 8 }, { 32, 128 } };
  // yz-plane faces (bits 0-3)
  for (uint8_t z = 0; z < 2; ++z)
  {
    for (uint8_t y = 0; y < 2; ++y)
    {
      const bool aActive = (voxelCase & yzBitA[z][y]) != 0;
      const bool bActive = (voxelCase & yzBitB[z][y]) != 0;
      if (aActive != bActive)
      {
        edgeCase |= (1 << bit);
      }
      ++bit;
    }
  }

  // xz-plane faces (bits 4-7): outer=z, inner=x, A=(x,0,z), B=(x,1,z)
  static constexpr uint8_t xzBitA[2][2] = { { 1, 2 }, { 16, 32 } };
  static constexpr uint8_t xzBitB[2][2] = { { 4, 8 }, { 64, 128 } };
  // xz-plane faces (bits 4-7)
  for (uint8_t z = 0; z < 2; ++z)
  {
    for (uint8_t x = 0; x < 2; ++x)
    {
      const bool aActive = (voxelCase & xzBitA[z][x]) != 0;
      const bool bActive = (voxelCase & xzBitB[z][x]) != 0;
      if (aActive != bActive)
      {
        edgeCase |= (1 << bit);
      }
      ++bit;
    }
  }

  // xy-plane faces (bits 8-11): outer=y, inner=x, A=(x,y,0), B=(x,y,1)
  static constexpr uint8_t xyBitA[2][2] = { { 1, 2 }, { 4, 8 } };
  static constexpr uint8_t xyBitB[2][2] = { { 16, 32 }, { 64, 128 } };
  // xy-plane faces (bits 8-11)
  for (uint8_t y = 0; y < 2; ++y)
  {
    for (uint8_t x = 0; x < 2; ++x)
    {
      const bool aActive = (voxelCase & xyBitA[y][x]) != 0;
      const bool bActive = (voxelCase & xyBitB[y][x]) != 0;
      if (aActive != bActive)
      {
        edgeCase |= (1 << bit);
      }
      ++bit;
    }
  }

  return edgeCase;
}

#ifdef VTK_SURFACE_NETS_3D_NON_MANIFOLD_CASES_CREATE_TABLE
/**
 * Computes the edge case for the coordinates of a voxel cas under a given coloring (material
 * assignment).
 *
 * Each of the 12 face bits is resolved by examining the two voxels sharing that face
 * and their material colors:
 *   - Neither present               → 0 (no face)
 *   - One present, one absent       → 1 (boundary with background)
 *   - Both present, same color      → 0 (same material, no boundary)
 *   - Both present, different color → 1 (material boundary)
 *
 * The face ordering follows the following convention:
 *   bits 0–3:  yz-plane faces ((0,y,z), (1,y,z))
 *   bits 4–7:  xz-plane faces ((x,0,z), (x,1,z))
 *   bits 8–11: xy-plane faces ((x,y,0), (x,y,1))
 *
 * @tparam N              The number of voxel case's coordinates.
 * @tparam T              The unsigned type of the color indices (e.g., uint8_t). Must be
 *                        unsigned so that (T)-1 is a valid sentinel for absent voxels.
 * @param voxelCaseCoords The voxel case's coordinates.
 * @param coloring        A 1-based color assignment of length N, where elements with the
 *                        same color index share a material label, as produced by
 *                        ColoredVariations(). Color index 0 is reserved as a sentinel for
 *                        inactive corners and must not appear in this array.
 * @return The single 12-bit edge case integer for this colored voxel case.
 */
template <size_t N, class T = uint8_t>
auto ComputeEdgeCaseForColoring(
  const VoxelCaseCoords<N>& voxelCaseCoords, const std::array<T, N>& coloring) -> uint16_t
{
  // Use (T)-1 as a sentinel for "not present". Safe as long as the number of
  // materials never reaches std::numeric_limits<T>::max().
  static_assert(std::is_unsigned_v<T>, "T must be unsigned for sentinel (T)-1 to be valid");

  // Returns the color of coord c, or (T)-1 if c is not in the voxel case's coordinates
  auto colorOf = [&](const Coord& c) -> T
  {
    const auto it = std::find(voxelCaseCoords.begin(), voxelCaseCoords.end(), c);
    if (it == voxelCaseCoords.end())
    {
      return static_cast<T>(-1);
    }
    return coloring[static_cast<size_t>(std::distance(voxelCaseCoords.begin(), it))];
  };

  // Resolves a single face pair (a, b) to 0 or 1 based on presence and material
  auto resolveFace = [&](const Coord& a, const Coord& b) -> bool
  {
    const T ca = colorOf(a);
    const T cb = colorOf(b);
    const bool aPresent = ca != static_cast<T>(-1);
    const bool bPresent = cb != static_cast<T>(-1);
    if (!aPresent && !bPresent)
    {
      return false; // neither present: no face
    }
    if (!aPresent || !bPresent)
    {
      return true; // one absent: boundary with background
    }
    return ca != cb; // both present: face iff different materials
  };

  std::bitset<12> faceArray{};
  int bit = 0;

  // yz-plane faces (bits 0–3): ((0,y,z),(1,y,z))
  for (uint8_t z : { 0, 1 })
  {
    for (uint8_t y : { 0, 1 })
    {
      faceArray[bit++] = resolveFace({ 0, y, z }, { 1, y, z });
    }
  }
  // xz-plane faces (bits 4–7): ((x,0,z),(x,1,z))
  for (uint8_t z : { 0, 1 })
  {
    for (uint8_t x : { 0, 1 })
    {
      faceArray[bit++] = resolveFace({ x, 0, z }, { x, 1, z });
    }
  }
  // xy-plane faces (bits 8–11): ((x,y,0),(x,y,1))
  for (uint8_t y : { 0, 1 })
  {
    for (uint8_t x : { 0, 1 })
    {
      faceArray[bit++] = resolveFace({ x, y, 0 }, { x, y, 1 });
    }
  }
  return static_cast<uint16_t>(faceArray.to_ulong());
}

/**
 * Computes all distinct edge cases for a voxel case's coordinates across all possible material
 * assignments.
 *
 * The voxel case's coordinates define which of the 8 voxel corners are active. Each active
 * corner can be assigned a material label, and different label assignments produce
 * different edge cases — specifically, a face between two active voxels is generated
 * only when they carry different material labels.
 *
 * All possible material assignments are enumerated using ColoredVariations(), which
 * generates all non-trivial set partitions of the active voxels via restricted growth
 * strings. The trivial all-same-material assignment is always included as the base case.
 * Each assignment is resolved to a single deterministic edge case by
 * ComputeEdgeCaseForColoring(). Duplicate edge cases (produced by symmetrically
 * equivalent colorings) are removed after sorting.
 *
 * The number of colorings checked is B(n) where n is the number of active voxels and
 * B(n) is the Bell number. For the largest cases (n=6), this is B(6)=203 colorings.
 *
 * @tparam N              The number of voxel case's coordinates.
 * @param voxelCaseCoords The voxel case's coordinates.
 * @return A sorted, deduplicated vector of all distinct 12-bit edge case integers
 *         reachable by any valid material assignment of this voxel case.
 */
template <size_t N>
auto ComputeEdgeCases(const VoxelCaseCoords<N>& voxelCaseCoords) -> std::vector<uint16_t>
{
  std::vector<uint16_t> edgeCases;

  // colorings: each produces a deterministic edge case
  for (const auto& coloring : ColoredVariations<N>())
  {
    auto colorEncodedVoxelCase = ComputeColorEncodedVoxelCase(voxelCaseCoords, coloring);
    if (IsNonManifoldCaseForColorEncodedVoxelCase(colorEncodedVoxelCase))
    {
      edgeCases.emplace_back(ComputeEdgeCaseForColoring(voxelCaseCoords, coloring));
    }
  }

  if (!std::is_sorted(edgeCases.begin(), edgeCases.end()))
  {
    std::sort(edgeCases.begin(), edgeCases.end());
  }
  edgeCases.erase(std::unique(edgeCases.begin(), edgeCases.end()), edgeCases.end());
  return edgeCases;
}
//------------------------------------------------------------------------------

//------------------Compute Face Point Indices and sub-edge cases---------------
/**
 * Groups voxels into connected components where two voxels are directly connected if their
 * Hamming distance is exactly dMax. A group grows by repeatedly adding any ungrouped voxel that is
 * directly connected to at least one voxel already in the group. This continues until no more
 * voxels can be added, then a new group is started with the next ungrouped voxel. Note that voxels
 * within the same group do not need to be directly connected to each other — they only need to be
 * reachable through a path of direct connections.
 *
 * @tparam N              The number of voxel case's coordinates.
 * @param voxelCaseCoords The voxel case's coordinates.
 * @param dMax            The Hamming distance defining direct connectivity between voxels.
 * @return Groups of connected components, where each component is a vector of voxel coordinates.
 */
template <size_t N>
auto GroupByHammingThreshold(const VoxelCaseCoords<N>& voxelCaseCoords, size_t dMax = 1)
  -> std::vector<std::vector<Coord>>
{
  std::array<bool, N> grouped{}; // initialized to false
  std::vector<std::vector<Coord>> groups;

  for (size_t start = 0; start < voxelCaseCoords.size(); ++start)
  {
    if (grouped[start])
    {
      continue;
    }

    // Start a new group with this voxel
    std::vector<Coord> group;
    group.push_back(voxelCaseCoords[start]);
    grouped[start] = true;

    bool changed = true;
    while (changed)
    {
      changed = false;
      for (size_t i = 0; i < voxelCaseCoords.size(); ++i)
      {
        if (grouped[i])
        {
          continue;
        }
        // Add voxel if it has at least one neighbor in the group at exactly dMax distance
        // and the face between them is not generated (not active in the edge case)
        for (const auto& coord : group)
        {
          if (HammingDistance(voxelCaseCoords[i], coord) == dMax)
          {
            group.push_back(voxelCaseCoords[i]);
            grouped[i] = true;
            changed = true;
            break;
          }
        }
      }
    }

    groups.push_back(group);
  }

  return groups;
}

/**
 * Computes the 12 point indices for the duplicate points of each face connected to the center
 * vertex, given a voxel subset and an edge case mask. Each index identifies which group (as
 * returned by GroupByHammingThreshold with dMax=1) the face belongs to, which determines which
 * duplicate point should be used for that face. A value of -1 means no face is generated for
 * that bit.
 *
 * @param groups            The face-connected groups of voxels, as produced by
 *                          GroupByHammingThreshold() with dMax=1.
 * @param edgeCase          A 12-bit mask where each bit indicates whether a face is generated.
 * @return A 12-element array of point indices, one per face bit, with -1 for ungenerated faces.
 */
auto ComputeFacePointIndices(const std::vector<std::vector<Coord>>& groups, uint16_t edgeCase)
  -> std::array<int8_t, 12>
{
  auto inGroup = [](const std::vector<Coord>& group, const Coord& c) -> bool
  { return std::find(group.begin(), group.end(), c) != group.end(); };

  std::array<int8_t, 12> pointIndices{};
  pointIndices.fill(-1);
  size_t bit = 0;

  // yz-plane faces (bits 0–3): pairs ((0,y,z),(1,y,z))
  for (uint8_t z : { 0, 1 })
  {
    for (uint8_t y : { 0, 1 })
    {
      if ((edgeCase >> bit) & 1) // if this face is generated
      {
        for (size_t idx = 0; idx < groups.size(); ++idx)
        {
          const auto& group = groups[idx];
          if (inGroup(group, { 0, y, z }) || inGroup(group, { 1, y, z }))
          {
            pointIndices[bit] = static_cast<int8_t>(idx);
            break;
          }
        }
      }
      ++bit;
    }
  }

  // xz-plane faces (bits 4–7): pairs ((x,0,z),(x,1,z))
  for (uint8_t z : { 0, 1 })
  {
    for (uint8_t x : { 0, 1 })
    {
      if ((edgeCase >> bit) & 1) // if this face is generated
      {
        for (size_t idx = 0; idx < groups.size(); ++idx)
        {
          const auto& group = groups[idx];
          if (inGroup(group, { x, 0, z }) || inGroup(group, { x, 1, z }))
          {
            pointIndices[bit] = static_cast<int8_t>(idx);
            break;
          }
        }
      }
      ++bit;
    }
  }

  // xy-plane faces (bits 8–11): pairs ((x,y,0),(x,y,1))
  for (uint8_t y : { 0, 1 })
  {
    for (uint8_t x : { 0, 1 })
    {
      if ((edgeCase >> bit) & 1) // if this face is generated
      {
        for (size_t idx = 0; idx < groups.size(); ++idx)
        {
          const auto& group = groups[idx];
          if (inGroup(group, { x, y, 0 }) || inGroup(group, { x, y, 1 }))
          {
            pointIndices[bit] = static_cast<int8_t>(idx);
            break;
          }
        }
      }
      ++bit;
    }
  }

  return pointIndices;
}

/**
 * Converts face-connected groups of voxels into voxel case bitmasks. Each group is converted
 * to an 8-bit voxel case bitmask where bit i is set if Coords[i] is present in that group.
 * Returns a fixed-size array of 4 masks, padded with zeros for unused groups.
 *
 * @param groups The face-connected groups of voxels, as produced by GroupByHammingThreshold()
 *               with dMax=1.
 * @return An array of up to 4 voxel case bitmasks, one per group.
 */
auto ComputeSubVoxelCases(const std::vector<std::vector<Coord>>& groups) -> std::array<uint8_t, 4>
{
  std::array<uint8_t, 4> subVoxelCases{}; // initialized to 0 for non-existent groups
  for (size_t i = 0; i < std::min(groups.size(), subVoxelCases.size()); ++i)
  {
    subVoxelCases[i] = ComputeVoxelCase(groups[i]);
  }
  return subVoxelCases;
}

/**
 * Given an array of face point indices, computes up to 4 sub-edge-case masks corresponding
 * to the face-connected groups. Each sub-mask is a 12-bit integer where bit j is set if
 * face j belongs to that group. Sub-masks for groups that don't exist are set to 0.
 *
 * @param pointIndices A 12-element array where each element holds the group index of that
 *                     face, or -1 if no face is generated for that bit position, as produced
 *                     by ComputeFacePointIndices().
 * @return An array of up to 4 sub-masks, one per face-connected group, as 12-bit integers.
 */
auto ComputeSubEdgeCases(const std::array<int8_t, 12>& pointIndices) -> std::array<uint16_t, 4>
{
  // Find the number of groups (max group id + 1)
  const int8_t maxGroupId = *std::max_element(pointIndices.begin(), pointIndices.end());
  const size_t numberOfGroups = maxGroupId >= 0 ? static_cast<size_t>(maxGroupId) + 1 : 0;

  std::array<uint16_t, 4> subEdgeCases{}; // initialized to 0 for non-existent groups
  for (size_t i = 0; i < numberOfGroups; ++i)
  {
    // Build a 12-bit mask where bit j is set if face j belongs to group i
    std::bitset<12> subMaskBits;
    for (int8_t j = 0; j < 12; ++j)
    {
      subMaskBits[j] = pointIndices[j] == i;
    }
    subEdgeCases[i] = static_cast<uint16_t>(subMaskBits.to_ulong());
  }

  return subEdgeCases;
}
//------------------------------------------------------------------------------

/**
 * Builds a compact lookup table mapping every possible 12-bit edge case to its non-manifold
 * case metadata. The table uses a flat array structure with offsets to avoid wasting memory:
 *
 *   1. A [4097] offset array where offsets[edgeCase] gives the start index into the metadata
 *      array for that edge case, and offsets[edgeCase+1] - offsets[edgeCase] gives the number
 *      of metadata entries. A size of 0 indicates a manifold or geometrically impossible edge
 *      case.
 *   2. A flat [3736] array of NonManifoldCaseMetadata containing all metadata entries for all
 *      non-manifold edge cases, laid out contiguously in edge case order.
 *
 * Out of the 4096 theoretically possible 12-bit edge cases:
 *   - Only 1097 are geometrically reachable from any voxel configuration.
 *   - Only 564 of those can be non-manifold for some material assignment.
 *   - The remaining 3532 are always manifold and have size 0 in the offset array.
 *
 * An edge case is a 12-bit integer where each bit corresponds to one of the 12 faces of a
 * voxel neighborhood. A bit is set when the two voxels sharing that face belong to different
 * material regions, generating a surface patch between them. Most edge cases are manifold —
 * a single surface point at the voxel center suffices. Some edge cases correspond to
 * non-manifold topologies where multiple material regions meet at a pinch point, requiring
 * duplicate points — one per face-connected group of the local surface.
 *
 * For each non-manifold case (1-11 predefined, plus manifold voxel configurations that become
 * non-manifold under certain material assignments), all distinct non-manifold edge cases are
 * computed via ComputeEdgeCases(), which enumerates all possible material assignments of the
 * active voxels and retains only those that produce a non-manifold configuration.
 *
 * For non-manifold cases 1-11, the first edge case (index 0) corresponds to the homogeneous
 * configuration (all active voxels share the same material) and is marked with IsHomogeneous=true.
 * For manifold voxel configurations (cases 12+), the homogeneous configuration is always manifold,
 * so all stored edge cases correspond to material variations and are marked IsHomogeneous=false.
 *
 * For each edge case, the face-connected groups are computed via GroupByHammingThreshold()
 * with dMax=1, which groups voxels purely by geometric face-connectivity regardless of material.
 * The resulting groups determine the SubVoxelCases (voxel bitmask per group), PointIndices
 * (face-to-group assignment), and SubEdgeCases (edge case per group).
 *
 * @return A pair of:
 *   - A [4097] array of uint16_t offsets where offsets[edgeCase] is the start index into the
 *     metadata array, and offsets[edgeCase+1] - offsets[edgeCase] is the number of entries.
 *   - A flat array of NonManifoldCaseMetadata containing all metadata entries contiguously.
 */
auto CreateNonManifoldMetaDataPerEdgeCase() -> std::pair<std::array<uint16_t, 4097>,
  std::array<vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseMetadata, 3736>>
{
  // first pass: compute sizes per edge case
  std::array<uint16_t, 4096> size{};
  std::cout << "  std::array<uint16_t, 4096> size{};  // initialized to 0\n";
  std::apply(
    [&](auto&&... nonManifoldCases)
    {
      vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseType nonManifoldCaseIndex = 1;
      (
        [&](auto&& nonManifoldCase)
        {
          for (const auto& [voxelCaseCoords, voxelCase] : nonManifoldCase)
          {
            const std::vector<uint16_t> edgeCases = ComputeEdgeCases(voxelCaseCoords);
            for (const auto& edgeCase : edgeCases)
            {
              ++size[edgeCase];
            }
          }
          ++nonManifoldCaseIndex;
        }(nonManifoldCases),
        ...);
    },
    AllCases);

  // print sizes for constexpr version — only non-zero entries (564 of them)
  std::cout << "  // Edge case sizes\n";
  for (size_t edgeCase = 0; edgeCase < 4096; ++edgeCase)
  {
    if (size[edgeCase] > 0)
    {
      std::cout << "  size[" << edgeCase << "] = " << size[edgeCase] << ";\n";
    }
  }

  // compute offsets from sizes via prefix sum
  std::array<uint16_t, 4097> offsets{};
  for (size_t i = 0; i < 4096; ++i)
  {
    offsets[i + 1] = offsets[i] + size[i];
  }
  std::cout << "  std::array<uint16_t, 4097> offsets{};\n";
  std::cout << "  for (size_t i = 0; i < 4096; ++i)\n";
  std::cout << "  {\n";
  std::cout << "    offsets[i + 1] = offsets[i] + size[i];\n";
  std::cout << "  }\n";

  // total number of metadata entries
  const uint16_t totalEntries = offsets[4096];
  assert(3736 == totalEntries);
  // print total entries
  std::cout << "  // Total metadata entries: " << totalEntries << std::endl;
  // flat metadata array
  std::array<vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseMetadata, 3736> metadata;
  std::cout << "  std::array<vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseMetadata, 3736> "
               "metadata{};\n";

  // current insertion index per edge case — starts at offsets[edgeCase]
  std::array<uint8_t, 4096> current{};

  // second pass: fill metadata entries
  std::apply(
    [&](auto&&... nonManifoldCases)
    {
      vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseType nonManifoldCaseIndex = 1;
      (
        [&](auto&& nonManifoldCase)
        {
          std::cout << "  // Processing non-manifold case "
                    << static_cast<int>(nonManifoldCaseIndex) << " with " << nonManifoldCase.size()
                    << " voxel cases\n";
          // For each non-manifold case, iterate over all its voxel configurations
          vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseMetadata meta;
          meta.NonManifoldCase = nonManifoldCaseIndex;
          for (const auto& [voxelCaseCoords, voxelCase] : nonManifoldCase)
          {
            // Store the voxel case bitmask of the voxel case's coordinates
            meta.VoxelCase = voxelCase;
            // Compute all distinct non-manifold edge cases for the voxel case's coordinates across
            // all possible material variations.
            const std::vector<uint16_t> edgeCases = ComputeEdgeCases(voxelCaseCoords);
            if (edgeCases.empty())
            {
              continue;
            }
            // For nonManifoldCaseIndex <= 11, the first edge case corresponds to the homogeneous
            // voxel configuration.
            //
            // For nonManifoldCaseIndex >= 12, the homogeneous voxel configuration is always
            // manifold but material variations can convert them to non-manifold, so the first edge
            // case which is non-manifold is not homogeneous.
            //
            // The rest of the edge cases which are all material variation edge cases — each
            // corresponds to a distinct material assignment of the active voxels, producing a
            // unique edge case. Entries are inserted incrementally into the table at the next
            // available slot for that edge case.
            const auto faceConnectedGroups = GroupByHammingThreshold(voxelCaseCoords);
            meta.SubVoxelCases = ComputeSubVoxelCases(faceConnectedGroups);
            for (size_t i = 0; i < edgeCases.size(); ++i)
            {
              const auto& edgeCase = edgeCases[i];
              meta.IsHomogeneous = i == 0 && nonManifoldCaseIndex <= 11;
              meta.PointIndices = ComputeFacePointIndices(faceConnectedGroups, edgeCase);
              meta.SubEdgeCases = ComputeSubEdgeCases(meta.PointIndices);
              std::cout << "  metadata[offsets[" << edgeCase << "] + "
                        << static_cast<int>(current[edgeCase]) << "] = { "
                        << static_cast<int>(meta.NonManifoldCase) << ", "
                        << static_cast<int>(meta.VoxelCase) << ", "
                        << (meta.IsHomogeneous ? "true" : "false") << ", { { ";
              for (int j = 0; j < 12; ++j)
              {
                std::cout << static_cast<int>(meta.PointIndices[j]) << (j < 11 ? ", " : "");
              }
              std::cout << " } }, { { ";
              for (int j = 0; j < 4; ++j)
              {
                std::cout << static_cast<int>(meta.SubVoxelCases[j]) << (j < 3 ? ", " : "");
              }
              std::cout << " } }, { { ";
              for (int j = 0; j < 4; ++j)
              {
                std::cout << static_cast<int>(meta.SubEdgeCases[j]) << (j < 3 ? ", " : "");
              }
              std::cout << " } } };\n";
              metadata[offsets[edgeCase] + current[edgeCase]++] = meta;
            }
          }
          ++nonManifoldCaseIndex;
        }(nonManifoldCases),
        ...);
    },
    AllCases);

  // collect statistics
  std::map<size_t, size_t> tableEntrySizeCount;
  for (size_t edgeCase = 0; edgeCase < 4096; ++edgeCase)
  {
    const size_t entrySize = offsets[edgeCase + 1] - offsets[edgeCase];
    auto result = tableEntrySizeCount.find(entrySize);
    if (result != tableEntrySizeCount.end())
    {
      result->second++;
    }
    else
    {
      tableEntrySizeCount[entrySize] = 1;
    }
  }
  // print statistics
  for (const auto& entry : tableEntrySizeCount)
  {
    std::cout << entry.second << " edge cases have " << entry.first << " metadata entries.\n";
  }
  // exit(0);
  return { offsets, metadata };
}
const auto [Offsets, Metadata] = CreateNonManifoldMetaDataPerEdgeCase();

// create a function to extensively test the maximum Table Index
template <size_t N>
auto ComputeTableIndices(const VoxelCaseCoords<N>& voxelCaseCoords) -> std::vector<int8_t>
{
  std::vector<int8_t> tableIndices;
  vtkSurfaceNets3DNonManifoldCases::VoxelNeighborhood<uint8_t> voxelNeighborhood;

  // colorings: each produces a deterministic edge case
  for (const auto& coloring : ColoredVariations<N>())
  {
    auto colorEncodedVoxelCase = ComputeColorEncodedVoxelCase(voxelCaseCoords, coloring);
    // if (IsNonManifoldCaseForColorEncodedVoxelCase(colorEncodedVoxelCase))
    {
      voxelNeighborhood.EdgeCase = ComputeEdgeCaseForColoring(voxelCaseCoords, coloring);
      voxelNeighborhood.VoxelCase = ComputeVoxelCase(voxelCaseCoords);
      voxelNeighborhood.Labels = colorEncodedVoxelCase;
      // print labels
      for (size_t i = 0; i < 8; ++i)
      {
        std::cout << static_cast<int>(voxelNeighborhood.Labels[i]) << " ";
      }
      std::cout << std::endl;
      tableIndices.emplace_back(
        vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(voxelNeighborhood));
    }
  }
  return tableIndices;
}
auto TestTableIndexAllVoxelConfigurations() -> bool
{
  std::map<int8_t, size_t> countPerTableIndex;
  std::apply(
    [&](auto&&... nonManifoldCases)
    {
      vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseType nonManifoldCaseIndex = 1;
      (
        [&](auto&& nonManifoldCase)
        {
          std::cout << "Testing non-manifold case " << static_cast<int>(nonManifoldCaseIndex)
                    << "\n";
          for (const auto& [voxelCaseCoords, voxelCase] : nonManifoldCase)
          {
            auto indices = ComputeTableIndices(voxelCaseCoords);
            std::cout << "  ManifoldCaseIndex " << static_cast<int>(nonManifoldCaseIndex)
                      << " Voxel case " << static_cast<int>(voxelCase) << " has " << indices.size()
                      << " non-manifold table indices {";
            for (const auto& index : indices)
            {
              std::cout << static_cast<int>(index) << " ";
              if (countPerTableIndex.find(index) == countPerTableIndex.end())
              {
                countPerTableIndex[index] = 1;
              }
              else
              {
                ++countPerTableIndex[index];
              }
            }
            std::cout << "}\n";
          }
          ++nonManifoldCaseIndex;
        }(nonManifoldCases),
        ...);
    },
    AllCases);
  // print count per index
  for (const auto& [index, count] : countPerTableIndex)
  {
    std::cout << "Table index " << static_cast<int>(index) << " appears " << count << " times\n";
  }
  // Developer note (diagnostic, not normative): the histogram below is useful to understand how
  // often GetNonManifoldTableIndex() returns manifold/unsolvable/solvable indices when iterating
  // across the enumerated voxel configurations and their material colorings.
  //
  // The exact numbers depend on compile-time switches such as
  // VTK_SURFACE_NETS_3D_ALWAYS_SPLIT_CASES_1_3 and
  // VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8.
  //
  // Output from one run (with support for all solvable variations of case 8 enabled):
  //   Table index -2 appears 5784 times   (ManifoldIndex)
  //   Table index -1 appears 14174 times (NonManifoldIndexUnsolvable)
  //   Table index 0 appears 884 times
  //   Table index 1 appears 256 times
  //   Table index 2 appears 6 times
  //   Table index 3 appears 6 times
  //   Table index 4 appears 6 times
  //   Table index 5 appears 6 times
  //   Table index 6 appears 6 times
  //   Table index 7 appears 6 times
  //   Table index 8 appears 6 times
  //   Table index 9 appears 6 times
  exit(0);
  return true;
}
[[maybe_unused]] const bool exhaustiveTesting = TestTableIndexAllVoxelConfigurations();
#else
constexpr auto CreateNonManifoldMetaDataPerEdgeCase() -> std::pair<std::array<uint16_t, 4097>,
  std::array<vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseMetadata, 3736>>
{
  // clang-format off
  std::array<uint16_t, 4096> size{};  // initialized to 0
  // Edge case sizes
  size[255] = 7;
  size[443] = 5;
  size[447] = 3;
  size[494] = 5;
  size[495] = 3;
  size[507] = 3;
  size[510] = 3;
  size[511] = 9;
  size[635] = 5;
  size[639] = 3;
  size[734] = 5;
  size[735] = 3;
  size[763] = 3;
  size[766] = 3;
  size[767] = 9;
  size[831] = 7;
  size[869] = 5;
  size[879] = 3;
  size[885] = 3;
  size[891] = 3;
  size[895] = 9;
  size[917] = 5;
  size[927] = 3;
  size[949] = 3;
  size[955] = 3;
  size[959] = 9;
  size[975] = 7;
  size[981] = 3;
  size[990] = 3;
  size[991] = 9;
  size[997] = 3;
  size[1006] = 3;
  size[1007] = 9;
  size[1013] = 8;
  size[1019] = 8;
  size[1022] = 8;
  size[1023] = 13;
  size[1207] = 5;
  size[1215] = 3;
  size[1261] = 5;
  size[1263] = 3;
  size[1271] = 3;
  size[1277] = 3;
  size[1279] = 9;
  size[1366] = 5;
  size[1367] = 3;
  size[1369] = 5;
  size[1371] = 3;
  size[1373] = 3;
  size[1374] = 3;
  size[1375] = 8;
  size[1463] = 3;
  size[1467] = 3;
  size[1471] = 8;
  size[1517] = 3;
  size[1518] = 3;
  size[1519] = 8;
  size[1523] = 7;
  size[1526] = 3;
  size[1527] = 9;
  size[1529] = 3;
  size[1531] = 9;
  size[1532] = 7;
  size[1533] = 9;
  size[1534] = 9;
  size[1535] = 13;
  size[1587] = 5;
  size[1599] = 3;
  size[1641] = 5;
  size[1647] = 3;
  size[1655] = 3;
  size[1659] = 7;
  size[1661] = 3;
  size[1663] = 5;
  size[1686] = 5;
  size[1695] = 3;
  size[1719] = 7;
  size[1723] = 3;
  size[1726] = 3;
  size[1727] = 5;
  size[1740] = 5;
  size[1743] = 3;
  size[1751] = 3;
  size[1757] = 3;
  size[1758] = 7;
  size[1759] = 5;
  size[1771] = 3;
  size[1773] = 7;
  size[1774] = 3;
  size[1775] = 5;
  size[1779] = 3;
  size[1782] = 3;
  size[1783] = 5;
  size[1785] = 3;
  size[1787] = 5;
  size[1788] = 3;
  size[1789] = 5;
  size[1790] = 5;
  size[1791] = 12;
  size[1837] = 5;
  size[1839] = 3;
  size[1843] = 3;
  size[1853] = 3;
  size[1855] = 9;
  size[1895] = 3;
  size[1899] = 3;
  size[1901] = 7;
  size[1903] = 5;
  size[1910] = 3;
  size[1911] = 11;
  size[1912] = 5;
  size[1913] = 7;
  size[1914] = 3;
  size[1915] = 8;
  size[1916] = 3;
  size[1917] = 8;
  size[1918] = 8;
  size[1919] = 14;
  size[1927] = 5;
  size[1935] = 3;
  size[1943] = 7;
  size[1949] = 3;
  size[1950] = 3;
  size[1951] = 5;
  size[1959] = 3;
  size[1965] = 3;
  size[1967] = 8;
  size[1974] = 3;
  size[1975] = 8;
  size[1979] = 8;
  size[1981] = 8;
  size[1982] = 4;
  size[1983] = 12;
  size[1991] = 3;
  size[1996] = 3;
  size[1999] = 9;
  size[2002] = 5;
  size[2003] = 3;
  size[2006] = 7;
  size[2007] = 8;
  size[2009] = 3;
  size[2010] = 3;
  size[2011] = 8;
  size[2013] = 11;
  size[2014] = 8;
  size[2015] = 14;
  size[2023] = 8;
  size[2025] = 3;
  size[2027] = 4;
  size[2029] = 8;
  size[2030] = 8;
  size[2031] = 12;
  size[2034] = 3;
  size[2035] = 9;
  size[2038] = 5;
  size[2039] = 14;
  size[2040] = 3;
  size[2041] = 5;
  size[2042] = 8;
  size[2043] = 12;
  size[2044] = 9;
  size[2045] = 14;
  size[2046] = 12;
  size[2047] = 20;
  size[2167] = 5;
  size[2175] = 3;
  size[2269] = 5;
  size[2271] = 3;
  size[2295] = 3;
  size[2301] = 3;
  size[2303] = 9;
  size[2355] = 5;
  size[2367] = 3;
  size[2406] = 5;
  size[2415] = 3;
  size[2423] = 7;
  size[2427] = 3;
  size[2430] = 3;
  size[2431] = 5;
  size[2457] = 5;
  size[2463] = 3;
  size[2487] = 3;
  size[2491] = 7;
  size[2493] = 3;
  size[2495] = 5;
  size[2508] = 5;
  size[2511] = 3;
  size[2523] = 3;
  size[2525] = 7;
  size[2526] = 3;
  size[2527] = 5;
  size[2535] = 3;
  size[2541] = 3;
  size[2542] = 7;
  size[2543] = 5;
  size[2547] = 3;
  size[2550] = 3;
  size[2551] = 5;
  size[2553] = 3;
  size[2555] = 5;
  size[2556] = 3;
  size[2557] = 5;
  size[2558] = 5;
  size[2559] = 12;
  size[2679] = 3;
  size[2683] = 3;
  size[2687] = 8;
  size[2726] = 5;
  size[2727] = 3;
  size[2729] = 5;
  size[2731] = 3;
  size[2733] = 3;
  size[2734] = 3;
  size[2735] = 8;
  size[2781] = 3;
  size[2782] = 3;
  size[2783] = 8;
  size[2803] = 7;
  size[2806] = 3;
  size[2807] = 9;
  size[2809] = 3;
  size[2811] = 9;
  size[2812] = 7;
  size[2813] = 9;
  size[2814] = 9;
  size[2815] = 13;
  size[2845] = 5;
  size[2847] = 3;
  size[2867] = 3;
  size[2877] = 3;
  size[2879] = 9;
  size[2887] = 5;
  size[2895] = 3;
  size[2903] = 3;
  size[2909] = 3;
  size[2911] = 8;
  size[2919] = 7;
  size[2925] = 3;
  size[2926] = 3;
  size[2927] = 5;
  size[2934] = 3;
  size[2935] = 8;
  size[2939] = 8;
  size[2941] = 8;
  size[2942] = 4;
  size[2943] = 12;
  size[2967] = 3;
  size[2971] = 3;
  size[2973] = 7;
  size[2975] = 5;
  size[2998] = 3;
  size[2999] = 11;
  size[3000] = 5;
  size[3001] = 7;
  size[3002] = 3;
  size[3003] = 8;
  size[3004] = 3;
  size[3005] = 8;
  size[3006] = 8;
  size[3007] = 14;
  size[3015] = 3;
  size[3020] = 3;
  size[3023] = 9;
  size[3031] = 8;
  size[3033] = 3;
  size[3035] = 4;
  size[3037] = 8;
  size[3038] = 8;
  size[3039] = 12;
  size[3042] = 5;
  size[3043] = 3;
  size[3046] = 7;
  size[3047] = 8;
  size[3049] = 3;
  size[3050] = 3;
  size[3051] = 8;
  size[3053] = 11;
  size[3054] = 8;
  size[3055] = 14;
  size[3058] = 3;
  size[3059] = 9;
  size[3062] = 5;
  size[3063] = 14;
  size[3064] = 3;
  size[3065] = 5;
  size[3066] = 8;
  size[3067] = 12;
  size[3068] = 9;
  size[3069] = 14;
  size[3070] = 12;
  size[3071] = 20;
  size[3135] = 7;
  size[3178] = 5;
  size[3183] = 3;
  size[3191] = 3;
  size[3194] = 3;
  size[3199] = 9;
  size[3226] = 5;
  size[3231] = 3;
  size[3255] = 3;
  size[3258] = 3;
  size[3263] = 9;
  size[3279] = 7;
  size[3290] = 3;
  size[3293] = 3;
  size[3295] = 9;
  size[3306] = 3;
  size[3309] = 3;
  size[3311] = 9;
  size[3319] = 8;
  size[3322] = 8;
  size[3325] = 8;
  size[3327] = 13;
  size[3374] = 5;
  size[3375] = 3;
  size[3379] = 3;
  size[3390] = 3;
  size[3391] = 9;
  size[3431] = 3;
  size[3435] = 3;
  size[3438] = 7;
  size[3439] = 5;
  size[3444] = 5;
  size[3445] = 3;
  size[3446] = 7;
  size[3447] = 8;
  size[3449] = 3;
  size[3451] = 11;
  size[3452] = 3;
  size[3453] = 8;
  size[3454] = 8;
  size[3455] = 14;
  size[3467] = 5;
  size[3471] = 3;
  size[3483] = 7;
  size[3485] = 3;
  size[3486] = 3;
  size[3487] = 5;
  size[3499] = 3;
  size[3502] = 3;
  size[3503] = 8;
  size[3511] = 8;
  size[3513] = 3;
  size[3515] = 8;
  size[3517] = 4;
  size[3518] = 8;
  size[3519] = 12;
  size[3531] = 3;
  size[3532] = 3;
  size[3535] = 9;
  size[3537] = 5;
  size[3539] = 3;
  size[3541] = 3;
  size[3542] = 3;
  size[3543] = 8;
  size[3545] = 7;
  size[3547] = 8;
  size[3549] = 8;
  size[3550] = 11;
  size[3551] = 14;
  size[3558] = 3;
  size[3559] = 4;
  size[3563] = 8;
  size[3565] = 8;
  size[3566] = 8;
  size[3567] = 12;
  size[3569] = 3;
  size[3571] = 9;
  size[3572] = 3;
  size[3573] = 8;
  size[3574] = 5;
  size[3575] = 12;
  size[3577] = 5;
  size[3579] = 14;
  size[3580] = 9;
  size[3581] = 12;
  size[3582] = 14;
  size[3583] = 20;
  size[3614] = 5;
  size[3615] = 3;
  size[3635] = 3;
  size[3646] = 3;
  size[3647] = 9;
  size[3659] = 5;
  size[3663] = 3;
  size[3675] = 3;
  size[3678] = 3;
  size[3679] = 8;
  size[3691] = 7;
  size[3693] = 3;
  size[3694] = 3;
  size[3695] = 5;
  size[3703] = 8;
  size[3705] = 3;
  size[3707] = 8;
  size[3709] = 4;
  size[3710] = 8;
  size[3711] = 12;
  size[3735] = 3;
  size[3739] = 3;
  size[3742] = 7;
  size[3743] = 5;
  size[3764] = 5;
  size[3765] = 3;
  size[3766] = 7;
  size[3767] = 8;
  size[3769] = 3;
  size[3771] = 11;
  size[3772] = 3;
  size[3773] = 8;
  size[3774] = 8;
  size[3775] = 14;
  size[3787] = 3;
  size[3788] = 3;
  size[3791] = 9;
  size[3798] = 3;
  size[3799] = 4;
  size[3803] = 8;
  size[3805] = 8;
  size[3806] = 8;
  size[3807] = 12;
  size[3809] = 5;
  size[3811] = 3;
  size[3813] = 3;
  size[3814] = 3;
  size[3815] = 8;
  size[3817] = 7;
  size[3819] = 8;
  size[3821] = 8;
  size[3822] = 11;
  size[3823] = 14;
  size[3825] = 3;
  size[3827] = 9;
  size[3828] = 3;
  size[3829] = 8;
  size[3830] = 5;
  size[3831] = 12;
  size[3833] = 5;
  size[3835] = 14;
  size[3836] = 9;
  size[3837] = 12;
  size[3838] = 14;
  size[3839] = 20;
  size[3855] = 7;
  size[3869] = 3;
  size[3870] = 3;
  size[3871] = 9;
  size[3885] = 3;
  size[3886] = 3;
  size[3887] = 9;
  size[3891] = 8;
  size[3901] = 8;
  size[3902] = 8;
  size[3903] = 13;
  size[3911] = 3;
  size[3915] = 3;
  size[3919] = 9;
  size[3925] = 7;
  size[3926] = 3;
  size[3927] = 9;
  size[3929] = 3;
  size[3930] = 7;
  size[3931] = 9;
  size[3933] = 9;
  size[3934] = 9;
  size[3935] = 13;
  size[3941] = 3;
  size[3942] = 3;
  size[3943] = 5;
  size[3945] = 3;
  size[3946] = 3;
  size[3947] = 5;
  size[3949] = 5;
  size[3950] = 5;
  size[3951] = 12;
  size[3956] = 3;
  size[3957] = 9;
  size[3958] = 5;
  size[3959] = 14;
  size[3960] = 3;
  size[3961] = 5;
  size[3962] = 9;
  size[3963] = 14;
  size[3964] = 8;
  size[3965] = 12;
  size[3966] = 12;
  size[3967] = 20;
  size[3975] = 3;
  size[3979] = 3;
  size[3983] = 9;
  size[3989] = 3;
  size[3990] = 3;
  size[3991] = 5;
  size[3993] = 3;
  size[3994] = 3;
  size[3995] = 5;
  size[3997] = 5;
  size[3998] = 5;
  size[3999] = 12;
  size[4005] = 7;
  size[4006] = 3;
  size[4007] = 9;
  size[4009] = 3;
  size[4010] = 7;
  size[4011] = 9;
  size[4013] = 9;
  size[4014] = 9;
  size[4015] = 13;
  size[4020] = 3;
  size[4021] = 9;
  size[4022] = 5;
  size[4023] = 14;
  size[4024] = 3;
  size[4025] = 5;
  size[4026] = 9;
  size[4027] = 14;
  size[4028] = 8;
  size[4029] = 12;
  size[4030] = 12;
  size[4031] = 20;
  size[4039] = 8;
  size[4043] = 8;
  size[4044] = 8;
  size[4047] = 13;
  size[4049] = 3;
  size[4050] = 3;
  size[4051] = 8;
  size[4053] = 9;
  size[4054] = 5;
  size[4055] = 12;
  size[4057] = 5;
  size[4058] = 9;
  size[4059] = 12;
  size[4061] = 14;
  size[4062] = 14;
  size[4063] = 20;
  size[4065] = 3;
  size[4066] = 3;
  size[4067] = 8;
  size[4069] = 9;
  size[4070] = 5;
  size[4071] = 12;
  size[4073] = 5;
  size[4074] = 9;
  size[4075] = 12;
  size[4077] = 14;
  size[4078] = 14;
  size[4079] = 20;
  size[4080] = 7;
  size[4081] = 9;
  size[4082] = 9;
  size[4083] = 13;
  size[4084] = 9;
  size[4085] = 13;
  size[4086] = 12;
  size[4087] = 20;
  size[4088] = 9;
  size[4089] = 12;
  size[4090] = 13;
  size[4091] = 20;
  size[4092] = 13;
  size[4093] = 20;
  size[4094] = 20;
  size[4095] = 35;
  std::array<uint16_t, 4097> offsets{};
  for (size_t i = 0; i < 4096; ++i)
  {
    offsets[i + 1] = offsets[i] + size[i];
  }
  // Total metadata entries: 3736
  std::array<vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseMetadata, 3736> metadata{};
  // Processing non-manifold case 1 with 4 voxel cases
  metadata[offsets[2457] + 0] = { 1, 129, true, { { 0, -1, -1, 1, 0, -1, -1, 1, 0, -1, -1, 1 } }, { { 1, 128, 0, 0 } }, { { 273, 2184, 0, 0 } } };
  metadata[offsets[1641] + 0] = { 1, 66, true, { { 0, -1, -1, 1, -1, 0, 1, -1, -1, 0, 1, -1 } }, { { 2, 64, 0, 0 } }, { { 545, 1096, 0, 0 } } };
  metadata[offsets[1686] + 0] = { 1, 36, true, { { -1, 0, 1, -1, 0, -1, -1, 1, -1, 1, 0, -1 } }, { { 4, 32, 0, 0 } }, { { 1042, 644, 0, 0 } } };
  metadata[offsets[2406] + 0] = { 1, 24, true, { { -1, 0, 1, -1, -1, 0, 1, -1, 1, -1, -1, 0 } }, { { 8, 16, 0, 0 } }, { { 2082, 324, 0, 0 } } };
  // Processing non-manifold case 2 with 12 voxel cases
  metadata[offsets[2355] + 0] = { 2, 9, true, { { 0, 1, -1, -1, 0, 1, -1, -1, 0, -1, -1, 1 } }, { { 1, 8, 0, 0 } }, { { 273, 2082, 0, 0 } } };
  metadata[offsets[917] + 0] = { 2, 33, true, { { 0, -1, 1, -1, 0, -1, -1, 1, 0, 1, -1, -1 } }, { { 1, 32, 0, 0 } }, { { 273, 644, 0, 0 } } };
  metadata[offsets[1369] + 0] = { 2, 65, true, { { 0, -1, -1, 1, 0, -1, 1, -1, 0, -1, 1, -1 } }, { { 1, 64, 0, 0 } }, { { 273, 1096, 0, 0 } } };
  metadata[offsets[1587] + 0] = { 2, 6, true, { { 0, 1, -1, -1, 1, 0, -1, -1, -1, 0, 1, -1 } }, { { 2, 4, 0, 0 } }, { { 545, 1042, 0, 0 } } };
  metadata[offsets[869] + 0] = { 2, 18, true, { { 0, -1, 1, -1, -1, 0, 1, -1, 1, 0, -1, -1 } }, { { 2, 16, 0, 0 } }, { { 545, 324, 0, 0 } } };
  metadata[offsets[2729] + 0] = { 2, 130, true, { { 0, -1, -1, 1, -1, 0, -1, 1, -1, 0, -1, 1 } }, { { 2, 128, 0, 0 } }, { { 545, 2184, 0, 0 } } };
  metadata[offsets[1366] + 0] = { 2, 20, true, { { -1, 0, 1, -1, 0, -1, 1, -1, 1, -1, 0, -1 } }, { { 4, 16, 0, 0 } }, { { 1042, 324, 0, 0 } } };
  metadata[offsets[3226] + 0] = { 2, 132, true, { { -1, 0, -1, 1, 0, -1, -1, 1, -1, -1, 0, 1 } }, { { 4, 128, 0, 0 } }, { { 1042, 2184, 0, 0 } } };
  metadata[offsets[2726] + 0] = { 2, 40, true, { { -1, 0, 1, -1, -1, 0, -1, 1, -1, 1, -1, 0 } }, { { 8, 32, 0, 0 } }, { { 2082, 644, 0, 0 } } };
  metadata[offsets[3178] + 0] = { 2, 72, true, { { -1, 0, -1, 1, -1, 0, 1, -1, -1, -1, 1, 0 } }, { { 8, 64, 0, 0 } }, { { 2082, 1096, 0, 0 } } };
  metadata[offsets[2508] + 0] = { 2, 144, true, { { -1, -1, 0, 1, -1, -1, 0, 1, 0, -1, -1, 1 } }, { { 16, 128, 0, 0 } }, { { 324, 2184, 0, 0 } } };
  metadata[offsets[1740] + 0] = { 2, 96, true, { { -1, -1, 0, 1, -1, -1, 1, 0, -1, 0, 1, -1 } }, { { 32, 64, 0, 0 } }, { { 644, 1096, 0, 0 } } };
  // Processing non-manifold case 3 with 24 voxel cases
  metadata[offsets[1912] + 0] = { 3, 67, true, { { -1, -1, -1, 1, 0, 0, 1, -1, 0, 0, 1, -1 } }, { { 3, 64, 0, 0 } }, { { 816, 1096, 0, 0 } } };
  metadata[offsets[1913] + 0] = { 3, 67, false, { { 0, -1, -1, 1, 0, 0, 1, -1, 0, 0, 1, -1 } }, { { 3, 64, 0, 0 } }, { { 817, 1096, 0, 0 } } };
  metadata[offsets[3000] + 0] = { 3, 131, true, { { -1, -1, -1, 1, 0, 0, -1, 1, 0, 0, -1, 1 } }, { { 3, 128, 0, 0 } }, { { 816, 2184, 0, 0 } } };
  metadata[offsets[3001] + 0] = { 3, 131, false, { { 0, -1, -1, 1, 0, 0, -1, 1, 0, 0, -1, 1 } }, { { 3, 128, 0, 0 } }, { { 817, 2184, 0, 0 } } };
  metadata[offsets[1927] + 0] = { 3, 37, true, { { 0, 0, 1, -1, -1, -1, -1, 1, 0, 1, 0, -1 } }, { { 5, 32, 0, 0 } }, { { 1283, 644, 0, 0 } } };
  metadata[offsets[1943] + 0] = { 3, 37, false, { { 0, 0, 1, -1, 0, -1, -1, 1, 0, 1, 0, -1 } }, { { 5, 32, 0, 0 } }, { { 1299, 644, 0, 0 } } };
  metadata[offsets[3467] + 0] = { 3, 133, true, { { 0, 0, -1, 1, -1, -1, -1, 1, 0, -1, 0, 1 } }, { { 5, 128, 0, 0 } }, { { 1283, 2184, 0, 0 } } };
  metadata[offsets[3483] + 0] = { 3, 133, false, { { 0, 0, -1, 1, 0, -1, -1, 1, 0, -1, 0, 1 } }, { { 5, 128, 0, 0 } }, { { 1299, 2184, 0, 0 } } };
  metadata[offsets[2167] + 0] = { 3, 25, true, { { 0, 1, 0, -1, 0, 1, 0, -1, -1, -1, -1, 1 } }, { { 17, 8, 0, 0 } }, { { 85, 2082, 0, 0 } } };
  metadata[offsets[2423] + 0] = { 3, 25, false, { { 0, 1, 0, -1, 0, 1, 0, -1, 0, -1, -1, 1 } }, { { 17, 8, 0, 0 } }, { { 341, 2082, 0, 0 } } };
  metadata[offsets[443] + 0] = { 3, 137, true, { { 0, 1, -1, 1, 0, 1, -1, 1, 0, -1, -1, -1 } }, { { 1, 136, 0, 0 } }, { { 273, 170, 0, 0 } } };
  metadata[offsets[2491] + 0] = { 3, 137, false, { { 0, 1, -1, 1, 0, 1, -1, 1, 0, -1, -1, 1 } }, { { 1, 136, 0, 0 } }, { { 273, 2218, 0, 0 } } };
  metadata[offsets[2269] + 0] = { 3, 145, true, { { 0, -1, 0, 1, 0, -1, 0, 1, -1, -1, -1, 1 } }, { { 17, 128, 0, 0 } }, { { 85, 2184, 0, 0 } } };
  metadata[offsets[2525] + 0] = { 3, 145, false, { { 0, -1, 0, 1, 0, -1, 0, 1, 0, -1, -1, 1 } }, { { 17, 128, 0, 0 } }, { { 341, 2184, 0, 0 } } };
  metadata[offsets[2845] + 0] = { 3, 161, true, { { 0, -1, 1, 1, 0, -1, -1, -1, 0, 1, -1, 1 } }, { { 1, 160, 0, 0 } }, { { 273, 2572, 0, 0 } } };
  metadata[offsets[2973] + 0] = { 3, 161, false, { { 0, -1, 1, 1, 0, -1, -1, 1, 0, 1, -1, 1 } }, { { 1, 160, 0, 0 } }, { { 273, 2700, 0, 0 } } };
  metadata[offsets[3537] + 0] = { 3, 193, true, { { 0, -1, -1, -1, 0, -1, 1, 1, 0, -1, 1, 1 } }, { { 1, 192, 0, 0 } }, { { 273, 3264, 0, 0 } } };
  metadata[offsets[3545] + 0] = { 3, 193, false, { { 0, -1, -1, 1, 0, -1, 1, 1, 0, -1, 1, 1 } }, { { 1, 192, 0, 0 } }, { { 273, 3272, 0, 0 } } };
  metadata[offsets[1207] + 0] = { 3, 38, true, { { 0, 1, 0, -1, 1, 0, -1, 0, -1, -1, 1, -1 } }, { { 34, 4, 0, 0 } }, { { 165, 1042, 0, 0 } } };
  metadata[offsets[1719] + 0] = { 3, 38, false, { { 0, 1, 0, -1, 1, 0, -1, 0, -1, 0, 1, -1 } }, { { 34, 4, 0, 0 } }, { { 677, 1042, 0, 0 } } };
  metadata[offsets[635] + 0] = { 3, 70, true, { { 0, 1, -1, 1, 1, 0, 1, -1, -1, 0, -1, -1 } }, { { 2, 68, 0, 0 } }, { { 545, 90, 0, 0 } } };
  metadata[offsets[1659] + 0] = { 3, 70, false, { { 0, 1, -1, 1, 1, 0, 1, -1, -1, 0, 1, -1 } }, { { 2, 68, 0, 0 } }, { { 545, 1114, 0, 0 } } };
  metadata[offsets[2887] + 0] = { 3, 26, true, { { 0, 0, 1, -1, -1, -1, 1, -1, 1, 0, -1, 0 } }, { { 10, 16, 0, 0 } }, { { 2563, 324, 0, 0 } } };
  metadata[offsets[2919] + 0] = { 3, 26, false, { { 0, 0, 1, -1, -1, 0, 1, -1, 1, 0, -1, 0 } }, { { 10, 16, 0, 0 } }, { { 2595, 324, 0, 0 } } };
  metadata[offsets[3659] + 0] = { 3, 74, true, { { 0, 0, -1, 1, -1, -1, 1, -1, -1, 0, 1, 0 } }, { { 10, 64, 0, 0 } }, { { 2563, 1096, 0, 0 } } };
  metadata[offsets[3691] + 0] = { 3, 74, false, { { 0, 0, -1, 1, -1, 0, 1, -1, -1, 0, 1, 0 } }, { { 10, 64, 0, 0 } }, { { 2595, 1096, 0, 0 } } };
  metadata[offsets[1837] + 0] = { 3, 82, true, { { 0, -1, 1, 1, -1, 0, -1, -1, 1, 0, 1, -1 } }, { { 2, 80, 0, 0 } }, { { 545, 1292, 0, 0 } } };
  metadata[offsets[1901] + 0] = { 3, 82, false, { { 0, -1, 1, 1, -1, 0, 1, -1, 1, 0, 1, -1 } }, { { 2, 80, 0, 0 } }, { { 545, 1356, 0, 0 } } };
  metadata[offsets[1261] + 0] = { 3, 98, true, { { 0, -1, 0, 1, -1, 0, 1, 0, -1, -1, 1, -1 } }, { { 34, 64, 0, 0 } }, { { 165, 1096, 0, 0 } } };
  metadata[offsets[1773] + 0] = { 3, 98, false, { { 0, -1, 0, 1, -1, 0, 1, 0, -1, 0, 1, -1 } }, { { 34, 64, 0, 0 } }, { { 677, 1096, 0, 0 } } };
  metadata[offsets[3809] + 0] = { 3, 194, true, { { 0, -1, -1, -1, -1, 0, 1, 1, -1, 0, 1, 1 } }, { { 2, 192, 0, 0 } }, { { 545, 3264, 0, 0 } } };
  metadata[offsets[3817] + 0] = { 3, 194, false, { { 0, -1, -1, 1, -1, 0, 1, 1, -1, 0, 1, 1 } }, { { 2, 192, 0, 0 } }, { { 545, 3272, 0, 0 } } };
  metadata[offsets[3444] + 0] = { 3, 28, true, { { -1, -1, 1, -1, 0, 0, 1, -1, 1, -1, 0, 0 } }, { { 12, 16, 0, 0 } }, { { 3120, 324, 0, 0 } } };
  metadata[offsets[3446] + 0] = { 3, 28, false, { { -1, 0, 1, -1, 0, 0, 1, -1, 1, -1, 0, 0 } }, { { 12, 16, 0, 0 } }, { { 3122, 324, 0, 0 } } };
  metadata[offsets[3764] + 0] = { 3, 44, true, { { -1, -1, 1, -1, 0, 0, -1, 1, -1, 1, 0, 0 } }, { { 12, 32, 0, 0 } }, { { 3120, 644, 0, 0 } } };
  metadata[offsets[3766] + 0] = { 3, 44, false, { { -1, 0, 1, -1, 0, 0, -1, 1, -1, 1, 0, 0 } }, { { 12, 32, 0, 0 } }, { { 3122, 644, 0, 0 } } };
  metadata[offsets[2002] + 0] = { 3, 52, true, { { -1, 0, -1, -1, 0, -1, 1, 1, 1, 1, 0, -1 } }, { { 4, 48, 0, 0 } }, { { 1042, 960, 0, 0 } } };
  metadata[offsets[2006] + 0] = { 3, 52, false, { { -1, 0, 1, -1, 0, -1, 1, 1, 1, 1, 0, -1 } }, { { 4, 48, 0, 0 } }, { { 1042, 964, 0, 0 } } };
  metadata[offsets[734] + 0] = { 3, 100, true, { { -1, 0, 1, 0, 0, -1, 0, 1, -1, 1, -1, -1 } }, { { 68, 32, 0, 0 } }, { { 90, 644, 0, 0 } } };
  metadata[offsets[1758] + 0] = { 3, 100, false, { { -1, 0, 1, 0, 0, -1, 0, 1, -1, 1, 0, -1 } }, { { 68, 32, 0, 0 } }, { { 1114, 644, 0, 0 } } };
  metadata[offsets[3614] + 0] = { 3, 164, true, { { -1, 0, 1, 1, 0, -1, -1, -1, -1, 1, 0, 1 } }, { { 4, 160, 0, 0 } }, { { 1042, 2572, 0, 0 } } };
  metadata[offsets[3742] + 0] = { 3, 164, false, { { -1, 0, 1, 1, 0, -1, -1, 1, -1, 1, 0, 1 } }, { { 4, 160, 0, 0 } }, { { 1042, 2700, 0, 0 } } };
  metadata[offsets[3042] + 0] = { 3, 56, true, { { -1, 0, -1, -1, -1, 0, 1, 1, 1, 1, -1, 0 } }, { { 8, 48, 0, 0 } }, { { 2082, 960, 0, 0 } } };
  metadata[offsets[3046] + 0] = { 3, 56, false, { { -1, 0, 1, -1, -1, 0, 1, 1, 1, 1, -1, 0 } }, { { 8, 48, 0, 0 } }, { { 2082, 964, 0, 0 } } };
  metadata[offsets[3374] + 0] = { 3, 88, true, { { -1, 0, 1, 1, -1, 0, -1, -1, 1, -1, 1, 0 } }, { { 8, 80, 0, 0 } }, { { 2082, 1292, 0, 0 } } };
  metadata[offsets[3438] + 0] = { 3, 88, false, { { -1, 0, 1, 1, -1, 0, 1, -1, 1, -1, 1, 0 } }, { { 8, 80, 0, 0 } }, { { 2082, 1356, 0, 0 } } };
  metadata[offsets[494] + 0] = { 3, 152, true, { { -1, 0, 1, 0, -1, 0, 1, 0, 1, -1, -1, -1 } }, { { 136, 16, 0, 0 } }, { { 170, 324, 0, 0 } } };
  metadata[offsets[2542] + 0] = { 3, 152, false, { { -1, 0, 1, 0, -1, 0, 1, 0, 1, -1, -1, 0 } }, { { 136, 16, 0, 0 } }, { { 2218, 324, 0, 0 } } };
  // Processing non-manifold case 4 with 8 voxel cases
  metadata[offsets[2999] + 0] = { 4, 41, true, { { 0, 1, 2, -1, 0, 1, -1, 2, 0, 2, -1, 1 } }, { { 1, 8, 32, 0 } }, { { 273, 2082, 644, 0 } } };
  metadata[offsets[3451] + 0] = { 4, 73, true, { { 0, 1, -1, 2, 0, 1, 2, -1, 0, -1, 2, 1 } }, { { 1, 8, 64, 0 } }, { { 273, 2082, 1096, 0 } } };
  metadata[offsets[2013] + 0] = { 4, 97, true, { { 0, -1, 1, 2, 0, -1, 2, 1, 0, 1, 2, -1 } }, { { 1, 32, 64, 0 } }, { { 273, 644, 1096, 0 } } };
  metadata[offsets[1911] + 0] = { 4, 22, true, { { 0, 1, 2, -1, 1, 0, 2, -1, 2, 0, 1, -1 } }, { { 2, 4, 16, 0 } }, { { 545, 1042, 324, 0 } } };
  metadata[offsets[3771] + 0] = { 4, 134, true, { { 0, 1, -1, 2, 1, 0, -1, 2, -1, 0, 1, 2 } }, { { 2, 4, 128, 0 } }, { { 545, 1042, 2184, 0 } } };
  metadata[offsets[3053] + 0] = { 4, 146, true, { { 0, -1, 1, 2, -1, 0, 1, 2, 1, 0, -1, 2 } }, { { 2, 16, 128, 0 } }, { { 545, 324, 2184, 0 } } };
  metadata[offsets[3550] + 0] = { 4, 148, true, { { -1, 0, 1, 2, 0, -1, 1, 2, 1, -1, 0, 2 } }, { { 4, 16, 128, 0 } }, { { 1042, 324, 2184, 0 } } };
  metadata[offsets[3822] + 0] = { 4, 104, true, { { -1, 0, 1, 2, -1, 0, 2, 1, -1, 1, 2, 0 } }, { { 8, 32, 64, 0 } }, { { 2082, 644, 1096, 0 } } };
  // Processing non-manifold case 5 with 6 voxel cases
  metadata[offsets[4080] + 0] = { 5, 195, true, { { -1, -1, -1, -1, 0, 0, 1, 1, 0, 0, 1, 1 } }, { { 3, 192, 0, 0 } }, { { 816, 3264, 0, 0 } } };
  metadata[offsets[4081] + 0] = { 5, 195, false, { { 0, -1, -1, -1, 0, 0, 1, 1, 0, 0, 1, 1 } }, { { 3, 192, 0, 0 } }, { { 817, 3264, 0, 0 } } };
  metadata[offsets[4088] + 0] = { 5, 195, false, { { -1, -1, -1, 1, 0, 0, 1, 1, 0, 0, 1, 1 } }, { { 3, 192, 0, 0 } }, { { 816, 3272, 0, 0 } } };
  metadata[offsets[4089] + 0] = { 5, 195, false, { { 0, -1, -1, 1, 0, 0, 1, 1, 0, 0, 1, 1 } }, { { 3, 192, 0, 0 } }, { { 817, 3272, 0, 0 } } };
  metadata[offsets[3855] + 0] = { 5, 165, true, { { 0, 0, 1, 1, -1, -1, -1, -1, 0, 1, 0, 1 } }, { { 5, 160, 0, 0 } }, { { 1283, 2572, 0, 0 } } };
  metadata[offsets[3871] + 0] = { 5, 165, false, { { 0, 0, 1, 1, 0, -1, -1, -1, 0, 1, 0, 1 } }, { { 5, 160, 0, 0 } }, { { 1299, 2572, 0, 0 } } };
  metadata[offsets[3983] + 0] = { 5, 165, false, { { 0, 0, 1, 1, -1, -1, -1, 1, 0, 1, 0, 1 } }, { { 5, 160, 0, 0 } }, { { 1283, 2700, 0, 0 } } };
  metadata[offsets[3999] + 0] = { 5, 165, false, { { 0, 0, 1, 1, 0, -1, -1, 1, 0, 1, 0, 1 } }, { { 5, 160, 0, 0 } }, { { 1299, 2700, 0, 0 } } };
  metadata[offsets[255] + 0] = { 5, 153, true, { { 0, 1, 0, 1, 0, 1, 0, 1, -1, -1, -1, -1 } }, { { 17, 136, 0, 0 } }, { { 85, 170, 0, 0 } } };
  metadata[offsets[511] + 0] = { 5, 153, false, { { 0, 1, 0, 1, 0, 1, 0, 1, 0, -1, -1, -1 } }, { { 17, 136, 0, 0 } }, { { 341, 170, 0, 0 } } };
  metadata[offsets[2303] + 0] = { 5, 153, false, { { 0, 1, 0, 1, 0, 1, 0, 1, -1, -1, -1, 1 } }, { { 17, 136, 0, 0 } }, { { 85, 2218, 0, 0 } } };
  metadata[offsets[2559] + 0] = { 5, 153, false, { { 0, 1, 0, 1, 0, 1, 0, 1, 0, -1, -1, 1 } }, { { 17, 136, 0, 0 } }, { { 341, 2218, 0, 0 } } };
  metadata[offsets[255] + 1] = { 5, 102, true, { { 0, 1, 0, 1, 1, 0, 1, 0, -1, -1, -1, -1 } }, { { 34, 68, 0, 0 } }, { { 165, 90, 0, 0 } } };
  metadata[offsets[767] + 0] = { 5, 102, false, { { 0, 1, 0, 1, 1, 0, 1, 0, -1, 0, -1, -1 } }, { { 34, 68, 0, 0 } }, { { 677, 90, 0, 0 } } };
  metadata[offsets[1279] + 0] = { 5, 102, false, { { 0, 1, 0, 1, 1, 0, 1, 0, -1, -1, 1, -1 } }, { { 34, 68, 0, 0 } }, { { 165, 1114, 0, 0 } } };
  metadata[offsets[1791] + 0] = { 5, 102, false, { { 0, 1, 0, 1, 1, 0, 1, 0, -1, 0, 1, -1 } }, { { 34, 68, 0, 0 } }, { { 677, 1114, 0, 0 } } };
  metadata[offsets[3855] + 1] = { 5, 90, true, { { 0, 0, 1, 1, -1, -1, -1, -1, 1, 0, 1, 0 } }, { { 10, 80, 0, 0 } }, { { 2563, 1292, 0, 0 } } };
  metadata[offsets[3887] + 0] = { 5, 90, false, { { 0, 0, 1, 1, -1, 0, -1, -1, 1, 0, 1, 0 } }, { { 10, 80, 0, 0 } }, { { 2595, 1292, 0, 0 } } };
  metadata[offsets[3919] + 0] = { 5, 90, false, { { 0, 0, 1, 1, -1, -1, 1, -1, 1, 0, 1, 0 } }, { { 10, 80, 0, 0 } }, { { 2563, 1356, 0, 0 } } };
  metadata[offsets[3951] + 0] = { 5, 90, false, { { 0, 0, 1, 1, -1, 0, 1, -1, 1, 0, 1, 0 } }, { { 10, 80, 0, 0 } }, { { 2595, 1356, 0, 0 } } };
  metadata[offsets[4080] + 1] = { 5, 60, true, { { -1, -1, -1, -1, 0, 0, 1, 1, 1, 1, 0, 0 } }, { { 12, 48, 0, 0 } }, { { 3120, 960, 0, 0 } } };
  metadata[offsets[4082] + 0] = { 5, 60, false, { { -1, 0, -1, -1, 0, 0, 1, 1, 1, 1, 0, 0 } }, { { 12, 48, 0, 0 } }, { { 3122, 960, 0, 0 } } };
  metadata[offsets[4084] + 0] = { 5, 60, false, { { -1, -1, 1, -1, 0, 0, 1, 1, 1, 1, 0, 0 } }, { { 12, 48, 0, 0 } }, { { 3120, 964, 0, 0 } } };
  metadata[offsets[4086] + 0] = { 5, 60, false, { { -1, 0, 1, -1, 0, 0, 1, 1, 1, 1, 0, 0 } }, { { 12, 48, 0, 0 } }, { { 3122, 964, 0, 0 } } };
  // Processing non-manifold case 6 with 24 voxel cases
  metadata[offsets[4010] + 0] = { 6, 135, true, { { -1, 0, -1, 1, -1, 0, -1, 1, 0, 0, 0, 1 } }, { { 7, 128, 0, 0 } }, { { 1826, 2184, 0, 0 } } };
  metadata[offsets[4011] + 0] = { 6, 135, false, { { 0, 0, -1, 1, -1, 0, -1, 1, 0, 0, 0, 1 } }, { { 7, 128, 0, 0 } }, { { 1827, 2184, 0, 0 } } };
  metadata[offsets[4026] + 0] = { 6, 135, false, { { -1, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 1 } }, { { 7, 128, 0, 0 } }, { { 1842, 2184, 0, 0 } } };
  metadata[offsets[4027] + 0] = { 6, 135, false, { { 0, 0, -1, 1, 0, 0, -1, 1, 0, 0, 0, 1 } }, { { 7, 128, 0, 0 } }, { { 1843, 2184, 0, 0 } } };
  metadata[offsets[3930] + 0] = { 6, 75, true, { { -1, 0, -1, 1, 0, -1, 1, -1, 0, 0, 1, 0 } }, { { 11, 64, 0, 0 } }, { { 2834, 1096, 0, 0 } } };
  metadata[offsets[3931] + 0] = { 6, 75, false, { { 0, 0, -1, 1, 0, -1, 1, -1, 0, 0, 1, 0 } }, { { 11, 64, 0, 0 } }, { { 2835, 1096, 0, 0 } } };
  metadata[offsets[3962] + 0] = { 6, 75, false, { { -1, 0, -1, 1, 0, 0, 1, -1, 0, 0, 1, 0 } }, { { 11, 64, 0, 0 } }, { { 2866, 1096, 0, 0 } } };
  metadata[offsets[3963] + 0] = { 6, 75, false, { { 0, 0, -1, 1, 0, 0, 1, -1, 0, 0, 1, 0 } }, { { 11, 64, 0, 0 } }, { { 2867, 1096, 0, 0 } } };
  metadata[offsets[2812] + 0] = { 6, 147, true, { { -1, -1, 0, 1, 0, 0, 0, 1, -1, 0, -1, 1 } }, { { 19, 128, 0, 0 } }, { { 628, 2184, 0, 0 } } };
  metadata[offsets[2813] + 0] = { 6, 147, false, { { 0, -1, 0, 1, 0, 0, 0, 1, -1, 0, -1, 1 } }, { { 19, 128, 0, 0 } }, { { 629, 2184, 0, 0 } } };
  metadata[offsets[3068] + 0] = { 6, 147, false, { { -1, -1, 0, 1, 0, 0, 0, 1, 0, 0, -1, 1 } }, { { 19, 128, 0, 0 } }, { { 884, 2184, 0, 0 } } };
  metadata[offsets[3069] + 0] = { 6, 147, false, { { 0, -1, 0, 1, 0, 0, 0, 1, 0, 0, -1, 1 } }, { { 19, 128, 0, 0 } }, { { 885, 2184, 0, 0 } } };
  metadata[offsets[1532] + 0] = { 6, 99, true, { { -1, -1, 0, 1, 0, 0, 1, 0, 0, -1, 1, -1 } }, { { 35, 64, 0, 0 } }, { { 436, 1096, 0, 0 } } };
  metadata[offsets[1533] + 0] = { 6, 99, false, { { 0, -1, 0, 1, 0, 0, 1, 0, 0, -1, 1, -1 } }, { { 35, 64, 0, 0 } }, { { 437, 1096, 0, 0 } } };
  metadata[offsets[2044] + 0] = { 6, 99, false, { { -1, -1, 0, 1, 0, 0, 1, 0, 0, 0, 1, -1 } }, { { 35, 64, 0, 0 } }, { { 948, 1096, 0, 0 } } };
  metadata[offsets[2045] + 0] = { 6, 99, false, { { 0, -1, 0, 1, 0, 0, 1, 0, 0, 0, 1, -1 } }, { { 35, 64, 0, 0 } }, { { 949, 1096, 0, 0 } } };
  metadata[offsets[4005] + 0] = { 6, 45, true, { { 0, -1, 1, -1, -1, 0, -1, 1, 0, 1, 0, 0 } }, { { 13, 32, 0, 0 } }, { { 3361, 644, 0, 0 } } };
  metadata[offsets[4007] + 0] = { 6, 45, false, { { 0, 0, 1, -1, -1, 0, -1, 1, 0, 1, 0, 0 } }, { { 13, 32, 0, 0 } }, { { 3363, 644, 0, 0 } } };
  metadata[offsets[4021] + 0] = { 6, 45, false, { { 0, -1, 1, -1, 0, 0, -1, 1, 0, 1, 0, 0 } }, { { 13, 32, 0, 0 } }, { { 3377, 644, 0, 0 } } };
  metadata[offsets[4023] + 0] = { 6, 45, false, { { 0, 0, 1, -1, 0, 0, -1, 1, 0, 1, 0, 0 } }, { { 13, 32, 0, 0 } }, { { 3379, 644, 0, 0 } } };
  metadata[offsets[3279] + 0] = { 6, 149, true, { { 0, 0, 0, 1, -1, -1, 0, 1, -1, -1, 0, 1 } }, { { 21, 128, 0, 0 } }, { { 1095, 2184, 0, 0 } } };
  metadata[offsets[3295] + 0] = { 6, 149, false, { { 0, 0, 0, 1, 0, -1, 0, 1, -1, -1, 0, 1 } }, { { 21, 128, 0, 0 } }, { { 1111, 2184, 0, 0 } } };
  metadata[offsets[3535] + 0] = { 6, 149, false, { { 0, 0, 0, 1, -1, -1, 0, 1, 0, -1, 0, 1 } }, { { 21, 128, 0, 0 } }, { { 1351, 2184, 0, 0 } } };
  metadata[offsets[3551] + 0] = { 6, 149, false, { { 0, 0, 0, 1, 0, -1, 0, 1, 0, -1, 0, 1 } }, { { 21, 128, 0, 0 } }, { { 1367, 2184, 0, 0 } } };
  metadata[offsets[975] + 0] = { 6, 101, true, { { 0, 0, 1, 0, -1, -1, 0, 1, 0, 1, -1, -1 } }, { { 69, 32, 0, 0 } }, { { 331, 644, 0, 0 } } };
  metadata[offsets[991] + 0] = { 6, 101, false, { { 0, 0, 1, 0, 0, -1, 0, 1, 0, 1, -1, -1 } }, { { 69, 32, 0, 0 } }, { { 347, 644, 0, 0 } } };
  metadata[offsets[1999] + 0] = { 6, 101, false, { { 0, 0, 1, 0, -1, -1, 0, 1, 0, 1, 0, -1 } }, { { 69, 32, 0, 0 } }, { { 1355, 644, 0, 0 } } };
  metadata[offsets[2015] + 0] = { 6, 101, false, { { 0, 0, 1, 0, 0, -1, 0, 1, 0, 1, 0, -1 } }, { { 69, 32, 0, 0 } }, { { 1371, 644, 0, 0 } } };
  metadata[offsets[2803] + 0] = { 6, 57, true, { { 0, 1, -1, -1, 0, 1, 0, 0, -1, 0, -1, 1 } }, { { 49, 8, 0, 0 } }, { { 721, 2082, 0, 0 } } };
  metadata[offsets[2807] + 0] = { 6, 57, false, { { 0, 1, 0, -1, 0, 1, 0, 0, -1, 0, -1, 1 } }, { { 49, 8, 0, 0 } }, { { 725, 2082, 0, 0 } } };
  metadata[offsets[3059] + 0] = { 6, 57, false, { { 0, 1, -1, -1, 0, 1, 0, 0, 0, 0, -1, 1 } }, { { 49, 8, 0, 0 } }, { { 977, 2082, 0, 0 } } };
  metadata[offsets[3063] + 0] = { 6, 57, false, { { 0, 1, 0, -1, 0, 1, 0, 0, 0, 0, -1, 1 } }, { { 49, 8, 0, 0 } }, { { 981, 2082, 0, 0 } } };
  metadata[offsets[3135] + 0] = { 6, 89, true, { { 0, 1, 0, 0, 0, 1, -1, -1, -1, -1, 0, 1 } }, { { 81, 8, 0, 0 } }, { { 1053, 2082, 0, 0 } } };
  metadata[offsets[3199] + 0] = { 6, 89, false, { { 0, 1, 0, 0, 0, 1, 0, -1, -1, -1, 0, 1 } }, { { 81, 8, 0, 0 } }, { { 1117, 2082, 0, 0 } } };
  metadata[offsets[3391] + 0] = { 6, 89, false, { { 0, 1, 0, 0, 0, 1, -1, -1, 0, -1, 0, 1 } }, { { 81, 8, 0, 0 } }, { { 1309, 2082, 0, 0 } } };
  metadata[offsets[3455] + 0] = { 6, 89, false, { { 0, 1, 0, 0, 0, 1, 0, -1, 0, -1, 0, 1 } }, { { 81, 8, 0, 0 } }, { { 1373, 2082, 0, 0 } } };
  metadata[offsets[831] + 0] = { 6, 169, true, { { 0, 1, 1, 1, 0, 1, -1, -1, 0, 1, -1, -1 } }, { { 1, 168, 0, 0 } }, { { 273, 558, 0, 0 } } };
  metadata[offsets[959] + 0] = { 6, 169, false, { { 0, 1, 1, 1, 0, 1, -1, 1, 0, 1, -1, -1 } }, { { 1, 168, 0, 0 } }, { { 273, 686, 0, 0 } } };
  metadata[offsets[2879] + 0] = { 6, 169, false, { { 0, 1, 1, 1, 0, 1, -1, -1, 0, 1, -1, 1 } }, { { 1, 168, 0, 0 } }, { { 273, 2606, 0, 0 } } };
  metadata[offsets[3007] + 0] = { 6, 169, false, { { 0, 1, 1, 1, 0, 1, -1, 1, 0, 1, -1, 1 } }, { { 1, 168, 0, 0 } }, { { 273, 2734, 0, 0 } } };
  metadata[offsets[1523] + 0] = { 6, 201, true, { { 0, 1, -1, -1, 0, 1, 1, 1, 0, -1, 1, -1 } }, { { 1, 200, 0, 0 } }, { { 273, 1250, 0, 0 } } };
  metadata[offsets[1531] + 0] = { 6, 201, false, { { 0, 1, -1, 1, 0, 1, 1, 1, 0, -1, 1, -1 } }, { { 1, 200, 0, 0 } }, { { 273, 1258, 0, 0 } } };
  metadata[offsets[3571] + 0] = { 6, 201, false, { { 0, 1, -1, -1, 0, 1, 1, 1, 0, -1, 1, 1 } }, { { 1, 200, 0, 0 } }, { { 273, 3298, 0, 0 } } };
  metadata[offsets[3579] + 0] = { 6, 201, false, { { 0, 1, -1, 1, 0, 1, 1, 1, 0, -1, 1, 1 } }, { { 1, 200, 0, 0 } }, { { 273, 3306, 0, 0 } } };
  metadata[offsets[3925] + 0] = { 6, 225, true, { { 0, -1, 1, -1, 0, -1, 1, -1, 0, 1, 1, 1 } }, { { 1, 224, 0, 0 } }, { { 273, 3652, 0, 0 } } };
  metadata[offsets[3933] + 0] = { 6, 225, false, { { 0, -1, 1, 1, 0, -1, 1, -1, 0, 1, 1, 1 } }, { { 1, 224, 0, 0 } }, { { 273, 3660, 0, 0 } } };
  metadata[offsets[4053] + 0] = { 6, 225, false, { { 0, -1, 1, -1, 0, -1, 1, 1, 0, 1, 1, 1 } }, { { 1, 224, 0, 0 } }, { { 273, 3780, 0, 0 } } };
  metadata[offsets[4061] + 0] = { 6, 225, false, { { 0, -1, 1, 1, 0, -1, 1, 1, 0, 1, 1, 1 } }, { { 1, 224, 0, 0 } }, { { 273, 3788, 0, 0 } } };
  metadata[offsets[3925] + 1] = { 6, 30, true, { { 0, -1, 1, -1, 0, -1, 1, -1, 1, 0, 0, 0 } }, { { 14, 16, 0, 0 } }, { { 3601, 324, 0, 0 } } };
  metadata[offsets[3927] + 0] = { 6, 30, false, { { 0, 0, 1, -1, 0, -1, 1, -1, 1, 0, 0, 0 } }, { { 14, 16, 0, 0 } }, { { 3603, 324, 0, 0 } } };
  metadata[offsets[3957] + 0] = { 6, 30, false, { { 0, -1, 1, -1, 0, 0, 1, -1, 1, 0, 0, 0 } }, { { 14, 16, 0, 0 } }, { { 3633, 324, 0, 0 } } };
  metadata[offsets[3959] + 0] = { 6, 30, false, { { 0, 0, 1, -1, 0, 0, 1, -1, 1, 0, 0, 0 } }, { { 14, 16, 0, 0 } }, { { 3635, 324, 0, 0 } } };
  metadata[offsets[1523] + 1] = { 6, 54, true, { { 0, 1, -1, -1, 1, 0, 0, 0, 0, -1, 1, -1 } }, { { 50, 4, 0, 0 } }, { { 481, 1042, 0, 0 } } };
  metadata[offsets[1527] + 0] = { 6, 54, false, { { 0, 1, 0, -1, 1, 0, 0, 0, 0, -1, 1, -1 } }, { { 50, 4, 0, 0 } }, { { 485, 1042, 0, 0 } } };
  metadata[offsets[2035] + 0] = { 6, 54, false, { { 0, 1, -1, -1, 1, 0, 0, 0, 0, 0, 1, -1 } }, { { 50, 4, 0, 0 } }, { { 993, 1042, 0, 0 } } };
  metadata[offsets[2039] + 0] = { 6, 54, false, { { 0, 1, 0, -1, 1, 0, 0, 0, 0, 0, 1, -1 } }, { { 50, 4, 0, 0 } }, { { 997, 1042, 0, 0 } } };
  metadata[offsets[831] + 1] = { 6, 86, true, { { 0, 1, 1, 1, 1, 0, -1, -1, 1, 0, -1, -1 } }, { { 2, 84, 0, 0 } }, { { 545, 286, 0, 0 } } };
  metadata[offsets[895] + 0] = { 6, 86, false, { { 0, 1, 1, 1, 1, 0, 1, -1, 1, 0, -1, -1 } }, { { 2, 84, 0, 0 } }, { { 545, 350, 0, 0 } } };
  metadata[offsets[1855] + 0] = { 6, 86, false, { { 0, 1, 1, 1, 1, 0, -1, -1, 1, 0, 1, -1 } }, { { 2, 84, 0, 0 } }, { { 545, 1310, 0, 0 } } };
  metadata[offsets[1919] + 0] = { 6, 86, false, { { 0, 1, 1, 1, 1, 0, 1, -1, 1, 0, 1, -1 } }, { { 2, 84, 0, 0 } }, { { 545, 1374, 0, 0 } } };
  metadata[offsets[3135] + 1] = { 6, 166, true, { { 0, 1, 0, 0, 1, 0, -1, -1, -1, -1, 1, 0 } }, { { 162, 4, 0, 0 } }, { { 2093, 1042, 0, 0 } } };
  metadata[offsets[3263] + 0] = { 6, 166, false, { { 0, 1, 0, 0, 1, 0, -1, 0, -1, -1, 1, 0 } }, { { 162, 4, 0, 0 } }, { { 2221, 1042, 0, 0 } } };
  metadata[offsets[3647] + 0] = { 6, 166, false, { { 0, 1, 0, 0, 1, 0, -1, -1, -1, 0, 1, 0 } }, { { 162, 4, 0, 0 } }, { { 2605, 1042, 0, 0 } } };
  metadata[offsets[3775] + 0] = { 6, 166, false, { { 0, 1, 0, 0, 1, 0, -1, 0, -1, 0, 1, 0 } }, { { 162, 4, 0, 0 } }, { { 2733, 1042, 0, 0 } } };
  metadata[offsets[2803] + 1] = { 6, 198, true, { { 0, 1, -1, -1, 1, 0, 1, 1, -1, 0, -1, 1 } }, { { 2, 196, 0, 0 } }, { { 545, 2258, 0, 0 } } };
  metadata[offsets[2811] + 0] = { 6, 198, false, { { 0, 1, -1, 1, 1, 0, 1, 1, -1, 0, -1, 1 } }, { { 2, 196, 0, 0 } }, { { 545, 2266, 0, 0 } } };
  metadata[offsets[3827] + 0] = { 6, 198, false, { { 0, 1, -1, -1, 1, 0, 1, 1, -1, 0, 1, 1 } }, { { 2, 196, 0, 0 } }, { { 545, 3282, 0, 0 } } };
  metadata[offsets[3835] + 0] = { 6, 198, false, { { 0, 1, -1, 1, 1, 0, 1, 1, -1, 0, 1, 1 } }, { { 2, 196, 0, 0 } }, { { 545, 3290, 0, 0 } } };
  metadata[offsets[975] + 1] = { 6, 154, true, { { 0, 0, 1, 0, -1, -1, 1, 0, 1, 0, -1, -1 } }, { { 138, 16, 0, 0 } }, { { 651, 324, 0, 0 } } };
  metadata[offsets[1007] + 0] = { 6, 154, false, { { 0, 0, 1, 0, -1, 0, 1, 0, 1, 0, -1, -1 } }, { { 138, 16, 0, 0 } }, { { 683, 324, 0, 0 } } };
  metadata[offsets[3023] + 0] = { 6, 154, false, { { 0, 0, 1, 0, -1, -1, 1, 0, 1, 0, -1, 0 } }, { { 138, 16, 0, 0 } }, { { 2699, 324, 0, 0 } } };
  metadata[offsets[3055] + 0] = { 6, 154, false, { { 0, 0, 1, 0, -1, 0, 1, 0, 1, 0, -1, 0 } }, { { 138, 16, 0, 0 } }, { { 2731, 324, 0, 0 } } };
  metadata[offsets[3279] + 1] = { 6, 106, true, { { 0, 0, 0, 1, -1, -1, 1, 0, -1, -1, 1, 0 } }, { { 42, 64, 0, 0 } }, { { 2183, 1096, 0, 0 } } };
  metadata[offsets[3311] + 0] = { 6, 106, false, { { 0, 0, 0, 1, -1, 0, 1, 0, -1, -1, 1, 0 } }, { { 42, 64, 0, 0 } }, { { 2215, 1096, 0, 0 } } };
  metadata[offsets[3791] + 0] = { 6, 106, false, { { 0, 0, 0, 1, -1, -1, 1, 0, -1, 0, 1, 0 } }, { { 42, 64, 0, 0 } }, { { 2695, 1096, 0, 0 } } };
  metadata[offsets[3823] + 0] = { 6, 106, false, { { 0, 0, 0, 1, -1, 0, 1, 0, -1, 0, 1, 0 } }, { { 42, 64, 0, 0 } }, { { 2727, 1096, 0, 0 } } };
  metadata[offsets[4005] + 1] = { 6, 210, true, { { 0, -1, 1, -1, -1, 0, -1, 1, 1, 0, 1, 1 } }, { { 2, 208, 0, 0 } }, { { 545, 3460, 0, 0 } } };
  metadata[offsets[4013] + 0] = { 6, 210, false, { { 0, -1, 1, 1, -1, 0, -1, 1, 1, 0, 1, 1 } }, { { 2, 208, 0, 0 } }, { { 545, 3468, 0, 0 } } };
  metadata[offsets[4069] + 0] = { 6, 210, false, { { 0, -1, 1, -1, -1, 0, 1, 1, 1, 0, 1, 1 } }, { { 2, 208, 0, 0 } }, { { 545, 3524, 0, 0 } } };
  metadata[offsets[4077] + 0] = { 6, 210, false, { { 0, -1, 1, 1, -1, 0, 1, 1, 1, 0, 1, 1 } }, { { 2, 208, 0, 0 } }, { { 545, 3532, 0, 0 } } };
  metadata[offsets[1532] + 1] = { 6, 156, true, { { -1, -1, 1, 0, 0, 0, 1, 0, 1, -1, 0, -1 } }, { { 140, 16, 0, 0 } }, { { 1208, 324, 0, 0 } } };
  metadata[offsets[1534] + 0] = { 6, 156, false, { { -1, 0, 1, 0, 0, 0, 1, 0, 1, -1, 0, -1 } }, { { 140, 16, 0, 0 } }, { { 1210, 324, 0, 0 } } };
  metadata[offsets[3580] + 0] = { 6, 156, false, { { -1, -1, 1, 0, 0, 0, 1, 0, 1, -1, 0, 0 } }, { { 140, 16, 0, 0 } }, { { 3256, 324, 0, 0 } } };
  metadata[offsets[3582] + 0] = { 6, 156, false, { { -1, 0, 1, 0, 0, 0, 1, 0, 1, -1, 0, 0 } }, { { 140, 16, 0, 0 } }, { { 3258, 324, 0, 0 } } };
  metadata[offsets[2812] + 1] = { 6, 108, true, { { -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, -1, 0 } }, { { 76, 32, 0, 0 } }, { { 2168, 644, 0, 0 } } };
  metadata[offsets[2814] + 0] = { 6, 108, false, { { -1, 0, 1, 0, 0, 0, 0, 1, -1, 1, -1, 0 } }, { { 76, 32, 0, 0 } }, { { 2170, 644, 0, 0 } } };
  metadata[offsets[3836] + 0] = { 6, 108, false, { { -1, -1, 1, 0, 0, 0, 0, 1, -1, 1, 0, 0 } }, { { 76, 32, 0, 0 } }, { { 3192, 644, 0, 0 } } };
  metadata[offsets[3838] + 0] = { 6, 108, false, { { -1, 0, 1, 0, 0, 0, 0, 1, -1, 1, 0, 0 } }, { { 76, 32, 0, 0 } }, { { 3194, 644, 0, 0 } } };
  metadata[offsets[3930] + 1] = { 6, 180, true, { { -1, 0, -1, 1, 0, -1, 1, -1, 1, 1, 0, 1 } }, { { 4, 176, 0, 0 } }, { { 1042, 2888, 0, 0 } } };
  metadata[offsets[3934] + 0] = { 6, 180, false, { { -1, 0, 1, 1, 0, -1, 1, -1, 1, 1, 0, 1 } }, { { 4, 176, 0, 0 } }, { { 1042, 2892, 0, 0 } } };
  metadata[offsets[4058] + 0] = { 6, 180, false, { { -1, 0, -1, 1, 0, -1, 1, 1, 1, 1, 0, 1 } }, { { 4, 176, 0, 0 } }, { { 1042, 3016, 0, 0 } } };
  metadata[offsets[4062] + 0] = { 6, 180, false, { { -1, 0, 1, 1, 0, -1, 1, 1, 1, 1, 0, 1 } }, { { 4, 176, 0, 0 } }, { { 1042, 3020, 0, 0 } } };
  metadata[offsets[4010] + 1] = { 6, 120, true, { { -1, 0, -1, 1, -1, 0, -1, 1, 1, 1, 1, 0 } }, { { 8, 112, 0, 0 } }, { { 2082, 1928, 0, 0 } } };
  metadata[offsets[4014] + 0] = { 6, 120, false, { { -1, 0, 1, 1, -1, 0, -1, 1, 1, 1, 1, 0 } }, { { 8, 112, 0, 0 } }, { { 2082, 1932, 0, 0 } } };
  metadata[offsets[4074] + 0] = { 6, 120, false, { { -1, 0, -1, 1, -1, 0, 1, 1, 1, 1, 1, 0 } }, { { 8, 112, 0, 0 } }, { { 2082, 1992, 0, 0 } } };
  metadata[offsets[4078] + 0] = { 6, 120, false, { { -1, 0, 1, 1, -1, 0, 1, 1, 1, 1, 1, 0 } }, { { 8, 112, 0, 0 } }, { { 2082, 1996, 0, 0 } } };
  // Processing non-manifold case 7 with 2 voxel cases
  metadata[offsets[4095] + 0] = { 7, 105, true, { { 0, 1, 2, 3, 0, 1, 3, 2, 0, 2, 3, 1 } }, { { 1, 8, 32, 64 } }, { { 273, 2082, 644, 1096 } } };
  metadata[offsets[4095] + 1] = { 7, 150, true, { { 0, 1, 2, 3, 1, 0, 2, 3, 2, 0, 1, 3 } }, { { 2, 4, 16, 128 } }, { { 545, 1042, 324, 2184 } } };
  // Processing non-manifold case 8 with 8 voxel cases
  metadata[offsets[3822] + 1] = { 8, 151, true, { { -1, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1638, 2184, 0, 0 } } };
  metadata[offsets[3823] + 1] = { 8, 151, false, { { 0, 0, 0, 1, -1, 0, 0, 1, -1, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1639, 2184, 0, 0 } } };
  metadata[offsets[3838] + 1] = { 8, 151, false, { { -1, 0, 0, 1, 0, 0, 0, 1, -1, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1654, 2184, 0, 0 } } };
  metadata[offsets[3839] + 0] = { 8, 151, false, { { 0, 0, 0, 1, 0, 0, 0, 1, -1, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1655, 2184, 0, 0 } } };
  metadata[offsets[4078] + 1] = { 8, 151, false, { { -1, 0, 0, 1, -1, 0, 0, 1, 0, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1894, 2184, 0, 0 } } };
  metadata[offsets[4079] + 0] = { 8, 151, false, { { 0, 0, 0, 1, -1, 0, 0, 1, 0, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1895, 2184, 0, 0 } } };
  metadata[offsets[4094] + 0] = { 8, 151, false, { { -1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1910, 2184, 0, 0 } } };
  metadata[offsets[4095] + 2] = { 8, 151, false, { { 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1 } }, { { 23, 128, 0, 0 } }, { { 1911, 2184, 0, 0 } } };
  metadata[offsets[3550] + 1] = { 8, 107, true, { { -1, 0, 0, 1, 0, -1, 1, 0, 0, -1, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2454, 1096, 0, 0 } } };
  metadata[offsets[3551] + 1] = { 8, 107, false, { { 0, 0, 0, 1, 0, -1, 1, 0, 0, -1, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2455, 1096, 0, 0 } } };
  metadata[offsets[3582] + 1] = { 8, 107, false, { { -1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2486, 1096, 0, 0 } } };
  metadata[offsets[3583] + 0] = { 8, 107, false, { { 0, 0, 0, 1, 0, 0, 1, 0, 0, -1, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2487, 1096, 0, 0 } } };
  metadata[offsets[4062] + 1] = { 8, 107, false, { { -1, 0, 0, 1, 0, -1, 1, 0, 0, 0, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2966, 1096, 0, 0 } } };
  metadata[offsets[4063] + 0] = { 8, 107, false, { { 0, 0, 0, 1, 0, -1, 1, 0, 0, 0, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2967, 1096, 0, 0 } } };
  metadata[offsets[4094] + 1] = { 8, 107, false, { { -1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2998, 1096, 0, 0 } } };
  metadata[offsets[4095] + 3] = { 8, 107, false, { { 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0 } }, { { 43, 64, 0, 0 } }, { { 2999, 1096, 0, 0 } } };
  metadata[offsets[3053] + 1] = { 8, 109, true, { { 0, -1, 1, 0, -1, 0, 0, 1, 0, 1, -1, 0 } }, { { 77, 32, 0, 0 } }, { { 2409, 644, 0, 0 } } };
  metadata[offsets[3055] + 1] = { 8, 109, false, { { 0, 0, 1, 0, -1, 0, 0, 1, 0, 1, -1, 0 } }, { { 77, 32, 0, 0 } }, { { 2411, 644, 0, 0 } } };
  metadata[offsets[3069] + 1] = { 8, 109, false, { { 0, -1, 1, 0, 0, 0, 0, 1, 0, 1, -1, 0 } }, { { 77, 32, 0, 0 } }, { { 2425, 644, 0, 0 } } };
  metadata[offsets[3071] + 0] = { 8, 109, false, { { 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, -1, 0 } }, { { 77, 32, 0, 0 } }, { { 2427, 644, 0, 0 } } };
  metadata[offsets[4077] + 1] = { 8, 109, false, { { 0, -1, 1, 0, -1, 0, 0, 1, 0, 1, 0, 0 } }, { { 77, 32, 0, 0 } }, { { 3433, 644, 0, 0 } } };
  metadata[offsets[4079] + 1] = { 8, 109, false, { { 0, 0, 1, 0, -1, 0, 0, 1, 0, 1, 0, 0 } }, { { 77, 32, 0, 0 } }, { { 3435, 644, 0, 0 } } };
  metadata[offsets[4093] + 0] = { 8, 109, false, { { 0, -1, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0 } }, { { 77, 32, 0, 0 } }, { { 3449, 644, 0, 0 } } };
  metadata[offsets[4095] + 4] = { 8, 109, false, { { 0, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0 } }, { { 77, 32, 0, 0 } }, { { 3451, 644, 0, 0 } } };
  metadata[offsets[3771] + 1] = { 8, 121, true, { { 0, 1, -1, 0, 0, 1, -1, 0, -1, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 1689, 2082, 0, 0 } } };
  metadata[offsets[3775] + 1] = { 8, 121, false, { { 0, 1, 0, 0, 0, 1, -1, 0, -1, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 1693, 2082, 0, 0 } } };
  metadata[offsets[3835] + 1] = { 8, 121, false, { { 0, 1, -1, 0, 0, 1, 0, 0, -1, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 1753, 2082, 0, 0 } } };
  metadata[offsets[3839] + 1] = { 8, 121, false, { { 0, 1, 0, 0, 0, 1, 0, 0, -1, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 1757, 2082, 0, 0 } } };
  metadata[offsets[4027] + 1] = { 8, 121, false, { { 0, 1, -1, 0, 0, 1, -1, 0, 0, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 1945, 2082, 0, 0 } } };
  metadata[offsets[4031] + 0] = { 8, 121, false, { { 0, 1, 0, 0, 0, 1, -1, 0, 0, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 1949, 2082, 0, 0 } } };
  metadata[offsets[4091] + 0] = { 8, 121, false, { { 0, 1, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 2009, 2082, 0, 0 } } };
  metadata[offsets[4095] + 5] = { 8, 121, false, { { 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1 } }, { { 113, 8, 0, 0 } }, { { 2013, 2082, 0, 0 } } };
  metadata[offsets[1911] + 1] = { 8, 233, true, { { 0, 1, 1, -1, 0, 1, 1, -1, 0, 1, 1, -1 } }, { { 1, 232, 0, 0 } }, { { 273, 1638, 0, 0 } } };
  metadata[offsets[1919] + 1] = { 8, 233, false, { { 0, 1, 1, 1, 0, 1, 1, -1, 0, 1, 1, -1 } }, { { 1, 232, 0, 0 } }, { { 273, 1646, 0, 0 } } };
  metadata[offsets[2039] + 1] = { 8, 233, false, { { 0, 1, 1, -1, 0, 1, 1, 1, 0, 1, 1, -1 } }, { { 1, 232, 0, 0 } }, { { 273, 1766, 0, 0 } } };
  metadata[offsets[2047] + 0] = { 8, 233, false, { { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, -1 } }, { { 1, 232, 0, 0 } }, { { 273, 1774, 0, 0 } } };
  metadata[offsets[3959] + 1] = { 8, 233, false, { { 0, 1, 1, -1, 0, 1, 1, -1, 0, 1, 1, 1 } }, { { 1, 232, 0, 0 } }, { { 273, 3686, 0, 0 } } };
  metadata[offsets[3967] + 0] = { 8, 233, false, { { 0, 1, 1, 1, 0, 1, 1, -1, 0, 1, 1, 1 } }, { { 1, 232, 0, 0 } }, { { 273, 3694, 0, 0 } } };
  metadata[offsets[4087] + 0] = { 8, 233, false, { { 0, 1, 1, -1, 0, 1, 1, 1, 0, 1, 1, 1 } }, { { 1, 232, 0, 0 } }, { { 273, 3814, 0, 0 } } };
  metadata[offsets[4095] + 6] = { 8, 233, false, { { 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1 } }, { { 1, 232, 0, 0 } }, { { 273, 3822, 0, 0 } } };
  metadata[offsets[2013] + 1] = { 8, 158, true, { { 0, -1, 1, 0, 0, -1, 1, 0, 1, 0, 0, -1 } }, { { 142, 16, 0, 0 } }, { { 1689, 324, 0, 0 } } };
  metadata[offsets[2015] + 1] = { 8, 158, false, { { 0, 0, 1, 0, 0, -1, 1, 0, 1, 0, 0, -1 } }, { { 142, 16, 0, 0 } }, { { 1691, 324, 0, 0 } } };
  metadata[offsets[2045] + 1] = { 8, 158, false, { { 0, -1, 1, 0, 0, 0, 1, 0, 1, 0, 0, -1 } }, { { 142, 16, 0, 0 } }, { { 1721, 324, 0, 0 } } };
  metadata[offsets[2047] + 1] = { 8, 158, false, { { 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, -1 } }, { { 142, 16, 0, 0 } }, { { 1723, 324, 0, 0 } } };
  metadata[offsets[4061] + 1] = { 8, 158, false, { { 0, -1, 1, 0, 0, -1, 1, 0, 1, 0, 0, 0 } }, { { 142, 16, 0, 0 } }, { { 3737, 324, 0, 0 } } };
  metadata[offsets[4063] + 1] = { 8, 158, false, { { 0, 0, 1, 0, 0, -1, 1, 0, 1, 0, 0, 0 } }, { { 142, 16, 0, 0 } }, { { 3739, 324, 0, 0 } } };
  metadata[offsets[4093] + 1] = { 8, 158, false, { { 0, -1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0 } }, { { 142, 16, 0, 0 } }, { { 3769, 324, 0, 0 } } };
  metadata[offsets[4095] + 7] = { 8, 158, false, { { 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0 } }, { { 142, 16, 0, 0 } }, { { 3771, 324, 0, 0 } } };
  metadata[offsets[3451] + 1] = { 8, 182, true, { { 0, 1, -1, 0, 1, 0, 0, -1, 0, -1, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 2409, 1042, 0, 0 } } };
  metadata[offsets[3455] + 1] = { 8, 182, false, { { 0, 1, 0, 0, 1, 0, 0, -1, 0, -1, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 2413, 1042, 0, 0 } } };
  metadata[offsets[3579] + 1] = { 8, 182, false, { { 0, 1, -1, 0, 1, 0, 0, 0, 0, -1, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 2537, 1042, 0, 0 } } };
  metadata[offsets[3583] + 1] = { 8, 182, false, { { 0, 1, 0, 0, 1, 0, 0, 0, 0, -1, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 2541, 1042, 0, 0 } } };
  metadata[offsets[3963] + 1] = { 8, 182, false, { { 0, 1, -1, 0, 1, 0, 0, -1, 0, 0, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 2921, 1042, 0, 0 } } };
  metadata[offsets[3967] + 1] = { 8, 182, false, { { 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 2925, 1042, 0, 0 } } };
  metadata[offsets[4091] + 1] = { 8, 182, false, { { 0, 1, -1, 0, 1, 0, 0, 0, 0, 0, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 3049, 1042, 0, 0 } } };
  metadata[offsets[4095] + 8] = { 8, 182, false, { { 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0 } }, { { 178, 4, 0, 0 } }, { { 3053, 1042, 0, 0 } } };
  metadata[offsets[2999] + 1] = { 8, 214, true, { { 0, 1, 1, -1, 1, 0, -1, 1, 1, 0, -1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 2454, 0, 0 } } };
  metadata[offsets[3007] + 1] = { 8, 214, false, { { 0, 1, 1, 1, 1, 0, -1, 1, 1, 0, -1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 2462, 0, 0 } } };
  metadata[offsets[3063] + 1] = { 8, 214, false, { { 0, 1, 1, -1, 1, 0, 1, 1, 1, 0, -1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 2518, 0, 0 } } };
  metadata[offsets[3071] + 1] = { 8, 214, false, { { 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, -1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 2526, 0, 0 } } };
  metadata[offsets[4023] + 1] = { 8, 214, false, { { 0, 1, 1, -1, 1, 0, -1, 1, 1, 0, 1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 3478, 0, 0 } } };
  metadata[offsets[4031] + 1] = { 8, 214, false, { { 0, 1, 1, 1, 1, 0, -1, 1, 1, 0, 1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 3486, 0, 0 } } };
  metadata[offsets[4087] + 1] = { 8, 214, false, { { 0, 1, 1, -1, 1, 0, 1, 1, 1, 0, 1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 3542, 0, 0 } } };
  metadata[offsets[4095] + 9] = { 8, 214, false, { { 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1 } }, { { 2, 212, 0, 0 } }, { { 545, 3550, 0, 0 } } };
  // Processing non-manifold case 9 with 24 voxel cases
  metadata[offsets[494] + 1] = { 9, 103, true, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1 } }, { { 103, 0, 0, 0 } }, { { 494, 0, 0, 0 } } };
  metadata[offsets[511] + 1] = { 9, 103, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 103, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[1007] + 1] = { 9, 103, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 103, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[1022] + 0] = { 9, 103, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 103, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1023] + 0] = { 9, 103, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 103, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1519] + 0] = { 9, 103, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 103, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1534] + 1] = { 9, 103, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 103, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[1535] + 0] = { 9, 103, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 103, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[2030] + 0] = { 9, 103, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 103, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[2031] + 0] = { 9, 103, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 103, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2046] + 0] = { 9, 103, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 103, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 2] = { 9, 103, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 103, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3374] + 1] = { 9, 167, true, { { -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3374, 0, 0, 0 } } };
  metadata[offsets[3391] + 1] = { 9, 167, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3503] + 0] = { 9, 167, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3518] + 0] = { 9, 167, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3519] + 0] = { 9, 167, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3887] + 1] = { 9, 167, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3902] + 0] = { 9, 167, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3903] + 0] = { 9, 167, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[4014] + 1] = { 9, 167, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4015] + 0] = { 9, 167, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4030] + 0] = { 9, 167, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 2] = { 9, 167, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 167, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[3042] + 1] = { 9, 199, true, { { -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 199, 0, 0, 0 } }, { { 3042, 0, 0, 0 } } };
  metadata[offsets[3051] + 0] = { 9, 199, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 199, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3059] + 1] = { 9, 199, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 199, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3066] + 0] = { 9, 199, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 199, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3067] + 0] = { 9, 199, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 199, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[4067] + 0] = { 9, 199, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 199, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4074] + 1] = { 9, 199, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 199, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4075] + 0] = { 9, 199, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 199, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4082] + 1] = { 9, 199, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 199, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4083] + 0] = { 9, 199, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 199, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4090] + 0] = { 9, 199, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 199, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 2] = { 9, 199, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 199, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[3614] + 1] = { 9, 91, true, { { -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3614, 0, 0, 0 } } };
  metadata[offsets[3647] + 1] = { 9, 91, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3679] + 0] = { 9, 91, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3710] + 0] = { 9, 91, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3711] + 0] = { 9, 91, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3871] + 1] = { 9, 91, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3902] + 1] = { 9, 91, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3903] + 1] = { 9, 91, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3934] + 1] = { 9, 91, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3935] + 0] = { 9, 91, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3966] + 0] = { 9, 91, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 2] = { 9, 91, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 91, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[734] + 1] = { 9, 155, true, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1 } }, { { 155, 0, 0, 0 } }, { { 734, 0, 0, 0 } } };
  metadata[offsets[767] + 1] = { 9, 155, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 155, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[991] + 1] = { 9, 155, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 155, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[1022] + 1] = { 9, 155, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 155, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1023] + 1] = { 9, 155, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 155, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[2783] + 0] = { 9, 155, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 155, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2814] + 1] = { 9, 155, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 155, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[2815] + 0] = { 9, 155, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 155, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[3038] + 0] = { 9, 155, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 155, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3039] + 0] = { 9, 155, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 155, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3070] + 0] = { 9, 155, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 155, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 2] = { 9, 155, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 155, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[2002] + 1] = { 9, 203, true, { { -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 203, 0, 0, 0 } }, { { 2002, 0, 0, 0 } } };
  metadata[offsets[2011] + 0] = { 9, 203, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 203, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2035] + 1] = { 9, 203, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 203, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2042] + 0] = { 9, 203, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 203, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2043] + 0] = { 9, 203, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 203, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[4051] + 0] = { 9, 203, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 203, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4058] + 1] = { 9, 203, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 203, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4059] + 0] = { 9, 203, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 203, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4082] + 2] = { 9, 203, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 203, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4083] + 1] = { 9, 203, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 203, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4090] + 1] = { 9, 203, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 203, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 3] = { 9, 203, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 203, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[3764] + 1] = { 9, 211, true, { { -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 3764, 0, 0, 0 } } };
  metadata[offsets[3773] + 0] = { 9, 211, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3829] + 0] = { 9, 211, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3836] + 1] = { 9, 211, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3837] + 0] = { 9, 211, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[4021] + 1] = { 9, 211, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4028] + 0] = { 9, 211, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4029] + 0] = { 9, 211, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4084] + 1] = { 9, 211, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4085] + 0] = { 9, 211, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4092] + 0] = { 9, 211, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 2] = { 9, 211, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 211, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[3444] + 1] = { 9, 227, true, { { -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3444, 0, 0, 0 } } };
  metadata[offsets[3453] + 0] = { 9, 227, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3573] + 0] = { 9, 227, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3580] + 1] = { 9, 227, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3581] + 0] = { 9, 227, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3957] + 1] = { 9, 227, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3964] + 0] = { 9, 227, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3965] + 0] = { 9, 227, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[4084] + 2] = { 9, 227, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4085] + 1] = { 9, 227, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4092] + 1] = { 9, 227, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 3] = { 9, 227, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 227, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[3809] + 1] = { 9, 61, true, { { 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 3809, 0, 0, 0 } } };
  metadata[offsets[3815] + 0] = { 9, 61, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3827] + 1] = { 9, 61, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3829] + 1] = { 9, 61, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3831] + 0] = { 9, 61, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[4067] + 1] = { 9, 61, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4069] + 1] = { 9, 61, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4071] + 0] = { 9, 61, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4081] + 1] = { 9, 61, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4083] + 2] = { 9, 61, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4085] + 2] = { 9, 61, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4087] + 2] = { 9, 61, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 61, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[1261] + 1] = { 9, 157, true, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1 } }, { { 157, 0, 0, 0 } }, { { 1261, 0, 0, 0 } } };
  metadata[offsets[1279] + 1] = { 9, 157, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 157, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1519] + 1] = { 9, 157, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 157, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1533] + 1] = { 9, 157, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 157, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1535] + 1] = { 9, 157, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 157, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[3311] + 1] = { 9, 157, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 157, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3325] + 0] = { 9, 157, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 157, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3327] + 0] = { 9, 157, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 157, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3565] + 0] = { 9, 157, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 157, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3567] + 0] = { 9, 157, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 157, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3581] + 1] = { 9, 157, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 157, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3583] + 2] = { 9, 157, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 157, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[1837] + 1] = { 9, 173, true, { { 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1 } }, { { 173, 0, 0, 0 } }, { { 1837, 0, 0, 0 } } };
  metadata[offsets[1855] + 1] = { 9, 173, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 173, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1967] + 0] = { 9, 173, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 173, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1981] + 0] = { 9, 173, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 173, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[1983] + 0] = { 9, 173, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 173, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[3887] + 2] = { 9, 173, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 173, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3901] + 0] = { 9, 173, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 173, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3903] + 2] = { 9, 173, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 173, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[4013] + 1] = { 9, 173, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 173, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4015] + 1] = { 9, 173, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 173, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4029] + 1] = { 9, 173, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 173, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4031] + 3] = { 9, 173, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 173, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[3659] + 1] = { 9, 181, true, { { 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3659, 0, 0, 0 } } };
  metadata[offsets[3679] + 1] = { 9, 181, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3791] + 1] = { 9, 181, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3803] + 0] = { 9, 181, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3807] + 0] = { 9, 181, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3919] + 1] = { 9, 181, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3931] + 1] = { 9, 181, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3935] + 1] = { 9, 181, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[4043] + 0] = { 9, 181, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4047] + 0] = { 9, 181, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4059] + 1] = { 9, 181, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4063] + 2] = { 9, 181, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 181, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[2887] + 1] = { 9, 229, true, { { 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0 } }, { { 229, 0, 0, 0 } }, { { 2887, 0, 0, 0 } } };
  metadata[offsets[2911] + 0] = { 9, 229, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 229, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[3023] + 1] = { 9, 229, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 229, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3031] + 0] = { 9, 229, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 229, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3039] + 1] = { 9, 229, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 229, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3919] + 2] = { 9, 229, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 229, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3927] + 1] = { 9, 229, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 229, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3935] + 2] = { 9, 229, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 229, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[4039] + 0] = { 9, 229, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 229, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4047] + 1] = { 9, 229, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 229, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4055] + 0] = { 9, 229, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 229, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4063] + 3] = { 9, 229, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 229, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[635] + 1] = { 9, 185, true, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1 } }, { { 185, 0, 0, 0 } }, { { 635, 0, 0, 0 } } };
  metadata[offsets[767] + 2] = { 9, 185, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 185, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[895] + 1] = { 9, 185, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 185, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[1019] + 0] = { 9, 185, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 185, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[1023] + 2] = { 9, 185, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 185, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[2687] + 0] = { 9, 185, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 185, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2811] + 1] = { 9, 185, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 185, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[2815] + 1] = { 9, 185, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 185, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2939] + 0] = { 9, 185, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 185, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[2943] + 0] = { 9, 185, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 185, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[3067] + 1] = { 9, 185, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 185, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3071] + 3] = { 9, 185, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 185, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[1207] + 1] = { 9, 217, true, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1 } }, { { 217, 0, 0, 0 } }, { { 1207, 0, 0, 0 } } };
  metadata[offsets[1279] + 2] = { 9, 217, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 217, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1471] + 0] = { 9, 217, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 217, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1527] + 1] = { 9, 217, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 217, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1535] + 2] = { 9, 217, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 217, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[3263] + 1] = { 9, 217, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 217, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3319] + 0] = { 9, 217, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 217, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3327] + 1] = { 9, 217, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 217, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3511] + 0] = { 9, 217, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 217, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3519] + 1] = { 9, 217, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 217, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3575] + 0] = { 9, 217, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 217, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3583] + 3] = { 9, 217, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 217, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3537] + 1] = { 9, 62, true, { { 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 3537, 0, 0, 0 } } };
  metadata[offsets[3543] + 0] = { 9, 62, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3571] + 1] = { 9, 62, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3573] + 1] = { 9, 62, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3575] + 1] = { 9, 62, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[4051] + 1] = { 9, 62, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4053] + 1] = { 9, 62, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4055] + 1] = { 9, 62, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4081] + 2] = { 9, 62, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4083] + 3] = { 9, 62, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4085] + 3] = { 9, 62, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4087] + 3] = { 9, 62, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 62, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[2845] + 1] = { 9, 94, true, { { 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0 } }, { { 94, 0, 0, 0 } }, { { 2845, 0, 0, 0 } } };
  metadata[offsets[2879] + 1] = { 9, 94, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 94, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2911] + 1] = { 9, 94, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 94, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[2941] + 0] = { 9, 94, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 94, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[2943] + 1] = { 9, 94, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 94, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[3871] + 2] = { 9, 94, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 94, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3901] + 1] = { 9, 94, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 94, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3903] + 3] = { 9, 94, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 94, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3933] + 1] = { 9, 94, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 94, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3935] + 3] = { 9, 94, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 94, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3965] + 1] = { 9, 94, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 94, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3967] + 3] = { 9, 94, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 94, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[2269] + 1] = { 9, 110, true, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0 } }, { { 110, 0, 0, 0 } }, { { 2269, 0, 0, 0 } } };
  metadata[offsets[2303] + 1] = { 9, 110, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 110, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2783] + 1] = { 9, 110, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 110, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2813] + 1] = { 9, 110, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 110, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2815] + 2] = { 9, 110, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 110, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[3295] + 1] = { 9, 110, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 110, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3325] + 1] = { 9, 110, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 110, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3327] + 2] = { 9, 110, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 110, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3805] + 0] = { 9, 110, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 110, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3807] + 1] = { 9, 110, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 110, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3837] + 1] = { 9, 110, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 110, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3839] + 2] = { 9, 110, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 110, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[443] + 1] = { 9, 118, true, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1 } }, { { 118, 0, 0, 0 } }, { { 443, 0, 0, 0 } } };
  metadata[offsets[511] + 2] = { 9, 118, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 118, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[959] + 1] = { 9, 118, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 118, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[1019] + 1] = { 9, 118, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 118, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[1023] + 3] = { 9, 118, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 118, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1471] + 1] = { 9, 118, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 118, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1531] + 1] = { 9, 118, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 118, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[1535] + 3] = { 9, 118, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 118, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1979] + 0] = { 9, 118, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 118, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[1983] + 1] = { 9, 118, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 118, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2043] + 1] = { 9, 118, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 118, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2047] + 3] = { 9, 118, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 118, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2167] + 1] = { 9, 230, true, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0 } }, { { 230, 0, 0, 0 } }, { { 2167, 0, 0, 0 } } };
  metadata[offsets[2303] + 2] = { 9, 230, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 230, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2687] + 1] = { 9, 230, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 230, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2807] + 1] = { 9, 230, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 230, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[2815] + 3] = { 9, 230, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 230, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[3199] + 1] = { 9, 230, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 230, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3319] + 1] = { 9, 230, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 230, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3327] + 3] = { 9, 230, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 230, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3703] + 0] = { 9, 230, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 230, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3711] + 1] = { 9, 230, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 230, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3831] + 1] = { 9, 230, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 230, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3839] + 3] = { 9, 230, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 230, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3467] + 1] = { 9, 122, true, { { 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 3467, 0, 0, 0 } } };
  metadata[offsets[3503] + 1] = { 9, 122, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3535] + 1] = { 9, 122, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3563] + 0] = { 9, 122, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3567] + 1] = { 9, 122, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3983] + 1] = { 9, 122, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[4011] + 1] = { 9, 122, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4015] + 2] = { 9, 122, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4043] + 1] = { 9, 122, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4047] + 2] = { 9, 122, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4075] + 1] = { 9, 122, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4079] + 2] = { 9, 122, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 122, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[1927] + 1] = { 9, 218, true, { { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1 } }, { { 218, 0, 0, 0 } }, { { 1927, 0, 0, 0 } } };
  metadata[offsets[1967] + 1] = { 9, 218, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 218, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1999] + 1] = { 9, 218, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 218, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2023] + 0] = { 9, 218, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 218, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2031] + 1] = { 9, 218, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 218, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[3983] + 2] = { 9, 218, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 218, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[4007] + 1] = { 9, 218, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 218, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4015] + 3] = { 9, 218, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 218, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4039] + 1] = { 9, 218, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 218, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4047] + 3] = { 9, 218, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 218, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4071] + 1] = { 9, 218, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 218, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4079] + 3] = { 9, 218, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 218, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[3000] + 1] = { 9, 124, true, { { -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 124, 0, 0, 0 } }, { { 3000, 0, 0, 0 } } };
  metadata[offsets[3006] + 0] = { 9, 124, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 124, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3066] + 1] = { 9, 124, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 124, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3068] + 1] = { 9, 124, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 124, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3070] + 1] = { 9, 124, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 124, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[4026] + 1] = { 9, 124, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 124, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4028] + 1] = { 9, 124, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 124, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4030] + 1] = { 9, 124, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 124, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4088] + 1] = { 9, 124, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 124, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4090] + 2] = { 9, 124, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 124, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4092] + 2] = { 9, 124, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 124, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4094] + 2] = { 9, 124, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 124, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[1912] + 1] = { 9, 188, true, { { -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 188, 0, 0, 0 } }, { { 1912, 0, 0, 0 } } };
  metadata[offsets[1918] + 0] = { 9, 188, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 188, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[2042] + 1] = { 9, 188, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 188, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2044] + 1] = { 9, 188, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 188, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2046] + 1] = { 9, 188, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 188, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[3962] + 1] = { 9, 188, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 188, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3964] + 1] = { 9, 188, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 188, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3966] + 1] = { 9, 188, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 188, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[4088] + 2] = { 9, 188, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 188, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4090] + 3] = { 9, 188, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 188, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4092] + 3] = { 9, 188, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 188, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4094] + 3] = { 9, 188, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 188, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  // Processing non-manifold case 10 with 12 voxel cases
  metadata[offsets[1740] + 1] = { 10, 159, true, { { -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 1740, 0, 0, 0 } } };
  metadata[offsets[1758] + 1] = { 10, 159, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 1758, 0, 0, 0 } } };
  metadata[offsets[1773] + 1] = { 10, 159, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 1773, 0, 0, 0 } } };
  metadata[offsets[1791] + 1] = { 10, 159, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1999] + 2] = { 10, 159, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2013] + 2] = { 10, 159, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2014] + 0] = { 10, 159, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2015] + 2] = { 10, 159, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2029] + 0] = { 10, 159, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[2030] + 1] = { 10, 159, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[2031] + 2] = { 10, 159, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2044] + 2] = { 10, 159, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2045] + 2] = { 10, 159, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2046] + 2] = { 10, 159, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 4] = { 10, 159, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 159, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3791] + 2] = { 10, 159, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3805] + 1] = { 10, 159, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3806] + 0] = { 10, 159, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[3807] + 2] = { 10, 159, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3821] + 0] = { 10, 159, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[3822] + 2] = { 10, 159, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 2] = { 10, 159, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3836] + 2] = { 10, 159, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3837] + 2] = { 10, 159, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3838] + 2] = { 10, 159, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 4] = { 10, 159, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[4044] + 0] = { 10, 159, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  metadata[offsets[4047] + 4] = { 10, 159, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4061] + 2] = { 10, 159, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4062] + 2] = { 10, 159, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 4] = { 10, 159, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4077] + 2] = { 10, 159, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4078] + 2] = { 10, 159, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 4] = { 10, 159, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4092] + 4] = { 10, 159, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 4] = { 10, 159, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4094] + 4] = { 10, 159, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 10] = { 10, 159, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 159, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[2508] + 1] = { 10, 111, true, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 2508, 0, 0, 0 } } };
  metadata[offsets[2525] + 1] = { 10, 111, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 2525, 0, 0, 0 } } };
  metadata[offsets[2542] + 1] = { 10, 111, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 2542, 0, 0, 0 } } };
  metadata[offsets[2559] + 1] = { 10, 111, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[3023] + 2] = { 10, 111, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3037] + 0] = { 10, 111, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3038] + 1] = { 10, 111, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3039] + 2] = { 10, 111, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3053] + 2] = { 10, 111, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3054] + 0] = { 10, 111, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[3055] + 2] = { 10, 111, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3068] + 2] = { 10, 111, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3069] + 2] = { 10, 111, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3070] + 2] = { 10, 111, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 4] = { 10, 111, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 111, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3535] + 2] = { 10, 111, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3549] + 0] = { 10, 111, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3550] + 2] = { 10, 111, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 2] = { 10, 111, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3565] + 1] = { 10, 111, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3566] + 0] = { 10, 111, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  metadata[offsets[3567] + 2] = { 10, 111, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3580] + 2] = { 10, 111, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3581] + 2] = { 10, 111, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3582] + 2] = { 10, 111, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 4] = { 10, 111, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[4044] + 1] = { 10, 111, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  metadata[offsets[4047] + 5] = { 10, 111, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4061] + 3] = { 10, 111, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4062] + 3] = { 10, 111, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 5] = { 10, 111, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4077] + 3] = { 10, 111, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4078] + 3] = { 10, 111, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 5] = { 10, 111, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4092] + 5] = { 10, 111, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 5] = { 10, 111, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4094] + 5] = { 10, 111, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 11] = { 10, 111, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 111, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[3178] + 1] = { 10, 183, true, { { -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3178, 0, 0, 0 } } };
  metadata[offsets[3199] + 2] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3311] + 2] = { 10, 183, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3322] + 0] = { 10, 183, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[3327] + 4] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3438] + 1] = { 10, 183, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3438, 0, 0, 0 } } };
  metadata[offsets[3451] + 2] = { 10, 183, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3454] + 0] = { 10, 183, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[3455] + 2] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3563] + 1] = { 10, 183, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3566] + 1] = { 10, 183, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  metadata[offsets[3567] + 3] = { 10, 183, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3579] + 2] = { 10, 183, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3582] + 3] = { 10, 183, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 5] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3691] + 1] = { 10, 183, false, { { 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3691, 0, 0, 0 } } };
  metadata[offsets[3707] + 0] = { 10, 183, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[3710] + 1] = { 10, 183, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3711] + 2] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3819] + 0] = { 10, 183, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[3822] + 3] = { 10, 183, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 3] = { 10, 183, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3835] + 2] = { 10, 183, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3838] + 3] = { 10, 183, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 5] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3951] + 1] = { 10, 183, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3962] + 2] = { 10, 183, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3963] + 2] = { 10, 183, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3966] + 2] = { 10, 183, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 4] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4074] + 2] = { 10, 183, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4075] + 2] = { 10, 183, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4078] + 4] = { 10, 183, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 6] = { 10, 183, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4090] + 4] = { 10, 183, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 4] = { 10, 183, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4094] + 6] = { 10, 183, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 12] = { 10, 183, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 183, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[2726] + 1] = { 10, 215, true, { { -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 2726, 0, 0, 0 } } };
  metadata[offsets[2735] + 0] = { 10, 215, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[2807] + 2] = { 10, 215, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[2814] + 2] = { 10, 215, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[2815] + 4] = { 10, 215, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2999] + 2] = { 10, 215, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3006] + 1] = { 10, 215, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3007] + 2] = { 10, 215, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3046] + 1] = { 10, 215, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3046, 0, 0, 0 } } };
  metadata[offsets[3047] + 0] = { 10, 215, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[3054] + 1] = { 10, 215, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[3055] + 3] = { 10, 215, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3063] + 2] = { 10, 215, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3070] + 3] = { 10, 215, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 5] = { 10, 215, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 215, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3766] + 1] = { 10, 215, false, { { -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3766, 0, 0, 0 } } };
  metadata[offsets[3767] + 0] = { 10, 215, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[3774] + 0] = { 10, 215, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[3775] + 2] = { 10, 215, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3815] + 1] = { 10, 215, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3822] + 4] = { 10, 215, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 4] = { 10, 215, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3831] + 2] = { 10, 215, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3838] + 4] = { 10, 215, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 6] = { 10, 215, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[4007] + 2] = { 10, 215, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4014] + 2] = { 10, 215, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4015] + 4] = { 10, 215, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4023] + 2] = { 10, 215, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4030] + 2] = { 10, 215, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 4] = { 10, 215, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4071] + 2] = { 10, 215, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4078] + 5] = { 10, 215, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 7] = { 10, 215, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4086] + 1] = { 10, 215, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 4] = { 10, 215, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4094] + 7] = { 10, 215, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 13] = { 10, 215, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 215, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[3226] + 1] = { 10, 123, true, { { -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3226, 0, 0, 0 } } };
  metadata[offsets[3263] + 2] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3295] + 2] = { 10, 123, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3322] + 1] = { 10, 123, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[3327] + 5] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3483] + 1] = { 10, 123, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3483, 0, 0, 0 } } };
  metadata[offsets[3515] + 0] = { 10, 123, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[3518] + 1] = { 10, 123, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3519] + 2] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3547] + 0] = { 10, 123, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[3550] + 3] = { 10, 123, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 3] = { 10, 123, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3579] + 3] = { 10, 123, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3582] + 4] = { 10, 123, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 6] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3742] + 1] = { 10, 123, false, { { -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3742, 0, 0, 0 } } };
  metadata[offsets[3771] + 2] = { 10, 123, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3774] + 1] = { 10, 123, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[3775] + 3] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3803] + 1] = { 10, 123, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3806] + 1] = { 10, 123, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[3807] + 3] = { 10, 123, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3835] + 3] = { 10, 123, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3838] + 5] = { 10, 123, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 7] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3999] + 1] = { 10, 123, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4026] + 2] = { 10, 123, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4027] + 2] = { 10, 123, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4030] + 3] = { 10, 123, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 5] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4058] + 2] = { 10, 123, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4059] + 2] = { 10, 123, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4062] + 4] = { 10, 123, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 6] = { 10, 123, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4090] + 5] = { 10, 123, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 5] = { 10, 123, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4094] + 8] = { 10, 123, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 14] = { 10, 123, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 123, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[1366] + 1] = { 10, 235, true, { { -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1366, 0, 0, 0 } } };
  metadata[offsets[1375] + 0] = { 10, 235, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[1527] + 2] = { 10, 235, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1534] + 2] = { 10, 235, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[1535] + 4] = { 10, 235, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1911] + 2] = { 10, 235, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1918] + 1] = { 10, 235, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[1919] + 2] = { 10, 235, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[2006] + 1] = { 10, 235, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 2006, 0, 0, 0 } } };
  metadata[offsets[2007] + 0] = { 10, 235, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2014] + 1] = { 10, 235, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2015] + 3] = { 10, 235, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2039] + 2] = { 10, 235, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2046] + 3] = { 10, 235, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 5] = { 10, 235, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 235, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3446] + 1] = { 10, 235, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3446, 0, 0, 0 } } };
  metadata[offsets[3447] + 0] = { 10, 235, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[3454] + 1] = { 10, 235, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[3455] + 3] = { 10, 235, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3543] + 1] = { 10, 235, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3550] + 4] = { 10, 235, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 4] = { 10, 235, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3575] + 2] = { 10, 235, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3582] + 5] = { 10, 235, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 7] = { 10, 235, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3927] + 2] = { 10, 235, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3934] + 2] = { 10, 235, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3935] + 4] = { 10, 235, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3959] + 2] = { 10, 235, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3966] + 3] = { 10, 235, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 5] = { 10, 235, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4055] + 2] = { 10, 235, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4062] + 5] = { 10, 235, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 7] = { 10, 235, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4086] + 2] = { 10, 235, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 5] = { 10, 235, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4094] + 9] = { 10, 235, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 15] = { 10, 235, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 235, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[2729] + 1] = { 10, 125, true, { { 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 2729, 0, 0, 0 } } };
  metadata[offsets[2735] + 1] = { 10, 125, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[2811] + 2] = { 10, 125, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[2813] + 2] = { 10, 125, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2815] + 5] = { 10, 125, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[3001] + 1] = { 10, 125, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3001, 0, 0, 0 } } };
  metadata[offsets[3003] + 0] = { 10, 125, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[3005] + 0] = { 10, 125, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3007] + 3] = { 10, 125, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3051] + 1] = { 10, 125, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3053] + 3] = { 10, 125, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3055] + 4] = { 10, 125, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3067] + 2] = { 10, 125, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3069] + 3] = { 10, 125, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3071] + 6] = { 10, 125, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 125, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3771] + 3] = { 10, 125, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3773] + 1] = { 10, 125, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3775] + 4] = { 10, 125, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3817] + 1] = { 10, 125, false, { { 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3817, 0, 0, 0 } } };
  metadata[offsets[3819] + 1] = { 10, 125, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[3821] + 1] = { 10, 125, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[3823] + 5] = { 10, 125, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3835] + 4] = { 10, 125, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3837] + 3] = { 10, 125, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3839] + 8] = { 10, 125, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[4011] + 2] = { 10, 125, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4013] + 2] = { 10, 125, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4015] + 5] = { 10, 125, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4027] + 3] = { 10, 125, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4029] + 2] = { 10, 125, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4031] + 6] = { 10, 125, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4075] + 3] = { 10, 125, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4077] + 4] = { 10, 125, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4079] + 8] = { 10, 125, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4089] + 1] = { 10, 125, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4091] + 6] = { 10, 125, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4093] + 6] = { 10, 125, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 16] = { 10, 125, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 125, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[869] + 1] = { 10, 237, true, { { 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1 } }, { { 237, 0, 0, 0 } }, { { 869, 0, 0, 0 } } };
  metadata[offsets[895] + 2] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 237, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[1007] + 2] = { 10, 237, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 237, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[1013] + 0] = { 10, 237, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 237, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1023] + 4] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 237, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1901] + 1] = { 10, 237, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 1901, 0, 0, 0 } } };
  metadata[offsets[1911] + 3] = { 10, 237, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1917] + 0] = { 10, 237, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[1919] + 3] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[2023] + 1] = { 10, 237, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2029] + 1] = { 10, 237, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[2031] + 3] = { 10, 237, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2039] + 3] = { 10, 237, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2045] + 3] = { 10, 237, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2047] + 6] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 237, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2919] + 1] = { 10, 237, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 2919, 0, 0, 0 } } };
  metadata[offsets[2935] + 0] = { 10, 237, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[2941] + 1] = { 10, 237, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[2943] + 2] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[3047] + 1] = { 10, 237, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[3053] + 4] = { 10, 237, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3055] + 5] = { 10, 237, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3063] + 3] = { 10, 237, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3069] + 4] = { 10, 237, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3071] + 7] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 237, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3951] + 2] = { 10, 237, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3957] + 2] = { 10, 237, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3959] + 3] = { 10, 237, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3965] + 2] = { 10, 237, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3967] + 6] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4069] + 2] = { 10, 237, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4071] + 3] = { 10, 237, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4077] + 5] = { 10, 237, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4079] + 9] = { 10, 237, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4085] + 4] = { 10, 237, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4087] + 6] = { 10, 237, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4093] + 7] = { 10, 237, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 17] = { 10, 237, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 237, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[1587] + 1] = { 10, 249, true, { { 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1587, 0, 0, 0 } } };
  metadata[offsets[1659] + 1] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1659, 0, 0, 0 } } };
  metadata[offsets[1719] + 1] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1719, 0, 0, 0 } } };
  metadata[offsets[1791] + 2] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1855] + 2] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1911] + 4] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1915] + 0] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[1919] + 4] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[1975] + 0] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[1979] + 1] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[1983] + 2] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2035] + 2] = { 10, 249, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2039] + 4] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2043] + 2] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2047] + 7] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 249, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3647] + 2] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3703] + 1] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3707] + 1] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[3711] + 3] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3767] + 1] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[3771] + 4] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3775] + 5] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3827] + 2] = { 10, 249, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3831] + 3] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3835] + 5] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3839] + 9] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3891] + 0] = { 10, 249, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[3903] + 4] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3959] + 4] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3963] + 3] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3967] + 7] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4023] + 3] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4027] + 4] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4031] + 7] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4083] + 4] = { 10, 249, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4087] + 7] = { 10, 249, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4091] + 7] = { 10, 249, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4095] + 18] = { 10, 249, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 249, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[1369] + 1] = { 10, 190, true, { { 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1369, 0, 0, 0 } } };
  metadata[offsets[1375] + 1] = { 10, 190, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[1531] + 2] = { 10, 190, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[1533] + 2] = { 10, 190, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1535] + 5] = { 10, 190, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1913] + 1] = { 10, 190, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1913, 0, 0, 0 } } };
  metadata[offsets[1915] + 1] = { 10, 190, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[1917] + 1] = { 10, 190, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[1919] + 5] = { 10, 190, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[2011] + 1] = { 10, 190, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2013] + 3] = { 10, 190, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2015] + 4] = { 10, 190, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2043] + 3] = { 10, 190, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2045] + 4] = { 10, 190, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2047] + 8] = { 10, 190, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 190, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3451] + 3] = { 10, 190, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3453] + 1] = { 10, 190, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3455] + 4] = { 10, 190, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3545] + 1] = { 10, 190, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3545, 0, 0, 0 } } };
  metadata[offsets[3547] + 1] = { 10, 190, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[3549] + 1] = { 10, 190, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3551] + 5] = { 10, 190, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3579] + 4] = { 10, 190, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3581] + 3] = { 10, 190, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3583] + 8] = { 10, 190, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3931] + 2] = { 10, 190, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3933] + 2] = { 10, 190, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3935] + 5] = { 10, 190, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3963] + 4] = { 10, 190, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3965] + 3] = { 10, 190, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3967] + 8] = { 10, 190, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4059] + 3] = { 10, 190, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4061] + 4] = { 10, 190, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4063] + 8] = { 10, 190, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4089] + 2] = { 10, 190, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4091] + 8] = { 10, 190, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4093] + 8] = { 10, 190, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 19] = { 10, 190, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 190, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[917] + 1] = { 10, 222, true, { { 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1 } }, { { 222, 0, 0, 0 } }, { { 917, 0, 0, 0 } } };
  metadata[offsets[959] + 2] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 222, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[991] + 2] = { 10, 222, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 222, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[1013] + 1] = { 10, 222, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 222, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1023] + 5] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 222, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1943] + 1] = { 10, 222, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 1943, 0, 0, 0 } } };
  metadata[offsets[1975] + 1] = { 10, 222, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[1981] + 1] = { 10, 222, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[1983] + 3] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2007] + 1] = { 10, 222, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2013] + 4] = { 10, 222, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2015] + 5] = { 10, 222, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2039] + 5] = { 10, 222, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2045] + 5] = { 10, 222, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2047] + 9] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 222, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2973] + 1] = { 10, 222, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 2973, 0, 0, 0 } } };
  metadata[offsets[2999] + 3] = { 10, 222, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3005] + 1] = { 10, 222, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3007] + 4] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3031] + 1] = { 10, 222, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3037] + 1] = { 10, 222, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3039] + 3] = { 10, 222, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3063] + 4] = { 10, 222, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3069] + 5] = { 10, 222, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3071] + 8] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 222, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3999] + 2] = { 10, 222, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4021] + 2] = { 10, 222, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4023] + 4] = { 10, 222, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4029] + 3] = { 10, 222, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4031] + 8] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4053] + 2] = { 10, 222, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4055] + 3] = { 10, 222, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4061] + 5] = { 10, 222, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4063] + 9] = { 10, 222, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4085] + 5] = { 10, 222, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4087] + 8] = { 10, 222, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4093] + 9] = { 10, 222, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 20] = { 10, 222, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 222, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[2355] + 1] = { 10, 246, true, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2355, 0, 0, 0 } } };
  metadata[offsets[2423] + 1] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2423, 0, 0, 0 } } };
  metadata[offsets[2491] + 1] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2491, 0, 0, 0 } } };
  metadata[offsets[2559] + 2] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2879] + 2] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2935] + 1] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[2939] + 1] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[2943] + 3] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[2999] + 4] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3003] + 1] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[3007] + 5] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3059] + 2] = { 10, 246, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3063] + 5] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3067] + 3] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3071] + 9] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 246, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3391] + 2] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3447] + 1] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[3451] + 4] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3455] + 5] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3511] + 1] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3515] + 1] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[3519] + 3] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3571] + 2] = { 10, 246, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3575] + 3] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3579] + 5] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3583] + 9] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3891] + 1] = { 10, 246, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[3903] + 5] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3959] + 5] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3963] + 5] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3967] + 9] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4023] + 5] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4027] + 5] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4031] + 9] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4083] + 5] = { 10, 246, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4087] + 9] = { 10, 246, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4091] + 9] = { 10, 246, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4095] + 21] = { 10, 246, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 246, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  // Processing non-manifold case 11 with 4 voxel cases
  metadata[offsets[2406] + 1] = { 11, 231, true, { { -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 2406, 0, 0, 0 } } };
  metadata[offsets[2423] + 2] = { 11, 231, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 2423, 0, 0, 0 } } };
  metadata[offsets[2542] + 2] = { 11, 231, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 2542, 0, 0, 0 } } };
  metadata[offsets[2559] + 3] = { 11, 231, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2919] + 2] = { 11, 231, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 2919, 0, 0, 0 } } };
  metadata[offsets[2943] + 4] = { 11, 231, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[3046] + 2] = { 11, 231, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 3046, 0, 0, 0 } } };
  metadata[offsets[3055] + 6] = { 11, 231, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3063] + 6] = { 11, 231, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3070] + 4] = { 11, 231, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 10] = { 11, 231, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 231, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3438] + 2] = { 11, 231, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3438, 0, 0, 0 } } };
  metadata[offsets[3446] + 2] = { 11, 231, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3446, 0, 0, 0 } } };
  metadata[offsets[3455] + 6] = { 11, 231, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3567] + 4] = { 11, 231, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3575] + 4] = { 11, 231, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3582] + 6] = { 11, 231, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 10] = { 11, 231, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3951] + 3] = { 11, 231, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3959] + 6] = { 11, 231, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3966] + 4] = { 11, 231, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 10] = { 11, 231, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4071] + 4] = { 11, 231, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4078] + 6] = { 11, 231, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 10] = { 11, 231, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4086] + 3] = { 11, 231, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 10] = { 11, 231, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4094] + 10] = { 11, 231, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 22] = { 11, 231, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 231, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[1686] + 1] = { 11, 219, true, { { -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 1686, 0, 0, 0 } } };
  metadata[offsets[1719] + 2] = { 11, 219, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 1719, 0, 0, 0 } } };
  metadata[offsets[1758] + 2] = { 11, 219, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 1758, 0, 0, 0 } } };
  metadata[offsets[1791] + 3] = { 11, 219, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1943] + 2] = { 11, 219, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 1943, 0, 0, 0 } } };
  metadata[offsets[1983] + 4] = { 11, 219, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2006] + 2] = { 11, 219, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 2006, 0, 0, 0 } } };
  metadata[offsets[2015] + 6] = { 11, 219, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2039] + 6] = { 11, 219, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2046] + 4] = { 11, 219, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 10] = { 11, 219, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 219, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3742] + 2] = { 11, 219, false, { { -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3742, 0, 0, 0 } } };
  metadata[offsets[3766] + 2] = { 11, 219, false, { { -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3766, 0, 0, 0 } } };
  metadata[offsets[3775] + 6] = { 11, 219, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3807] + 4] = { 11, 219, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3831] + 4] = { 11, 219, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3838] + 6] = { 11, 219, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 10] = { 11, 219, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3999] + 3] = { 11, 219, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4023] + 6] = { 11, 219, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4030] + 4] = { 11, 219, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 10] = { 11, 219, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4055] + 4] = { 11, 219, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4062] + 6] = { 11, 219, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 10] = { 11, 219, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4086] + 4] = { 11, 219, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 11] = { 11, 219, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4094] + 11] = { 11, 219, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 23] = { 11, 219, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 219, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[1641] + 1] = { 11, 189, true, { { 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 1641, 0, 0, 0 } } };
  metadata[offsets[1659] + 2] = { 11, 189, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 1659, 0, 0, 0 } } };
  metadata[offsets[1773] + 2] = { 11, 189, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 1773, 0, 0, 0 } } };
  metadata[offsets[1791] + 4] = { 11, 189, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1901] + 2] = { 11, 189, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 1901, 0, 0, 0 } } };
  metadata[offsets[1913] + 2] = { 11, 189, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 1913, 0, 0, 0 } } };
  metadata[offsets[1919] + 6] = { 11, 189, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[2031] + 4] = { 11, 189, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2043] + 4] = { 11, 189, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2045] + 6] = { 11, 189, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2047] + 11] = { 11, 189, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 189, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3691] + 2] = { 11, 189, false, { { 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3691, 0, 0, 0 } } };
  metadata[offsets[3711] + 4] = { 11, 189, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3817] + 2] = { 11, 189, false, { { 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3817, 0, 0, 0 } } };
  metadata[offsets[3823] + 6] = { 11, 189, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3835] + 6] = { 11, 189, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3837] + 4] = { 11, 189, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3839] + 11] = { 11, 189, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3951] + 4] = { 11, 189, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3963] + 6] = { 11, 189, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3965] + 4] = { 11, 189, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3967] + 11] = { 11, 189, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4075] + 4] = { 11, 189, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4077] + 6] = { 11, 189, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4079] + 11] = { 11, 189, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4089] + 3] = { 11, 189, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4091] + 10] = { 11, 189, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4093] + 10] = { 11, 189, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 24] = { 11, 189, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 189, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[2457] + 1] = { 11, 126, true, { { 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 2457, 0, 0, 0 } } };
  metadata[offsets[2491] + 2] = { 11, 126, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 2491, 0, 0, 0 } } };
  metadata[offsets[2525] + 2] = { 11, 126, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 2525, 0, 0, 0 } } };
  metadata[offsets[2559] + 4] = { 11, 126, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2973] + 2] = { 11, 126, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 2973, 0, 0, 0 } } };
  metadata[offsets[3001] + 2] = { 11, 126, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 3001, 0, 0, 0 } } };
  metadata[offsets[3007] + 6] = { 11, 126, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3039] + 4] = { 11, 126, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3067] + 4] = { 11, 126, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3069] + 6] = { 11, 126, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3071] + 11] = { 11, 126, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 126, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3483] + 2] = { 11, 126, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3483, 0, 0, 0 } } };
  metadata[offsets[3519] + 4] = { 11, 126, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3545] + 2] = { 11, 126, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3545, 0, 0, 0 } } };
  metadata[offsets[3551] + 6] = { 11, 126, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3579] + 6] = { 11, 126, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3581] + 4] = { 11, 126, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3583] + 11] = { 11, 126, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3999] + 4] = { 11, 126, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4027] + 6] = { 11, 126, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4029] + 4] = { 11, 126, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4031] + 11] = { 11, 126, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4059] + 4] = { 11, 126, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4061] + 6] = { 11, 126, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4063] + 11] = { 11, 126, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4089] + 4] = { 11, 126, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4091] + 11] = { 11, 126, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4093] + 11] = { 11, 126, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 25] = { 11, 126, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 126, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  // Processing non-manifold case 12 with 8 voxel cases
  // Processing non-manifold case 13 with 12 voxel cases
  // Processing non-manifold case 14 with 24 voxel cases
  metadata[offsets[1843] + 0] = { 14, 7, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 7, 0, 0, 0 } }, { { 1843, 0, 0, 0 } } };
  metadata[offsets[2867] + 0] = { 14, 11, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 11, 0, 0, 0 } }, { { 2867, 0, 0, 0 } } };
  metadata[offsets[885] + 0] = { 14, 19, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 19, 0, 0, 0 } }, { { 885, 0, 0, 0 } } };
  metadata[offsets[949] + 0] = { 14, 35, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 35, 0, 0, 0 } }, { { 949, 0, 0, 0 } } };
  metadata[offsets[3379] + 0] = { 14, 13, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 13, 0, 0, 0 } }, { { 3379, 0, 0, 0 } } };
  metadata[offsets[1367] + 0] = { 14, 21, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 21, 0, 0, 0 } }, { { 1367, 0, 0, 0 } } };
  metadata[offsets[1371] + 0] = { 14, 69, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 69, 0, 0, 0 } }, { { 1371, 0, 0, 0 } } };
  metadata[offsets[981] + 0] = { 14, 49, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 49, 0, 0, 0 } }, { { 981, 0, 0, 0 } } };
  metadata[offsets[1373] + 0] = { 14, 81, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 81, 0, 0, 0 } }, { { 1373, 0, 0, 0 } } };
  metadata[offsets[3635] + 0] = { 14, 14, false, { { 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 14, 0, 0, 0 } }, { { 3635, 0, 0, 0 } } };
  metadata[offsets[2727] + 0] = { 14, 42, false, { { 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 42, 0, 0, 0 } }, { { 2727, 0, 0, 0 } } };
  metadata[offsets[2731] + 0] = { 14, 138, false, { { 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 138, 0, 0, 0 } }, { { 2731, 0, 0, 0 } } };
  metadata[offsets[997] + 0] = { 14, 50, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 50, 0, 0, 0 } }, { { 997, 0, 0, 0 } } };
  metadata[offsets[2733] + 0] = { 14, 162, false, { { 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 162, 0, 0, 0 } }, { { 2733, 0, 0, 0 } } };
  metadata[offsets[3194] + 0] = { 14, 76, false, { { -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 76, 0, 0, 0 } }, { { 3194, 0, 0, 0 } } };
  metadata[offsets[3258] + 0] = { 14, 140, false, { { -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 140, 0, 0, 0 } }, { { 3258, 0, 0, 0 } } };
  metadata[offsets[1374] + 0] = { 14, 84, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 84, 0, 0, 0 } }, { { 1374, 0, 0, 0 } } };
  metadata[offsets[3290] + 0] = { 14, 196, false, { { -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 196, 0, 0, 0 } }, { { 3290, 0, 0, 0 } } };
  metadata[offsets[2734] + 0] = { 14, 168, false, { { -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 168, 0, 0, 0 } }, { { 2734, 0, 0, 0 } } };
  metadata[offsets[3306] + 0] = { 14, 200, false, { { -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 200, 0, 0, 0 } }, { { 3306, 0, 0, 0 } } };
  metadata[offsets[1996] + 0] = { 14, 112, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 112, 0, 0, 0 } }, { { 1996, 0, 0, 0 } } };
  metadata[offsets[3020] + 0] = { 14, 176, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 176, 0, 0, 0 } }, { { 3020, 0, 0, 0 } } };
  metadata[offsets[3532] + 0] = { 14, 208, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 208, 0, 0, 0 } }, { { 3532, 0, 0, 0 } } };
  metadata[offsets[3788] + 0] = { 14, 224, false, { { -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 224, 0, 0, 0 } }, { { 3788, 0, 0, 0 } } };
  // Processing non-manifold case 15 with 6 voxel cases
  metadata[offsets[3891] + 2] = { 15, 15, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 15, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[1013] + 2] = { 15, 51, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 51, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1375] + 2] = { 15, 85, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 85, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[2735] + 2] = { 15, 170, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 170, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[3322] + 2] = { 15, 204, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 204, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[4044] + 2] = { 15, 240, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 240, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  // Processing non-manifold case 16 with 8 voxel cases
  metadata[offsets[1655] + 0] = { 16, 23, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 23, 0, 0, 0 } }, { { 1655, 0, 0, 0 } } };
  metadata[offsets[1895] + 0] = { 16, 23, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 23, 0, 0, 0 } }, { { 1895, 0, 0, 0 } } };
  metadata[offsets[1910] + 0] = { 16, 23, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 23, 0, 0, 0 } }, { { 1910, 0, 0, 0 } } };
  metadata[offsets[1911] + 5] = { 16, 23, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 23, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[2487] + 0] = { 16, 43, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 43, 0, 0, 0 } }, { { 2487, 0, 0, 0 } } };
  metadata[offsets[2967] + 0] = { 16, 43, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 43, 0, 0, 0 } }, { { 2967, 0, 0, 0 } } };
  metadata[offsets[2998] + 0] = { 16, 43, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 43, 0, 0, 0 } }, { { 2998, 0, 0, 0 } } };
  metadata[offsets[2999] + 5] = { 16, 43, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 43, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[2427] + 0] = { 16, 77, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 77, 0, 0, 0 } }, { { 2427, 0, 0, 0 } } };
  metadata[offsets[3435] + 0] = { 16, 77, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 77, 0, 0, 0 } }, { { 3435, 0, 0, 0 } } };
  metadata[offsets[3449] + 0] = { 16, 77, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 77, 0, 0, 0 } }, { { 3449, 0, 0, 0 } } };
  metadata[offsets[3451] + 5] = { 16, 77, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 77, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[1757] + 0] = { 16, 113, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 113, 0, 0, 0 } }, { { 1757, 0, 0, 0 } } };
  metadata[offsets[1949] + 0] = { 16, 113, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 113, 0, 0, 0 } }, { { 1949, 0, 0, 0 } } };
  metadata[offsets[2009] + 0] = { 16, 113, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 113, 0, 0, 0 } }, { { 2009, 0, 0, 0 } } };
  metadata[offsets[2013] + 5] = { 16, 113, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 113, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[1723] + 0] = { 16, 142, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 142, 0, 0, 0 } }, { { 1723, 0, 0, 0 } } };
  metadata[offsets[3739] + 0] = { 16, 142, false, { { 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 142, 0, 0, 0 } }, { { 3739, 0, 0, 0 } } };
  metadata[offsets[3769] + 0] = { 16, 142, false, { { 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 142, 0, 0, 0 } }, { { 3769, 0, 0, 0 } } };
  metadata[offsets[3771] + 5] = { 16, 142, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 142, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[2541] + 0] = { 16, 178, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 178, 0, 0, 0 } }, { { 2541, 0, 0, 0 } } };
  metadata[offsets[2925] + 0] = { 16, 178, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 178, 0, 0, 0 } }, { { 2925, 0, 0, 0 } } };
  metadata[offsets[3049] + 0] = { 16, 178, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 178, 0, 0, 0 } }, { { 3049, 0, 0, 0 } } };
  metadata[offsets[3053] + 5] = { 16, 178, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 178, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[2526] + 0] = { 16, 212, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 212, 0, 0, 0 } }, { { 2526, 0, 0, 0 } } };
  metadata[offsets[3486] + 0] = { 16, 212, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 212, 0, 0, 0 } }, { { 3486, 0, 0, 0 } } };
  metadata[offsets[3542] + 0] = { 16, 212, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 212, 0, 0, 0 } }, { { 3542, 0, 0, 0 } } };
  metadata[offsets[3550] + 5] = { 16, 212, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 212, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[1774] + 0] = { 16, 232, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 232, 0, 0, 0 } }, { { 1774, 0, 0, 0 } } };
  metadata[offsets[3694] + 0] = { 16, 232, false, { { -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 232, 0, 0, 0 } }, { { 3694, 0, 0, 0 } } };
  metadata[offsets[3814] + 0] = { 16, 232, false, { { -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 232, 0, 0, 0 } }, { { 3814, 0, 0, 0 } } };
  metadata[offsets[3822] + 5] = { 16, 232, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 232, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  // Processing non-manifold case 17 with 24 voxel cases
  metadata[offsets[1463] + 0] = { 17, 39, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 39, 0, 0, 0 } }, { { 1463, 0, 0, 0 } } };
  metadata[offsets[1959] + 0] = { 17, 39, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 39, 0, 0, 0 } }, { { 1959, 0, 0, 0 } } };
  metadata[offsets[1974] + 0] = { 17, 39, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 39, 0, 0, 0 } }, { { 1974, 0, 0, 0 } } };
  metadata[offsets[1975] + 2] = { 17, 39, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 39, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[891] + 0] = { 17, 71, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 71, 0, 0, 0 } }, { { 891, 0, 0, 0 } } };
  metadata[offsets[1899] + 0] = { 17, 71, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 71, 0, 0, 0 } }, { { 1899, 0, 0, 0 } } };
  metadata[offsets[1914] + 0] = { 17, 71, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 71, 0, 0, 0 } }, { { 1914, 0, 0, 0 } } };
  metadata[offsets[1915] + 2] = { 17, 71, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 71, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[2679] + 0] = { 17, 27, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 27, 0, 0, 0 } }, { { 2679, 0, 0, 0 } } };
  metadata[offsets[2903] + 0] = { 17, 27, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 27, 0, 0, 0 } }, { { 2903, 0, 0, 0 } } };
  metadata[offsets[2934] + 0] = { 17, 27, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 27, 0, 0, 0 } }, { { 2934, 0, 0, 0 } } };
  metadata[offsets[2935] + 2] = { 17, 27, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 27, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[955] + 0] = { 17, 139, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 139, 0, 0, 0 } }, { { 955, 0, 0, 0 } } };
  metadata[offsets[2971] + 0] = { 17, 139, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 139, 0, 0, 0 } }, { { 2971, 0, 0, 0 } } };
  metadata[offsets[3002] + 0] = { 17, 139, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 139, 0, 0, 0 } }, { { 3002, 0, 0, 0 } } };
  metadata[offsets[3003] + 2] = { 17, 139, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 139, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[1661] + 0] = { 17, 83, false, { { 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 83, 0, 0, 0 } }, { { 1661, 0, 0, 0 } } };
  metadata[offsets[1853] + 0] = { 17, 83, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 83, 0, 0, 0 } }, { { 1853, 0, 0, 0 } } };
  metadata[offsets[1916] + 0] = { 17, 83, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 83, 0, 0, 0 } }, { { 1916, 0, 0, 0 } } };
  metadata[offsets[1917] + 2] = { 17, 83, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 83, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[2493] + 0] = { 17, 163, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 163, 0, 0, 0 } }, { { 2493, 0, 0, 0 } } };
  metadata[offsets[2877] + 0] = { 17, 163, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 163, 0, 0, 0 } }, { { 2877, 0, 0, 0 } } };
  metadata[offsets[3004] + 0] = { 17, 163, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 163, 0, 0, 0 } }, { { 3004, 0, 0, 0 } } };
  metadata[offsets[3005] + 2] = { 17, 163, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 163, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3191] + 0] = { 17, 29, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 29, 0, 0, 0 } }, { { 3191, 0, 0, 0 } } };
  metadata[offsets[3431] + 0] = { 17, 29, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 29, 0, 0, 0 } }, { { 3431, 0, 0, 0 } } };
  metadata[offsets[3445] + 0] = { 17, 29, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 29, 0, 0, 0 } }, { { 3445, 0, 0, 0 } } };
  metadata[offsets[3447] + 2] = { 17, 29, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 29, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[1467] + 0] = { 17, 141, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 141, 0, 0, 0 } }, { { 1467, 0, 0, 0 } } };
  metadata[offsets[3499] + 0] = { 17, 141, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 141, 0, 0, 0 } }, { { 3499, 0, 0, 0 } } };
  metadata[offsets[3513] + 0] = { 17, 141, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 141, 0, 0, 0 } }, { { 3513, 0, 0, 0 } } };
  metadata[offsets[3515] + 2] = { 17, 141, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 141, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[1751] + 0] = { 17, 53, false, { { 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 53, 0, 0, 0 } }, { { 1751, 0, 0, 0 } } };
  metadata[offsets[1991] + 0] = { 17, 53, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 53, 0, 0, 0 } }, { { 1991, 0, 0, 0 } } };
  metadata[offsets[2003] + 0] = { 17, 53, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 53, 0, 0, 0 } }, { { 2003, 0, 0, 0 } } };
  metadata[offsets[2007] + 2] = { 17, 53, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 53, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2523] + 0] = { 17, 197, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 197, 0, 0, 0 } }, { { 2523, 0, 0, 0 } } };
  metadata[offsets[3531] + 0] = { 17, 197, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 197, 0, 0, 0 } }, { { 3531, 0, 0, 0 } } };
  metadata[offsets[3539] + 0] = { 17, 197, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 197, 0, 0, 0 } }, { { 3539, 0, 0, 0 } } };
  metadata[offsets[3547] + 2] = { 17, 197, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 197, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[2781] + 0] = { 17, 177, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 177, 0, 0, 0 } }, { { 2781, 0, 0, 0 } } };
  metadata[offsets[2909] + 0] = { 17, 177, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 177, 0, 0, 0 } }, { { 2909, 0, 0, 0 } } };
  metadata[offsets[3033] + 0] = { 17, 177, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 177, 0, 0, 0 } }, { { 3033, 0, 0, 0 } } };
  metadata[offsets[3037] + 2] = { 17, 177, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 177, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3293] + 0] = { 17, 209, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 209, 0, 0, 0 } }, { { 3293, 0, 0, 0 } } };
  metadata[offsets[3485] + 0] = { 17, 209, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 209, 0, 0, 0 } }, { { 3485, 0, 0, 0 } } };
  metadata[offsets[3541] + 0] = { 17, 209, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 209, 0, 0, 0 } }, { { 3541, 0, 0, 0 } } };
  metadata[offsets[3549] + 2] = { 17, 209, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 209, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3255] + 0] = { 17, 46, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 46, 0, 0, 0 } }, { { 3255, 0, 0, 0 } } };
  metadata[offsets[3735] + 0] = { 17, 46, false, { { 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 46, 0, 0, 0 } }, { { 3735, 0, 0, 0 } } };
  metadata[offsets[3765] + 0] = { 17, 46, false, { { 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 46, 0, 0, 0 } }, { { 3765, 0, 0, 0 } } };
  metadata[offsets[3767] + 2] = { 17, 46, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 46, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[2683] + 0] = { 17, 78, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 78, 0, 0, 0 } }, { { 2683, 0, 0, 0 } } };
  metadata[offsets[3675] + 0] = { 17, 78, false, { { 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 78, 0, 0, 0 } }, { { 3675, 0, 0, 0 } } };
  metadata[offsets[3705] + 0] = { 17, 78, false, { { 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 78, 0, 0, 0 } }, { { 3705, 0, 0, 0 } } };
  metadata[offsets[3707] + 2] = { 17, 78, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 78, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[2535] + 0] = { 17, 58, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 58, 0, 0, 0 } }, { { 2535, 0, 0, 0 } } };
  metadata[offsets[3015] + 0] = { 17, 58, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 58, 0, 0, 0 } }, { { 3015, 0, 0, 0 } } };
  metadata[offsets[3043] + 0] = { 17, 58, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 58, 0, 0, 0 } }, { { 3043, 0, 0, 0 } } };
  metadata[offsets[3047] + 2] = { 17, 58, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 58, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[1771] + 0] = { 17, 202, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 202, 0, 0, 0 } }, { { 1771, 0, 0, 0 } } };
  metadata[offsets[3787] + 0] = { 17, 202, false, { { 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 202, 0, 0, 0 } }, { { 3787, 0, 0, 0 } } };
  metadata[offsets[3811] + 0] = { 17, 202, false, { { 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 202, 0, 0, 0 } }, { { 3811, 0, 0, 0 } } };
  metadata[offsets[3819] + 2] = { 17, 202, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 202, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[1517] + 0] = { 17, 114, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 114, 0, 0, 0 } }, { { 1517, 0, 0, 0 } } };
  metadata[offsets[1965] + 0] = { 17, 114, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 114, 0, 0, 0 } }, { { 1965, 0, 0, 0 } } };
  metadata[offsets[2025] + 0] = { 17, 114, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 114, 0, 0, 0 } }, { { 2025, 0, 0, 0 } } };
  metadata[offsets[2029] + 2] = { 17, 114, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 114, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[3309] + 0] = { 17, 226, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 226, 0, 0, 0 } }, { { 3309, 0, 0, 0 } } };
  metadata[offsets[3693] + 0] = { 17, 226, false, { { 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 226, 0, 0, 0 } }, { { 3693, 0, 0, 0 } } };
  metadata[offsets[3813] + 0] = { 17, 226, false, { { 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 226, 0, 0, 0 } }, { { 3813, 0, 0, 0 } } };
  metadata[offsets[3821] + 2] = { 17, 226, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 226, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[2430] + 0] = { 17, 92, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 92, 0, 0, 0 } }, { { 2430, 0, 0, 0 } } };
  metadata[offsets[3390] + 0] = { 17, 92, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 92, 0, 0, 0 } }, { { 3390, 0, 0, 0 } } };
  metadata[offsets[3452] + 0] = { 17, 92, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 92, 0, 0, 0 } }, { { 3452, 0, 0, 0 } } };
  metadata[offsets[3454] + 2] = { 17, 92, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 92, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[1726] + 0] = { 17, 172, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 172, 0, 0, 0 } }, { { 1726, 0, 0, 0 } } };
  metadata[offsets[3646] + 0] = { 17, 172, false, { { -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 172, 0, 0, 0 } }, { { 3646, 0, 0, 0 } } };
  metadata[offsets[3772] + 0] = { 17, 172, false, { { -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 172, 0, 0, 0 } }, { { 3772, 0, 0, 0 } } };
  metadata[offsets[3774] + 2] = { 17, 172, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 172, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[990] + 0] = { 17, 116, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 116, 0, 0, 0 } }, { { 990, 0, 0, 0 } } };
  metadata[offsets[1950] + 0] = { 17, 116, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 116, 0, 0, 0 } }, { { 1950, 0, 0, 0 } } };
  metadata[offsets[2010] + 0] = { 17, 116, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 116, 0, 0, 0 } }, { { 2010, 0, 0, 0 } } };
  metadata[offsets[2014] + 2] = { 17, 116, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 116, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2782] + 0] = { 17, 228, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 228, 0, 0, 0 } }, { { 2782, 0, 0, 0 } } };
  metadata[offsets[3678] + 0] = { 17, 228, false, { { -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 228, 0, 0, 0 } }, { { 3678, 0, 0, 0 } } };
  metadata[offsets[3798] + 0] = { 17, 228, false, { { -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 228, 0, 0, 0 } }, { { 3798, 0, 0, 0 } } };
  metadata[offsets[3806] + 2] = { 17, 228, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 228, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[1006] + 0] = { 17, 184, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 184, 0, 0, 0 } }, { { 1006, 0, 0, 0 } } };
  metadata[offsets[2926] + 0] = { 17, 184, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 184, 0, 0, 0 } }, { { 2926, 0, 0, 0 } } };
  metadata[offsets[3050] + 0] = { 17, 184, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 184, 0, 0, 0 } }, { { 3050, 0, 0, 0 } } };
  metadata[offsets[3054] + 2] = { 17, 184, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 184, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[1518] + 0] = { 17, 216, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 216, 0, 0, 0 } }, { { 1518, 0, 0, 0 } } };
  metadata[offsets[3502] + 0] = { 17, 216, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 216, 0, 0, 0 } }, { { 3502, 0, 0, 0 } } };
  metadata[offsets[3558] + 0] = { 17, 216, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 216, 0, 0, 0 } }, { { 3558, 0, 0, 0 } } };
  metadata[offsets[3566] + 2] = { 17, 216, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 216, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  // Processing non-manifold case 18 with 24 voxel cases
  metadata[offsets[3703] + 2] = { 18, 31, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3911] + 0] = { 18, 31, false, { { 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3911, 0, 0, 0 } } };
  metadata[offsets[3925] + 2] = { 18, 31, false, { { 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3925, 0, 0, 0 } } };
  metadata[offsets[3926] + 0] = { 18, 31, false, { { -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3926, 0, 0, 0 } } };
  metadata[offsets[3927] + 3] = { 18, 31, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3941] + 0] = { 18, 31, false, { { 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3941, 0, 0, 0 } } };
  metadata[offsets[3942] + 0] = { 18, 31, false, { { -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3942, 0, 0, 0 } } };
  metadata[offsets[3943] + 0] = { 18, 31, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3943, 0, 0, 0 } } };
  metadata[offsets[3956] + 0] = { 18, 31, false, { { -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3956, 0, 0, 0 } } };
  metadata[offsets[3957] + 3] = { 18, 31, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3958] + 0] = { 18, 31, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3958, 0, 0, 0 } } };
  metadata[offsets[3959] + 7] = { 18, 31, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 31, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3511] + 2] = { 18, 47, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3975] + 0] = { 18, 47, false, { { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 3975, 0, 0, 0 } } };
  metadata[offsets[3989] + 0] = { 18, 47, false, { { 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 3989, 0, 0, 0 } } };
  metadata[offsets[3990] + 0] = { 18, 47, false, { { -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 3990, 0, 0, 0 } } };
  metadata[offsets[3991] + 0] = { 18, 47, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 3991, 0, 0, 0 } } };
  metadata[offsets[4005] + 2] = { 18, 47, false, { { 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 4005, 0, 0, 0 } } };
  metadata[offsets[4006] + 0] = { 18, 47, false, { { -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 4006, 0, 0, 0 } } };
  metadata[offsets[4007] + 3] = { 18, 47, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4020] + 0] = { 18, 47, false, { { -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 4020, 0, 0, 0 } } };
  metadata[offsets[4021] + 3] = { 18, 47, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4022] + 0] = { 18, 47, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 4022, 0, 0, 0 } } };
  metadata[offsets[4023] + 7] = { 18, 47, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 47, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[2939] + 2] = { 18, 79, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 79, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[3915] + 0] = { 18, 79, false, { { 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3915, 0, 0, 0 } } };
  metadata[offsets[3929] + 0] = { 18, 79, false, { { 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3929, 0, 0, 0 } } };
  metadata[offsets[3930] + 2] = { 18, 79, false, { { -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3930, 0, 0, 0 } } };
  metadata[offsets[3931] + 3] = { 18, 79, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3945] + 0] = { 18, 79, false, { { 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3945, 0, 0, 0 } } };
  metadata[offsets[3946] + 0] = { 18, 79, false, { { -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3946, 0, 0, 0 } } };
  metadata[offsets[3947] + 0] = { 18, 79, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3947, 0, 0, 0 } } };
  metadata[offsets[3960] + 0] = { 18, 79, false, { { -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3960, 0, 0, 0 } } };
  metadata[offsets[3961] + 0] = { 18, 79, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3961, 0, 0, 0 } } };
  metadata[offsets[3962] + 3] = { 18, 79, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3963] + 7] = { 18, 79, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 79, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[1979] + 2] = { 18, 143, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 143, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[3979] + 0] = { 18, 143, false, { { 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 3979, 0, 0, 0 } } };
  metadata[offsets[3993] + 0] = { 18, 143, false, { { 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 3993, 0, 0, 0 } } };
  metadata[offsets[3994] + 0] = { 18, 143, false, { { -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 3994, 0, 0, 0 } } };
  metadata[offsets[3995] + 0] = { 18, 143, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 3995, 0, 0, 0 } } };
  metadata[offsets[4009] + 0] = { 18, 143, false, { { 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 4009, 0, 0, 0 } } };
  metadata[offsets[4010] + 2] = { 18, 143, false, { { -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 4010, 0, 0, 0 } } };
  metadata[offsets[4011] + 3] = { 18, 143, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4024] + 0] = { 18, 143, false, { { -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 4024, 0, 0, 0 } } };
  metadata[offsets[4025] + 0] = { 18, 143, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 4025, 0, 0, 0 } } };
  metadata[offsets[4026] + 3] = { 18, 143, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4027] + 7] = { 18, 143, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 143, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[1271] + 0] = { 18, 55, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 1271, 0, 0, 0 } } };
  metadata[offsets[1523] + 2] = { 18, 55, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 1523, 0, 0, 0 } } };
  metadata[offsets[1526] + 0] = { 18, 55, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 1526, 0, 0, 0 } } };
  metadata[offsets[1527] + 3] = { 18, 55, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1779] + 0] = { 18, 55, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 1779, 0, 0, 0 } } };
  metadata[offsets[1782] + 0] = { 18, 55, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 1782, 0, 0, 0 } } };
  metadata[offsets[1783] + 0] = { 18, 55, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 1783, 0, 0, 0 } } };
  metadata[offsets[2023] + 2] = { 18, 55, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2034] + 0] = { 18, 55, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 2034, 0, 0, 0 } } };
  metadata[offsets[2035] + 3] = { 18, 55, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2038] + 0] = { 18, 55, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 2038, 0, 0, 0 } } };
  metadata[offsets[2039] + 7] = { 18, 55, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 55, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[639] + 0] = { 18, 87, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1 } }, { { 87, 0, 0, 0 } }, { { 639, 0, 0, 0 } } };
  metadata[offsets[831] + 2] = { 18, 87, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1 } }, { { 87, 0, 0, 0 } }, { { 831, 0, 0, 0 } } };
  metadata[offsets[879] + 0] = { 18, 87, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1 } }, { { 87, 0, 0, 0 } }, { { 879, 0, 0, 0 } } };
  metadata[offsets[895] + 3] = { 18, 87, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 87, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[1599] + 0] = { 18, 87, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1599, 0, 0, 0 } } };
  metadata[offsets[1647] + 0] = { 18, 87, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1647, 0, 0, 0 } } };
  metadata[offsets[1663] + 0] = { 18, 87, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1663, 0, 0, 0 } } };
  metadata[offsets[1839] + 0] = { 18, 87, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1839, 0, 0, 0 } } };
  metadata[offsets[1855] + 3] = { 18, 87, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1903] + 0] = { 18, 87, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1903, 0, 0, 0 } } };
  metadata[offsets[1918] + 2] = { 18, 87, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[1919] + 7] = { 18, 87, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 87, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[2295] + 0] = { 18, 59, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 2295, 0, 0, 0 } } };
  metadata[offsets[2547] + 0] = { 18, 59, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 2547, 0, 0, 0 } } };
  metadata[offsets[2550] + 0] = { 18, 59, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 2550, 0, 0, 0 } } };
  metadata[offsets[2551] + 0] = { 18, 59, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 2551, 0, 0, 0 } } };
  metadata[offsets[2803] + 2] = { 18, 59, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 2803, 0, 0, 0 } } };
  metadata[offsets[2806] + 0] = { 18, 59, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 2806, 0, 0, 0 } } };
  metadata[offsets[2807] + 3] = { 18, 59, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[3031] + 2] = { 18, 59, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3058] + 0] = { 18, 59, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 3058, 0, 0, 0 } } };
  metadata[offsets[3059] + 3] = { 18, 59, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3062] + 0] = { 18, 59, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 3062, 0, 0, 0 } } };
  metadata[offsets[3063] + 7] = { 18, 59, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 59, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[447] + 0] = { 18, 171, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1 } }, { { 171, 0, 0, 0 } }, { { 447, 0, 0, 0 } } };
  metadata[offsets[831] + 3] = { 18, 171, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1 } }, { { 171, 0, 0, 0 } }, { { 831, 0, 0, 0 } } };
  metadata[offsets[927] + 0] = { 18, 171, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1 } }, { { 171, 0, 0, 0 } }, { { 927, 0, 0, 0 } } };
  metadata[offsets[959] + 3] = { 18, 171, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 171, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[2367] + 0] = { 18, 171, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 2367, 0, 0, 0 } } };
  metadata[offsets[2463] + 0] = { 18, 171, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 2463, 0, 0, 0 } } };
  metadata[offsets[2495] + 0] = { 18, 171, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 2495, 0, 0, 0 } } };
  metadata[offsets[2847] + 0] = { 18, 171, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 2847, 0, 0, 0 } } };
  metadata[offsets[2879] + 3] = { 18, 171, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2975] + 0] = { 18, 171, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 2975, 0, 0, 0 } } };
  metadata[offsets[3006] + 2] = { 18, 171, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3007] + 7] = { 18, 171, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 171, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[1277] + 0] = { 18, 115, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1277, 0, 0, 0 } } };
  metadata[offsets[1529] + 0] = { 18, 115, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1529, 0, 0, 0 } } };
  metadata[offsets[1532] + 2] = { 18, 115, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1532, 0, 0, 0 } } };
  metadata[offsets[1533] + 3] = { 18, 115, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1785] + 0] = { 18, 115, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1785, 0, 0, 0 } } };
  metadata[offsets[1788] + 0] = { 18, 115, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1788, 0, 0, 0 } } };
  metadata[offsets[1789] + 0] = { 18, 115, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1789, 0, 0, 0 } } };
  metadata[offsets[1981] + 2] = { 18, 115, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[2040] + 0] = { 18, 115, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 2040, 0, 0, 0 } } };
  metadata[offsets[2041] + 0] = { 18, 115, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 2041, 0, 0, 0 } } };
  metadata[offsets[2044] + 3] = { 18, 115, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2045] + 7] = { 18, 115, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 115, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2301] + 0] = { 18, 179, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2301, 0, 0, 0 } } };
  metadata[offsets[2553] + 0] = { 18, 179, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2553, 0, 0, 0 } } };
  metadata[offsets[2556] + 0] = { 18, 179, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2556, 0, 0, 0 } } };
  metadata[offsets[2557] + 0] = { 18, 179, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2557, 0, 0, 0 } } };
  metadata[offsets[2809] + 0] = { 18, 179, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2809, 0, 0, 0 } } };
  metadata[offsets[2812] + 2] = { 18, 179, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2812, 0, 0, 0 } } };
  metadata[offsets[2813] + 3] = { 18, 179, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2941] + 2] = { 18, 179, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[3064] + 0] = { 18, 179, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 3064, 0, 0, 0 } } };
  metadata[offsets[3065] + 0] = { 18, 179, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 3065, 0, 0, 0 } } };
  metadata[offsets[3068] + 3] = { 18, 179, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3069] + 7] = { 18, 179, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 179, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[2175] + 0] = { 18, 93, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0 } }, { { 93, 0, 0, 0 } }, { { 2175, 0, 0, 0 } } };
  metadata[offsets[2367] + 1] = { 18, 93, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0 } }, { { 93, 0, 0, 0 } }, { { 2367, 0, 0, 0 } } };
  metadata[offsets[2415] + 0] = { 18, 93, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0 } }, { { 93, 0, 0, 0 } }, { { 2415, 0, 0, 0 } } };
  metadata[offsets[2431] + 0] = { 18, 93, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 93, 0, 0, 0 } }, { { 2431, 0, 0, 0 } } };
  metadata[offsets[3135] + 2] = { 18, 93, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3135, 0, 0, 0 } } };
  metadata[offsets[3183] + 0] = { 18, 93, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3183, 0, 0, 0 } } };
  metadata[offsets[3199] + 3] = { 18, 93, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3375] + 0] = { 18, 93, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3375, 0, 0, 0 } } };
  metadata[offsets[3391] + 3] = { 18, 93, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3439] + 0] = { 18, 93, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3439, 0, 0, 0 } } };
  metadata[offsets[3453] + 2] = { 18, 93, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3455] + 7] = { 18, 93, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 93, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[507] + 0] = { 18, 205, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 205, 0, 0, 0 } }, { { 507, 0, 0, 0 } } };
  metadata[offsets[1523] + 3] = { 18, 205, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 205, 0, 0, 0 } }, { { 1523, 0, 0, 0 } } };
  metadata[offsets[1529] + 1] = { 18, 205, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 205, 0, 0, 0 } }, { { 1529, 0, 0, 0 } } };
  metadata[offsets[1531] + 3] = { 18, 205, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 205, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[2547] + 1] = { 18, 205, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 205, 0, 0, 0 } }, { { 2547, 0, 0, 0 } } };
  metadata[offsets[2553] + 1] = { 18, 205, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 205, 0, 0, 0 } }, { { 2553, 0, 0, 0 } } };
  metadata[offsets[2555] + 0] = { 18, 205, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 205, 0, 0, 0 } }, { { 2555, 0, 0, 0 } } };
  metadata[offsets[3563] + 2] = { 18, 205, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 205, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3569] + 0] = { 18, 205, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 205, 0, 0, 0 } }, { { 3569, 0, 0, 0 } } };
  metadata[offsets[3571] + 3] = { 18, 205, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 205, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3577] + 0] = { 18, 205, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 205, 0, 0, 0 } }, { { 3577, 0, 0, 0 } } };
  metadata[offsets[3579] + 7] = { 18, 205, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 205, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[735] + 0] = { 18, 117, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1 } }, { { 117, 0, 0, 0 } }, { { 735, 0, 0, 0 } } };
  metadata[offsets[927] + 1] = { 18, 117, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1 } }, { { 117, 0, 0, 0 } }, { { 927, 0, 0, 0 } } };
  metadata[offsets[975] + 2] = { 18, 117, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1 } }, { { 117, 0, 0, 0 } }, { { 975, 0, 0, 0 } } };
  metadata[offsets[991] + 3] = { 18, 117, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 117, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[1695] + 0] = { 18, 117, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 1695, 0, 0, 0 } } };
  metadata[offsets[1743] + 0] = { 18, 117, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 1743, 0, 0, 0 } } };
  metadata[offsets[1759] + 0] = { 18, 117, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 1759, 0, 0, 0 } } };
  metadata[offsets[1935] + 0] = { 18, 117, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 1935, 0, 0, 0 } } };
  metadata[offsets[1951] + 0] = { 18, 117, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 1951, 0, 0, 0 } } };
  metadata[offsets[1999] + 3] = { 18, 117, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2011] + 2] = { 18, 117, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2015] + 7] = { 18, 117, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 117, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2271] + 0] = { 18, 213, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0 } }, { { 213, 0, 0, 0 } }, { { 2271, 0, 0, 0 } } };
  metadata[offsets[2463] + 1] = { 18, 213, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0 } }, { { 213, 0, 0, 0 } }, { { 2463, 0, 0, 0 } } };
  metadata[offsets[2511] + 0] = { 18, 213, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0 } }, { { 213, 0, 0, 0 } }, { { 2511, 0, 0, 0 } } };
  metadata[offsets[2527] + 0] = { 18, 213, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 213, 0, 0, 0 } }, { { 2527, 0, 0, 0 } } };
  metadata[offsets[3231] + 0] = { 18, 213, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3231, 0, 0, 0 } } };
  metadata[offsets[3279] + 2] = { 18, 213, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3279, 0, 0, 0 } } };
  metadata[offsets[3295] + 3] = { 18, 213, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3471] + 0] = { 18, 213, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3471, 0, 0, 0 } } };
  metadata[offsets[3487] + 0] = { 18, 213, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3487, 0, 0, 0 } } };
  metadata[offsets[3535] + 3] = { 18, 213, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3543] + 2] = { 18, 213, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3551] + 7] = { 18, 213, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 213, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3805] + 2] = { 18, 241, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3869] + 0] = { 18, 241, false, { { 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3869, 0, 0, 0 } } };
  metadata[offsets[3925] + 3] = { 18, 241, false, { { 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3925, 0, 0, 0 } } };
  metadata[offsets[3929] + 1] = { 18, 241, false, { { 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3929, 0, 0, 0 } } };
  metadata[offsets[3933] + 3] = { 18, 241, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3989] + 1] = { 18, 241, false, { { 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3989, 0, 0, 0 } } };
  metadata[offsets[3993] + 1] = { 18, 241, false, { { 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3993, 0, 0, 0 } } };
  metadata[offsets[3997] + 0] = { 18, 241, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 3997, 0, 0, 0 } } };
  metadata[offsets[4049] + 0] = { 18, 241, false, { { 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 4049, 0, 0, 0 } } };
  metadata[offsets[4053] + 3] = { 18, 241, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4057] + 0] = { 18, 241, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 4057, 0, 0, 0 } } };
  metadata[offsets[4061] + 7] = { 18, 241, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 241, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[1215] + 0] = { 18, 174, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1 } }, { { 174, 0, 0, 0 } }, { { 1215, 0, 0, 0 } } };
  metadata[offsets[1599] + 1] = { 18, 174, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1 } }, { { 174, 0, 0, 0 } }, { { 1599, 0, 0, 0 } } };
  metadata[offsets[1695] + 1] = { 18, 174, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1 } }, { { 174, 0, 0, 0 } }, { { 1695, 0, 0, 0 } } };
  metadata[offsets[1727] + 0] = { 18, 174, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 174, 0, 0, 0 } }, { { 1727, 0, 0, 0 } } };
  metadata[offsets[3135] + 3] = { 18, 174, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3135, 0, 0, 0 } } };
  metadata[offsets[3231] + 1] = { 18, 174, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3231, 0, 0, 0 } } };
  metadata[offsets[3263] + 3] = { 18, 174, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3615] + 0] = { 18, 174, false, { { 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3615, 0, 0, 0 } } };
  metadata[offsets[3647] + 3] = { 18, 174, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3743] + 0] = { 18, 174, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3743, 0, 0, 0 } } };
  metadata[offsets[3773] + 2] = { 18, 174, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3775] + 7] = { 18, 174, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 174, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[763] + 0] = { 18, 206, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 206, 0, 0, 0 } }, { { 763, 0, 0, 0 } } };
  metadata[offsets[1779] + 1] = { 18, 206, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 206, 0, 0, 0 } }, { { 1779, 0, 0, 0 } } };
  metadata[offsets[1785] + 1] = { 18, 206, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 206, 0, 0, 0 } }, { { 1785, 0, 0, 0 } } };
  metadata[offsets[1787] + 0] = { 18, 206, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 206, 0, 0, 0 } }, { { 1787, 0, 0, 0 } } };
  metadata[offsets[2803] + 3] = { 18, 206, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 206, 0, 0, 0 } }, { { 2803, 0, 0, 0 } } };
  metadata[offsets[2809] + 1] = { 18, 206, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 206, 0, 0, 0 } }, { { 2809, 0, 0, 0 } } };
  metadata[offsets[2811] + 3] = { 18, 206, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 206, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[3803] + 2] = { 18, 206, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 206, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3825] + 0] = { 18, 206, false, { { 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 206, 0, 0, 0 } }, { { 3825, 0, 0, 0 } } };
  metadata[offsets[3827] + 3] = { 18, 206, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 206, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3833] + 0] = { 18, 206, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 206, 0, 0, 0 } }, { { 3833, 0, 0, 0 } } };
  metadata[offsets[3835] + 7] = { 18, 206, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 206, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[495] + 0] = { 18, 186, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1 } }, { { 186, 0, 0, 0 } }, { { 495, 0, 0, 0 } } };
  metadata[offsets[879] + 1] = { 18, 186, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1 } }, { { 186, 0, 0, 0 } }, { { 879, 0, 0, 0 } } };
  metadata[offsets[975] + 3] = { 18, 186, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1 } }, { { 186, 0, 0, 0 } }, { { 975, 0, 0, 0 } } };
  metadata[offsets[1007] + 3] = { 18, 186, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 186, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[2415] + 1] = { 18, 186, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 2415, 0, 0, 0 } } };
  metadata[offsets[2511] + 1] = { 18, 186, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 2511, 0, 0, 0 } } };
  metadata[offsets[2543] + 0] = { 18, 186, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 2543, 0, 0, 0 } } };
  metadata[offsets[2895] + 0] = { 18, 186, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 2895, 0, 0, 0 } } };
  metadata[offsets[2927] + 0] = { 18, 186, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 2927, 0, 0, 0 } } };
  metadata[offsets[3023] + 3] = { 18, 186, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3051] + 2] = { 18, 186, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3055] + 7] = { 18, 186, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 186, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[1263] + 0] = { 18, 234, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1 } }, { { 234, 0, 0, 0 } }, { { 1263, 0, 0, 0 } } };
  metadata[offsets[1647] + 1] = { 18, 234, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1 } }, { { 234, 0, 0, 0 } }, { { 1647, 0, 0, 0 } } };
  metadata[offsets[1743] + 1] = { 18, 234, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1 } }, { { 234, 0, 0, 0 } }, { { 1743, 0, 0, 0 } } };
  metadata[offsets[1775] + 0] = { 18, 234, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 234, 0, 0, 0 } }, { { 1775, 0, 0, 0 } } };
  metadata[offsets[3183] + 1] = { 18, 234, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3183, 0, 0, 0 } } };
  metadata[offsets[3279] + 3] = { 18, 234, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3279, 0, 0, 0 } } };
  metadata[offsets[3311] + 3] = { 18, 234, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3663] + 0] = { 18, 234, false, { { 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3663, 0, 0, 0 } } };
  metadata[offsets[3695] + 0] = { 18, 234, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3695, 0, 0, 0 } } };
  metadata[offsets[3791] + 3] = { 18, 234, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3815] + 2] = { 18, 234, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3823] + 7] = { 18, 234, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 234, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3565] + 2] = { 18, 242, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3885] + 0] = { 18, 242, false, { { 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 3885, 0, 0, 0 } } };
  metadata[offsets[3941] + 1] = { 18, 242, false, { { 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 3941, 0, 0, 0 } } };
  metadata[offsets[3945] + 1] = { 18, 242, false, { { 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 3945, 0, 0, 0 } } };
  metadata[offsets[3949] + 0] = { 18, 242, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 3949, 0, 0, 0 } } };
  metadata[offsets[4005] + 3] = { 18, 242, false, { { 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 4005, 0, 0, 0 } } };
  metadata[offsets[4009] + 1] = { 18, 242, false, { { 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 4009, 0, 0, 0 } } };
  metadata[offsets[4013] + 3] = { 18, 242, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4065] + 0] = { 18, 242, false, { { 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 4065, 0, 0, 0 } } };
  metadata[offsets[4069] + 3] = { 18, 242, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4073] + 0] = { 18, 242, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 4073, 0, 0, 0 } } };
  metadata[offsets[4077] + 7] = { 18, 242, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 242, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[510] + 0] = { 18, 220, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 220, 0, 0, 0 } }, { { 510, 0, 0, 0 } } };
  metadata[offsets[1526] + 1] = { 18, 220, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 220, 0, 0, 0 } }, { { 1526, 0, 0, 0 } } };
  metadata[offsets[1532] + 3] = { 18, 220, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 220, 0, 0, 0 } }, { { 1532, 0, 0, 0 } } };
  metadata[offsets[1534] + 3] = { 18, 220, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 220, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[2550] + 1] = { 18, 220, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 220, 0, 0, 0 } }, { { 2550, 0, 0, 0 } } };
  metadata[offsets[2556] + 1] = { 18, 220, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 220, 0, 0, 0 } }, { { 2556, 0, 0, 0 } } };
  metadata[offsets[2558] + 0] = { 18, 220, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 220, 0, 0, 0 } }, { { 2558, 0, 0, 0 } } };
  metadata[offsets[3518] + 2] = { 18, 220, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 220, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3572] + 0] = { 18, 220, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 220, 0, 0, 0 } }, { { 3572, 0, 0, 0 } } };
  metadata[offsets[3574] + 0] = { 18, 220, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 220, 0, 0, 0 } }, { { 3574, 0, 0, 0 } } };
  metadata[offsets[3580] + 3] = { 18, 220, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 220, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3582] + 7] = { 18, 220, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 220, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[766] + 0] = { 18, 236, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 236, 0, 0, 0 } }, { { 766, 0, 0, 0 } } };
  metadata[offsets[1782] + 1] = { 18, 236, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 236, 0, 0, 0 } }, { { 1782, 0, 0, 0 } } };
  metadata[offsets[1788] + 1] = { 18, 236, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 236, 0, 0, 0 } }, { { 1788, 0, 0, 0 } } };
  metadata[offsets[1790] + 0] = { 18, 236, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 236, 0, 0, 0 } }, { { 1790, 0, 0, 0 } } };
  metadata[offsets[2806] + 1] = { 18, 236, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 236, 0, 0, 0 } }, { { 2806, 0, 0, 0 } } };
  metadata[offsets[2812] + 3] = { 18, 236, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 236, 0, 0, 0 } }, { { 2812, 0, 0, 0 } } };
  metadata[offsets[2814] + 3] = { 18, 236, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 236, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[3710] + 2] = { 18, 236, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 236, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3828] + 0] = { 18, 236, false, { { -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 236, 0, 0, 0 } }, { { 3828, 0, 0, 0 } } };
  metadata[offsets[3830] + 0] = { 18, 236, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 236, 0, 0, 0 } }, { { 3830, 0, 0, 0 } } };
  metadata[offsets[3836] + 3] = { 18, 236, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 236, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3838] + 7] = { 18, 236, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 236, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3038] + 2] = { 18, 244, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 244, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3870] + 0] = { 18, 244, false, { { -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 3870, 0, 0, 0 } } };
  metadata[offsets[3926] + 1] = { 18, 244, false, { { -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 3926, 0, 0, 0 } } };
  metadata[offsets[3930] + 3] = { 18, 244, false, { { -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 3930, 0, 0, 0 } } };
  metadata[offsets[3934] + 3] = { 18, 244, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3990] + 1] = { 18, 244, false, { { -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 3990, 0, 0, 0 } } };
  metadata[offsets[3994] + 1] = { 18, 244, false, { { -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 3994, 0, 0, 0 } } };
  metadata[offsets[3998] + 0] = { 18, 244, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 3998, 0, 0, 0 } } };
  metadata[offsets[4050] + 0] = { 18, 244, false, { { -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 4050, 0, 0, 0 } } };
  metadata[offsets[4054] + 0] = { 18, 244, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 4054, 0, 0, 0 } } };
  metadata[offsets[4058] + 3] = { 18, 244, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4062] + 7] = { 18, 244, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 244, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[2030] + 2] = { 18, 248, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 248, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[3886] + 0] = { 18, 248, false, { { -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 3886, 0, 0, 0 } } };
  metadata[offsets[3942] + 1] = { 18, 248, false, { { -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 3942, 0, 0, 0 } } };
  metadata[offsets[3946] + 1] = { 18, 248, false, { { -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 3946, 0, 0, 0 } } };
  metadata[offsets[3950] + 0] = { 18, 248, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 3950, 0, 0, 0 } } };
  metadata[offsets[4006] + 1] = { 18, 248, false, { { -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 4006, 0, 0, 0 } } };
  metadata[offsets[4010] + 3] = { 18, 248, false, { { -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 4010, 0, 0, 0 } } };
  metadata[offsets[4014] + 3] = { 18, 248, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4066] + 0] = { 18, 248, false, { { -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 4066, 0, 0, 0 } } };
  metadata[offsets[4070] + 0] = { 18, 248, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 4070, 0, 0, 0 } } };
  metadata[offsets[4074] + 3] = { 18, 248, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4078] + 7] = { 18, 248, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 248, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  // Processing non-manifold case 19 with 12 voxel cases
  metadata[offsets[3319] + 2] = { 19, 63, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3537] + 2] = { 19, 63, false, { { 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3537, 0, 0, 0 } } };
  metadata[offsets[3542] + 1] = { 19, 63, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3542, 0, 0, 0 } } };
  metadata[offsets[3543] + 3] = { 19, 63, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3558] + 1] = { 19, 63, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3558, 0, 0, 0 } } };
  metadata[offsets[3559] + 0] = { 19, 63, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3559, 0, 0, 0 } } };
  metadata[offsets[3571] + 4] = { 19, 63, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3572] + 1] = { 19, 63, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3572, 0, 0, 0 } } };
  metadata[offsets[3573] + 2] = { 19, 63, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3574] + 1] = { 19, 63, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3574, 0, 0, 0 } } };
  metadata[offsets[3575] + 5] = { 19, 63, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3798] + 1] = { 19, 63, false, { { -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3798, 0, 0, 0 } } };
  metadata[offsets[3799] + 0] = { 19, 63, false, { { 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3799, 0, 0, 0 } } };
  metadata[offsets[3809] + 2] = { 19, 63, false, { { 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3809, 0, 0, 0 } } };
  metadata[offsets[3814] + 1] = { 19, 63, false, { { -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3814, 0, 0, 0 } } };
  metadata[offsets[3815] + 3] = { 19, 63, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3827] + 4] = { 19, 63, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3828] + 1] = { 19, 63, false, { { -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3828, 0, 0, 0 } } };
  metadata[offsets[3829] + 2] = { 19, 63, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3830] + 1] = { 19, 63, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3830, 0, 0, 0 } } };
  metadata[offsets[3831] + 5] = { 19, 63, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[4039] + 2] = { 19, 63, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4050] + 1] = { 19, 63, false, { { -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4050, 0, 0, 0 } } };
  metadata[offsets[4051] + 2] = { 19, 63, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4053] + 4] = { 19, 63, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4054] + 1] = { 19, 63, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4054, 0, 0, 0 } } };
  metadata[offsets[4055] + 5] = { 19, 63, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4066] + 1] = { 19, 63, false, { { -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4066, 0, 0, 0 } } };
  metadata[offsets[4067] + 2] = { 19, 63, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4069] + 4] = { 19, 63, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4070] + 1] = { 19, 63, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4070, 0, 0, 0 } } };
  metadata[offsets[4071] + 5] = { 19, 63, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4080] + 2] = { 19, 63, false, { { -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4080, 0, 0, 0 } } };
  metadata[offsets[4081] + 3] = { 19, 63, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4082] + 3] = { 19, 63, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4083] + 6] = { 19, 63, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4084] + 3] = { 19, 63, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4085] + 6] = { 19, 63, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4086] + 5] = { 19, 63, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 12] = { 19, 63, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 63, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[2687] + 2] = { 19, 95, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2845] + 2] = { 19, 95, false, { { 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2845, 0, 0, 0 } } };
  metadata[offsets[2879] + 4] = { 19, 95, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2895] + 1] = { 19, 95, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2895, 0, 0, 0 } } };
  metadata[offsets[2911] + 2] = { 19, 95, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[2925] + 1] = { 19, 95, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2925, 0, 0, 0 } } };
  metadata[offsets[2926] + 1] = { 19, 95, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2926, 0, 0, 0 } } };
  metadata[offsets[2927] + 1] = { 19, 95, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2927, 0, 0, 0 } } };
  metadata[offsets[2941] + 3] = { 19, 95, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[2942] + 0] = { 19, 95, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2942, 0, 0, 0 } } };
  metadata[offsets[2943] + 5] = { 19, 95, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 95, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[3614] + 2] = { 19, 95, false, { { -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3614, 0, 0, 0 } } };
  metadata[offsets[3647] + 4] = { 19, 95, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3663] + 1] = { 19, 95, false, { { 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3663, 0, 0, 0 } } };
  metadata[offsets[3679] + 2] = { 19, 95, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3693] + 1] = { 19, 95, false, { { 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3693, 0, 0, 0 } } };
  metadata[offsets[3694] + 1] = { 19, 95, false, { { -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3694, 0, 0, 0 } } };
  metadata[offsets[3695] + 1] = { 19, 95, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3695, 0, 0, 0 } } };
  metadata[offsets[3709] + 0] = { 19, 95, false, { { 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3709, 0, 0, 0 } } };
  metadata[offsets[3710] + 3] = { 19, 95, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3711] + 5] = { 19, 95, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3855] + 2] = { 19, 95, false, { { 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3855, 0, 0, 0 } } };
  metadata[offsets[3871] + 3] = { 19, 95, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3885] + 1] = { 19, 95, false, { { 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3885, 0, 0, 0 } } };
  metadata[offsets[3886] + 1] = { 19, 95, false, { { -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3886, 0, 0, 0 } } };
  metadata[offsets[3887] + 3] = { 19, 95, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3901] + 2] = { 19, 95, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3902] + 2] = { 19, 95, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3903] + 6] = { 19, 95, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3919] + 3] = { 19, 95, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3933] + 4] = { 19, 95, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3934] + 4] = { 19, 95, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3935] + 6] = { 19, 95, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3949] + 1] = { 19, 95, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3949, 0, 0, 0 } } };
  metadata[offsets[3950] + 1] = { 19, 95, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3950, 0, 0, 0 } } };
  metadata[offsets[3951] + 5] = { 19, 95, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3964] + 2] = { 19, 95, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3965] + 5] = { 19, 95, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3966] + 5] = { 19, 95, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 12] = { 19, 95, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 95, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[1471] + 2] = { 19, 175, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1837] + 2] = { 19, 175, false, { { 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1837, 0, 0, 0 } } };
  metadata[offsets[1855] + 4] = { 19, 175, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1935] + 1] = { 19, 175, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1935, 0, 0, 0 } } };
  metadata[offsets[1949] + 1] = { 19, 175, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1949, 0, 0, 0 } } };
  metadata[offsets[1950] + 1] = { 19, 175, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1950, 0, 0, 0 } } };
  metadata[offsets[1951] + 1] = { 19, 175, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1951, 0, 0, 0 } } };
  metadata[offsets[1967] + 2] = { 19, 175, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1981] + 3] = { 19, 175, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[1982] + 0] = { 19, 175, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1982, 0, 0, 0 } } };
  metadata[offsets[1983] + 5] = { 19, 175, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 175, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[3374] + 2] = { 19, 175, false, { { -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3374, 0, 0, 0 } } };
  metadata[offsets[3391] + 4] = { 19, 175, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3471] + 1] = { 19, 175, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3471, 0, 0, 0 } } };
  metadata[offsets[3485] + 1] = { 19, 175, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3485, 0, 0, 0 } } };
  metadata[offsets[3486] + 1] = { 19, 175, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3486, 0, 0, 0 } } };
  metadata[offsets[3487] + 1] = { 19, 175, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3487, 0, 0, 0 } } };
  metadata[offsets[3503] + 2] = { 19, 175, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3517] + 0] = { 19, 175, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3517, 0, 0, 0 } } };
  metadata[offsets[3518] + 3] = { 19, 175, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3519] + 5] = { 19, 175, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3855] + 3] = { 19, 175, false, { { 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3855, 0, 0, 0 } } };
  metadata[offsets[3869] + 1] = { 19, 175, false, { { 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3869, 0, 0, 0 } } };
  metadata[offsets[3870] + 1] = { 19, 175, false, { { -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3870, 0, 0, 0 } } };
  metadata[offsets[3871] + 4] = { 19, 175, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3887] + 4] = { 19, 175, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3901] + 3] = { 19, 175, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3902] + 3] = { 19, 175, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3903] + 7] = { 19, 175, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3983] + 3] = { 19, 175, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[3997] + 1] = { 19, 175, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3997, 0, 0, 0 } } };
  metadata[offsets[3998] + 1] = { 19, 175, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3998, 0, 0, 0 } } };
  metadata[offsets[3999] + 5] = { 19, 175, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4013] + 4] = { 19, 175, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4014] + 4] = { 19, 175, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4015] + 6] = { 19, 175, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4028] + 2] = { 19, 175, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4029] + 5] = { 19, 175, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4030] + 5] = { 19, 175, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 12] = { 19, 175, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 175, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[1019] + 2] = { 19, 207, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 207, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[2002] + 2] = { 19, 207, false, { { -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2002, 0, 0, 0 } } };
  metadata[offsets[2009] + 1] = { 19, 207, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2009, 0, 0, 0 } } };
  metadata[offsets[2011] + 3] = { 19, 207, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2025] + 1] = { 19, 207, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2025, 0, 0, 0 } } };
  metadata[offsets[2027] + 0] = { 19, 207, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2027, 0, 0, 0 } } };
  metadata[offsets[2035] + 4] = { 19, 207, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2040] + 1] = { 19, 207, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2040, 0, 0, 0 } } };
  metadata[offsets[2041] + 1] = { 19, 207, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2041, 0, 0, 0 } } };
  metadata[offsets[2042] + 2] = { 19, 207, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2043] + 5] = { 19, 207, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 207, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[3033] + 1] = { 19, 207, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3033, 0, 0, 0 } } };
  metadata[offsets[3035] + 0] = { 19, 207, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3035, 0, 0, 0 } } };
  metadata[offsets[3042] + 2] = { 19, 207, false, { { -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3042, 0, 0, 0 } } };
  metadata[offsets[3049] + 1] = { 19, 207, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3049, 0, 0, 0 } } };
  metadata[offsets[3051] + 3] = { 19, 207, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3059] + 4] = { 19, 207, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3064] + 1] = { 19, 207, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3064, 0, 0, 0 } } };
  metadata[offsets[3065] + 1] = { 19, 207, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3065, 0, 0, 0 } } };
  metadata[offsets[3066] + 2] = { 19, 207, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3067] + 5] = { 19, 207, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 207, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[4043] + 2] = { 19, 207, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4049] + 1] = { 19, 207, false, { { 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4049, 0, 0, 0 } } };
  metadata[offsets[4051] + 3] = { 19, 207, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4057] + 1] = { 19, 207, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4057, 0, 0, 0 } } };
  metadata[offsets[4058] + 4] = { 19, 207, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4059] + 5] = { 19, 207, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4065] + 1] = { 19, 207, false, { { 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4065, 0, 0, 0 } } };
  metadata[offsets[4067] + 3] = { 19, 207, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4073] + 1] = { 19, 207, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4073, 0, 0, 0 } } };
  metadata[offsets[4074] + 4] = { 19, 207, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4075] + 5] = { 19, 207, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4080] + 3] = { 19, 207, false, { { -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4080, 0, 0, 0 } } };
  metadata[offsets[4081] + 4] = { 19, 207, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4082] + 4] = { 19, 207, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4083] + 7] = { 19, 207, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4088] + 3] = { 19, 207, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4089] + 5] = { 19, 207, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4090] + 6] = { 19, 207, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 12] = { 19, 207, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 207, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[255] + 2] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 255, 0, 0, 0 } } };
  metadata[offsets[443] + 2] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 443, 0, 0, 0 } } };
  metadata[offsets[494] + 2] = { 19, 119, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 494, 0, 0, 0 } } };
  metadata[offsets[511] + 3] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[763] + 1] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 763, 0, 0, 0 } } };
  metadata[offsets[766] + 1] = { 19, 119, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 766, 0, 0, 0 } } };
  metadata[offsets[767] + 3] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[959] + 4] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[1007] + 4] = { 19, 119, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[1019] + 3] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[1022] + 2] = { 19, 119, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1023] + 6] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 119, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1215] + 1] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1215, 0, 0, 0 } } };
  metadata[offsets[1263] + 1] = { 19, 119, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1263, 0, 0, 0 } } };
  metadata[offsets[1279] + 3] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1471] + 3] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1519] + 2] = { 19, 119, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1531] + 4] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[1534] + 4] = { 19, 119, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[1535] + 6] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1723] + 1] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1723, 0, 0, 0 } } };
  metadata[offsets[1726] + 1] = { 19, 119, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1726, 0, 0, 0 } } };
  metadata[offsets[1727] + 1] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1727, 0, 0, 0 } } };
  metadata[offsets[1771] + 1] = { 19, 119, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1771, 0, 0, 0 } } };
  metadata[offsets[1774] + 1] = { 19, 119, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1774, 0, 0, 0 } } };
  metadata[offsets[1775] + 1] = { 19, 119, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1775, 0, 0, 0 } } };
  metadata[offsets[1787] + 1] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1787, 0, 0, 0 } } };
  metadata[offsets[1790] + 1] = { 19, 119, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1790, 0, 0, 0 } } };
  metadata[offsets[1791] + 5] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1967] + 3] = { 19, 119, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1979] + 3] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[1982] + 1] = { 19, 119, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1982, 0, 0, 0 } } };
  metadata[offsets[1983] + 6] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2027] + 1] = { 19, 119, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 2027, 0, 0, 0 } } };
  metadata[offsets[2030] + 3] = { 19, 119, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[2031] + 5] = { 19, 119, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2042] + 3] = { 19, 119, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2043] + 6] = { 19, 119, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2046] + 5] = { 19, 119, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 12] = { 19, 119, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 119, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[255] + 3] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 255, 0, 0, 0 } } };
  metadata[offsets[507] + 1] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 507, 0, 0, 0 } } };
  metadata[offsets[510] + 1] = { 19, 187, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 510, 0, 0, 0 } } };
  metadata[offsets[511] + 4] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[635] + 2] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 635, 0, 0, 0 } } };
  metadata[offsets[734] + 2] = { 19, 187, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 734, 0, 0, 0 } } };
  metadata[offsets[767] + 4] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[895] + 4] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[991] + 4] = { 19, 187, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[1019] + 4] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[1022] + 3] = { 19, 187, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1023] + 7] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 187, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[2175] + 1] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2175, 0, 0, 0 } } };
  metadata[offsets[2271] + 1] = { 19, 187, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2271, 0, 0, 0 } } };
  metadata[offsets[2303] + 3] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2427] + 1] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2427, 0, 0, 0 } } };
  metadata[offsets[2430] + 1] = { 19, 187, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2430, 0, 0, 0 } } };
  metadata[offsets[2431] + 1] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2431, 0, 0, 0 } } };
  metadata[offsets[2523] + 1] = { 19, 187, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2523, 0, 0, 0 } } };
  metadata[offsets[2526] + 1] = { 19, 187, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2526, 0, 0, 0 } } };
  metadata[offsets[2527] + 1] = { 19, 187, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2527, 0, 0, 0 } } };
  metadata[offsets[2555] + 1] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2555, 0, 0, 0 } } };
  metadata[offsets[2558] + 1] = { 19, 187, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2558, 0, 0, 0 } } };
  metadata[offsets[2559] + 5] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2687] + 3] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2783] + 2] = { 19, 187, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2811] + 4] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[2814] + 4] = { 19, 187, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[2815] + 6] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2911] + 3] = { 19, 187, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[2939] + 3] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[2942] + 1] = { 19, 187, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2942, 0, 0, 0 } } };
  metadata[offsets[2943] + 6] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[3035] + 1] = { 19, 187, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 3035, 0, 0, 0 } } };
  metadata[offsets[3038] + 3] = { 19, 187, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3039] + 5] = { 19, 187, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3066] + 3] = { 19, 187, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3067] + 6] = { 19, 187, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3070] + 5] = { 19, 187, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 12] = { 19, 187, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 187, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3325] + 2] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3444] + 2] = { 19, 243, false, { { -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3444, 0, 0, 0 } } };
  metadata[offsets[3449] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3449, 0, 0, 0 } } };
  metadata[offsets[3453] + 3] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3513] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3513, 0, 0, 0 } } };
  metadata[offsets[3517] + 1] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3517, 0, 0, 0 } } };
  metadata[offsets[3569] + 1] = { 19, 243, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3569, 0, 0, 0 } } };
  metadata[offsets[3573] + 3] = { 19, 243, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3577] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3577, 0, 0, 0 } } };
  metadata[offsets[3580] + 4] = { 19, 243, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3581] + 5] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3705] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3705, 0, 0, 0 } } };
  metadata[offsets[3709] + 1] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3709, 0, 0, 0 } } };
  metadata[offsets[3764] + 2] = { 19, 243, false, { { -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3764, 0, 0, 0 } } };
  metadata[offsets[3769] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3769, 0, 0, 0 } } };
  metadata[offsets[3773] + 3] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3825] + 1] = { 19, 243, false, { { 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3825, 0, 0, 0 } } };
  metadata[offsets[3829] + 3] = { 19, 243, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3833] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3833, 0, 0, 0 } } };
  metadata[offsets[3836] + 4] = { 19, 243, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3837] + 5] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3901] + 4] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3957] + 4] = { 19, 243, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3960] + 1] = { 19, 243, false, { { -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3960, 0, 0, 0 } } };
  metadata[offsets[3961] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3961, 0, 0, 0 } } };
  metadata[offsets[3964] + 3] = { 19, 243, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3965] + 6] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[4021] + 4] = { 19, 243, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4024] + 1] = { 19, 243, false, { { -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4024, 0, 0, 0 } } };
  metadata[offsets[4025] + 1] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4025, 0, 0, 0 } } };
  metadata[offsets[4028] + 3] = { 19, 243, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4029] + 6] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4080] + 4] = { 19, 243, false, { { -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4080, 0, 0, 0 } } };
  metadata[offsets[4081] + 5] = { 19, 243, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4084] + 4] = { 19, 243, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4085] + 7] = { 19, 243, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4088] + 4] = { 19, 243, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4089] + 6] = { 19, 243, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4092] + 6] = { 19, 243, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 12] = { 19, 243, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 243, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[255] + 4] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1 } }, { { 221, 0, 0, 0 } }, { { 255, 0, 0, 0 } } };
  metadata[offsets[447] + 1] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1 } }, { { 221, 0, 0, 0 } }, { { 447, 0, 0, 0 } } };
  metadata[offsets[495] + 1] = { 19, 221, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1 } }, { { 221, 0, 0, 0 } }, { { 495, 0, 0, 0 } } };
  metadata[offsets[511] + 5] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 221, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[1207] + 2] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1207, 0, 0, 0 } } };
  metadata[offsets[1261] + 2] = { 19, 221, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1261, 0, 0, 0 } } };
  metadata[offsets[1279] + 4] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1471] + 4] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1519] + 3] = { 19, 221, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1527] + 4] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1533] + 4] = { 19, 221, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1535] + 7] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 221, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[2295] + 1] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2295, 0, 0, 0 } } };
  metadata[offsets[2301] + 1] = { 19, 221, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2301, 0, 0, 0 } } };
  metadata[offsets[2303] + 4] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2487] + 1] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2487, 0, 0, 0 } } };
  metadata[offsets[2493] + 1] = { 19, 221, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2493, 0, 0, 0 } } };
  metadata[offsets[2495] + 1] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2495, 0, 0, 0 } } };
  metadata[offsets[2535] + 1] = { 19, 221, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2535, 0, 0, 0 } } };
  metadata[offsets[2541] + 1] = { 19, 221, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2541, 0, 0, 0 } } };
  metadata[offsets[2543] + 1] = { 19, 221, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2543, 0, 0, 0 } } };
  metadata[offsets[2551] + 1] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2551, 0, 0, 0 } } };
  metadata[offsets[2557] + 1] = { 19, 221, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2557, 0, 0, 0 } } };
  metadata[offsets[2559] + 6] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 221, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[3263] + 4] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3311] + 4] = { 19, 221, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3319] + 3] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3325] + 3] = { 19, 221, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3327] + 6] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3503] + 3] = { 19, 221, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3511] + 3] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3517] + 2] = { 19, 221, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3517, 0, 0, 0 } } };
  metadata[offsets[3519] + 6] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3559] + 1] = { 19, 221, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3559, 0, 0, 0 } } };
  metadata[offsets[3565] + 3] = { 19, 221, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3567] + 5] = { 19, 221, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3573] + 4] = { 19, 221, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3575] + 6] = { 19, 221, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3581] + 6] = { 19, 221, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3583] + 12] = { 19, 221, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 221, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[2783] + 3] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2847] + 1] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 2847, 0, 0, 0 } } };
  metadata[offsets[2887] + 2] = { 19, 245, false, { { 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 2887, 0, 0, 0 } } };
  metadata[offsets[2911] + 4] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[2967] + 1] = { 19, 245, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 2967, 0, 0, 0 } } };
  metadata[offsets[2971] + 1] = { 19, 245, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 2971, 0, 0, 0 } } };
  metadata[offsets[2975] + 1] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 2975, 0, 0, 0 } } };
  metadata[offsets[3023] + 4] = { 19, 245, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3031] + 3] = { 19, 245, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3035] + 2] = { 19, 245, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 3035, 0, 0, 0 } } };
  metadata[offsets[3039] + 6] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 245, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3615] + 1] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3615, 0, 0, 0 } } };
  metadata[offsets[3659] + 2] = { 19, 245, false, { { 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3659, 0, 0, 0 } } };
  metadata[offsets[3679] + 3] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3735] + 1] = { 19, 245, false, { { 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3735, 0, 0, 0 } } };
  metadata[offsets[3739] + 1] = { 19, 245, false, { { 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3739, 0, 0, 0 } } };
  metadata[offsets[3743] + 1] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3743, 0, 0, 0 } } };
  metadata[offsets[3791] + 4] = { 19, 245, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3799] + 1] = { 19, 245, false, { { 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3799, 0, 0, 0 } } };
  metadata[offsets[3803] + 3] = { 19, 245, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3807] + 5] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3855] + 4] = { 19, 245, false, { { 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3855, 0, 0, 0 } } };
  metadata[offsets[3871] + 5] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3919] + 4] = { 19, 245, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3927] + 4] = { 19, 245, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3931] + 4] = { 19, 245, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3935] + 7] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3975] + 1] = { 19, 245, false, { { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3975, 0, 0, 0 } } };
  metadata[offsets[3979] + 1] = { 19, 245, false, { { 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3979, 0, 0, 0 } } };
  metadata[offsets[3983] + 4] = { 19, 245, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[3991] + 1] = { 19, 245, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3991, 0, 0, 0 } } };
  metadata[offsets[3995] + 1] = { 19, 245, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3995, 0, 0, 0 } } };
  metadata[offsets[3999] + 6] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4039] + 3] = { 19, 245, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4043] + 3] = { 19, 245, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4047] + 6] = { 19, 245, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4051] + 4] = { 19, 245, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4055] + 6] = { 19, 245, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4059] + 6] = { 19, 245, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4063] + 12] = { 19, 245, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 245, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[255] + 5] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1 } }, { { 238, 0, 0, 0 } }, { { 255, 0, 0, 0 } } };
  metadata[offsets[639] + 1] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1 } }, { { 238, 0, 0, 0 } }, { { 639, 0, 0, 0 } } };
  metadata[offsets[735] + 1] = { 19, 238, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1 } }, { { 238, 0, 0, 0 } }, { { 735, 0, 0, 0 } } };
  metadata[offsets[767] + 5] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 238, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[1271] + 1] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1271, 0, 0, 0 } } };
  metadata[offsets[1277] + 1] = { 19, 238, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1277, 0, 0, 0 } } };
  metadata[offsets[1279] + 5] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1655] + 1] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1655, 0, 0, 0 } } };
  metadata[offsets[1661] + 1] = { 19, 238, false, { { 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1661, 0, 0, 0 } } };
  metadata[offsets[1663] + 1] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1663, 0, 0, 0 } } };
  metadata[offsets[1751] + 1] = { 19, 238, false, { { 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1751, 0, 0, 0 } } };
  metadata[offsets[1757] + 1] = { 19, 238, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1757, 0, 0, 0 } } };
  metadata[offsets[1759] + 1] = { 19, 238, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1759, 0, 0, 0 } } };
  metadata[offsets[1783] + 1] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1783, 0, 0, 0 } } };
  metadata[offsets[1789] + 1] = { 19, 238, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1789, 0, 0, 0 } } };
  metadata[offsets[1791] + 6] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 238, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[2167] + 2] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2167, 0, 0, 0 } } };
  metadata[offsets[2269] + 2] = { 19, 238, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2269, 0, 0, 0 } } };
  metadata[offsets[2303] + 5] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2687] + 4] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2783] + 4] = { 19, 238, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2807] + 4] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[2813] + 4] = { 19, 238, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2815] + 7] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 238, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[3199] + 4] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3295] + 4] = { 19, 238, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3319] + 4] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3325] + 4] = { 19, 238, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3327] + 7] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3679] + 4] = { 19, 238, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3703] + 3] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3709] + 2] = { 19, 238, false, { { 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3709, 0, 0, 0 } } };
  metadata[offsets[3711] + 6] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3799] + 2] = { 19, 238, false, { { 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3799, 0, 0, 0 } } };
  metadata[offsets[3805] + 3] = { 19, 238, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3807] + 6] = { 19, 238, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3829] + 4] = { 19, 238, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3831] + 6] = { 19, 238, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3837] + 6] = { 19, 238, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3839] + 12] = { 19, 238, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 238, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[1519] + 4] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1839] + 1] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1839, 0, 0, 0 } } };
  metadata[offsets[1895] + 1] = { 19, 250, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1895, 0, 0, 0 } } };
  metadata[offsets[1899] + 1] = { 19, 250, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1899, 0, 0, 0 } } };
  metadata[offsets[1903] + 1] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1903, 0, 0, 0 } } };
  metadata[offsets[1927] + 2] = { 19, 250, false, { { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1927, 0, 0, 0 } } };
  metadata[offsets[1967] + 4] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1999] + 4] = { 19, 250, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2023] + 3] = { 19, 250, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2027] + 2] = { 19, 250, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 2027, 0, 0, 0 } } };
  metadata[offsets[2031] + 6] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 250, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[3375] + 1] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3375, 0, 0, 0 } } };
  metadata[offsets[3431] + 1] = { 19, 250, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3431, 0, 0, 0 } } };
  metadata[offsets[3435] + 1] = { 19, 250, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3435, 0, 0, 0 } } };
  metadata[offsets[3439] + 1] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3439, 0, 0, 0 } } };
  metadata[offsets[3467] + 2] = { 19, 250, false, { { 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3467, 0, 0, 0 } } };
  metadata[offsets[3503] + 4] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3535] + 4] = { 19, 250, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3559] + 2] = { 19, 250, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3559, 0, 0, 0 } } };
  metadata[offsets[3563] + 3] = { 19, 250, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3567] + 6] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3855] + 5] = { 19, 250, false, { { 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3855, 0, 0, 0 } } };
  metadata[offsets[3887] + 5] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3911] + 1] = { 19, 250, false, { { 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3911, 0, 0, 0 } } };
  metadata[offsets[3915] + 1] = { 19, 250, false, { { 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3915, 0, 0, 0 } } };
  metadata[offsets[3919] + 5] = { 19, 250, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3943] + 1] = { 19, 250, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3943, 0, 0, 0 } } };
  metadata[offsets[3947] + 1] = { 19, 250, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3947, 0, 0, 0 } } };
  metadata[offsets[3951] + 6] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3983] + 5] = { 19, 250, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[4007] + 4] = { 19, 250, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4011] + 4] = { 19, 250, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4015] + 7] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4039] + 4] = { 19, 250, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4043] + 4] = { 19, 250, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4047] + 7] = { 19, 250, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4067] + 4] = { 19, 250, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4071] + 6] = { 19, 250, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4075] + 6] = { 19, 250, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4079] + 12] = { 19, 250, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 250, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[1022] + 4] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 252, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1910] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 1910, 0, 0, 0 } } };
  metadata[offsets[1912] + 2] = { 19, 252, false, { { -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 1912, 0, 0, 0 } } };
  metadata[offsets[1918] + 3] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[1974] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 1974, 0, 0, 0 } } };
  metadata[offsets[1982] + 2] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 1982, 0, 0, 0 } } };
  metadata[offsets[2034] + 1] = { 19, 252, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 2034, 0, 0, 0 } } };
  metadata[offsets[2038] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 2038, 0, 0, 0 } } };
  metadata[offsets[2042] + 4] = { 19, 252, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2044] + 4] = { 19, 252, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2046] + 6] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 252, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2934] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 2934, 0, 0, 0 } } };
  metadata[offsets[2942] + 2] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 2942, 0, 0, 0 } } };
  metadata[offsets[2998] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 2998, 0, 0, 0 } } };
  metadata[offsets[3000] + 2] = { 19, 252, false, { { -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 3000, 0, 0, 0 } } };
  metadata[offsets[3006] + 3] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3058] + 1] = { 19, 252, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 3058, 0, 0, 0 } } };
  metadata[offsets[3062] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 3062, 0, 0, 0 } } };
  metadata[offsets[3066] + 4] = { 19, 252, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3068] + 4] = { 19, 252, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3070] + 6] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 252, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3902] + 4] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3956] + 1] = { 19, 252, false, { { -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 3956, 0, 0, 0 } } };
  metadata[offsets[3958] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 3958, 0, 0, 0 } } };
  metadata[offsets[3962] + 4] = { 19, 252, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3964] + 4] = { 19, 252, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3966] + 6] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[4020] + 1] = { 19, 252, false, { { -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4020, 0, 0, 0 } } };
  metadata[offsets[4022] + 1] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4022, 0, 0, 0 } } };
  metadata[offsets[4026] + 4] = { 19, 252, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4028] + 4] = { 19, 252, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4030] + 6] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4080] + 5] = { 19, 252, false, { { -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4080, 0, 0, 0 } } };
  metadata[offsets[4082] + 5] = { 19, 252, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4084] + 5] = { 19, 252, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4086] + 6] = { 19, 252, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4088] + 5] = { 19, 252, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4090] + 7] = { 19, 252, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4092] + 7] = { 19, 252, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4094] + 12] = { 19, 252, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 252, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  // Processing non-manifold case 20 with 8 voxel cases
  metadata[offsets[2269] + 3] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2269, 0, 0, 0 } } };
  metadata[offsets[2303] + 6] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2457] + 2] = { 20, 127, false, { { 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2457, 0, 0, 0 } } };
  metadata[offsets[2491] + 3] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2491, 0, 0, 0 } } };
  metadata[offsets[2495] + 2] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2495, 0, 0, 0 } } };
  metadata[offsets[2508] + 2] = { 20, 127, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2508, 0, 0, 0 } } };
  metadata[offsets[2525] + 3] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2525, 0, 0, 0 } } };
  metadata[offsets[2542] + 3] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2542, 0, 0, 0 } } };
  metadata[offsets[2543] + 2] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2543, 0, 0, 0 } } };
  metadata[offsets[2555] + 2] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2555, 0, 0, 0 } } };
  metadata[offsets[2558] + 2] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2558, 0, 0, 0 } } };
  metadata[offsets[2559] + 7] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2729] + 2] = { 20, 127, false, { { 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2729, 0, 0, 0 } } };
  metadata[offsets[2734] + 1] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2734, 0, 0, 0 } } };
  metadata[offsets[2735] + 3] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[2782] + 1] = { 20, 127, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2782, 0, 0, 0 } } };
  metadata[offsets[2783] + 5] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2811] + 5] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[2812] + 4] = { 20, 127, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2812, 0, 0, 0 } } };
  metadata[offsets[2813] + 5] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2814] + 5] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[2815] + 8] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2973] + 3] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2973, 0, 0, 0 } } };
  metadata[offsets[2975] + 2] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 2975, 0, 0, 0 } } };
  metadata[offsets[3000] + 3] = { 20, 127, false, { { -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3000, 0, 0, 0 } } };
  metadata[offsets[3001] + 3] = { 20, 127, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3001, 0, 0, 0 } } };
  metadata[offsets[3003] + 3] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[3005] + 3] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3006] + 4] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3007] + 8] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3023] + 5] = { 20, 127, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3037] + 3] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3038] + 4] = { 20, 127, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3039] + 7] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3050] + 1] = { 20, 127, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3050, 0, 0, 0 } } };
  metadata[offsets[3051] + 4] = { 20, 127, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3053] + 6] = { 20, 127, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3054] + 3] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[3055] + 8] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3066] + 5] = { 20, 127, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3067] + 7] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3068] + 5] = { 20, 127, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3069] + 8] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3070] + 7] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 13] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 127, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3226] + 2] = { 20, 127, false, { { -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3226, 0, 0, 0 } } };
  metadata[offsets[3263] + 5] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3279] + 4] = { 20, 127, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3279, 0, 0, 0 } } };
  metadata[offsets[3295] + 5] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3306] + 1] = { 20, 127, false, { { -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3306, 0, 0, 0 } } };
  metadata[offsets[3309] + 1] = { 20, 127, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3309, 0, 0, 0 } } };
  metadata[offsets[3311] + 5] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3322] + 3] = { 20, 127, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[3325] + 5] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3327] + 8] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3467] + 3] = { 20, 127, false, { { 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3467, 0, 0, 0 } } };
  metadata[offsets[3483] + 3] = { 20, 127, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3483, 0, 0, 0 } } };
  metadata[offsets[3502] + 1] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3502, 0, 0, 0 } } };
  metadata[offsets[3503] + 5] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3515] + 3] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[3518] + 4] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3519] + 7] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3535] + 5] = { 20, 127, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3545] + 3] = { 20, 127, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3545, 0, 0, 0 } } };
  metadata[offsets[3547] + 3] = { 20, 127, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[3549] + 3] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3550] + 6] = { 20, 127, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 8] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3563] + 4] = { 20, 127, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3565] + 4] = { 20, 127, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3566] + 3] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  metadata[offsets[3567] + 7] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3577] + 2] = { 20, 127, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3577, 0, 0, 0 } } };
  metadata[offsets[3579] + 8] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3580] + 5] = { 20, 127, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3581] + 7] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3582] + 8] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 13] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3742] + 3] = { 20, 127, false, { { -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3742, 0, 0, 0 } } };
  metadata[offsets[3743] + 2] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3743, 0, 0, 0 } } };
  metadata[offsets[3771] + 6] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3772] + 1] = { 20, 127, false, { { -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3772, 0, 0, 0 } } };
  metadata[offsets[3773] + 4] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3774] + 3] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[3775] + 8] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3787] + 1] = { 20, 127, false, { { 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3787, 0, 0, 0 } } };
  metadata[offsets[3788] + 1] = { 20, 127, false, { { -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3788, 0, 0, 0 } } };
  metadata[offsets[3791] + 5] = { 20, 127, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3803] + 4] = { 20, 127, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3805] + 4] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3806] + 3] = { 20, 127, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[3807] + 7] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3817] + 3] = { 20, 127, false, { { 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3817, 0, 0, 0 } } };
  metadata[offsets[3819] + 3] = { 20, 127, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[3821] + 3] = { 20, 127, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[3822] + 6] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 8] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3833] + 2] = { 20, 127, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3833, 0, 0, 0 } } };
  metadata[offsets[3835] + 8] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3836] + 5] = { 20, 127, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3837] + 7] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3838] + 8] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 13] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3983] + 6] = { 20, 127, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[3997] + 2] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3997, 0, 0, 0 } } };
  metadata[offsets[3998] + 2] = { 20, 127, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3998, 0, 0, 0 } } };
  metadata[offsets[3999] + 7] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4010] + 4] = { 20, 127, false, { { -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4010, 0, 0, 0 } } };
  metadata[offsets[4011] + 5] = { 20, 127, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4013] + 5] = { 20, 127, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4014] + 5] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4015] + 8] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4026] + 5] = { 20, 127, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4027] + 8] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4028] + 5] = { 20, 127, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4029] + 7] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4030] + 7] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 13] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4043] + 5] = { 20, 127, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4044] + 3] = { 20, 127, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  metadata[offsets[4047] + 8] = { 20, 127, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4057] + 2] = { 20, 127, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4057, 0, 0, 0 } } };
  metadata[offsets[4058] + 5] = { 20, 127, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4059] + 7] = { 20, 127, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4061] + 8] = { 20, 127, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4062] + 8] = { 20, 127, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 13] = { 20, 127, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4073] + 2] = { 20, 127, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4073, 0, 0, 0 } } };
  metadata[offsets[4074] + 5] = { 20, 127, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4075] + 7] = { 20, 127, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4077] + 8] = { 20, 127, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4078] + 8] = { 20, 127, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 13] = { 20, 127, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4088] + 6] = { 20, 127, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4089] + 7] = { 20, 127, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4090] + 8] = { 20, 127, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 13] = { 20, 127, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4092] + 8] = { 20, 127, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 13] = { 20, 127, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4094] + 13] = { 20, 127, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 26] = { 20, 127, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 127, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[1261] + 3] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1261, 0, 0, 0 } } };
  metadata[offsets[1279] + 6] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1369] + 2] = { 20, 191, false, { { 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1369, 0, 0, 0 } } };
  metadata[offsets[1374] + 1] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1374, 0, 0, 0 } } };
  metadata[offsets[1375] + 3] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[1518] + 1] = { 20, 191, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1518, 0, 0, 0 } } };
  metadata[offsets[1519] + 5] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1531] + 5] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[1532] + 4] = { 20, 191, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1532, 0, 0, 0 } } };
  metadata[offsets[1533] + 5] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1534] + 5] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[1535] + 8] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1641] + 2] = { 20, 191, false, { { 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1641, 0, 0, 0 } } };
  metadata[offsets[1659] + 3] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1659, 0, 0, 0 } } };
  metadata[offsets[1663] + 2] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1663, 0, 0, 0 } } };
  metadata[offsets[1740] + 2] = { 20, 191, false, { { -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1740, 0, 0, 0 } } };
  metadata[offsets[1758] + 3] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1758, 0, 0, 0 } } };
  metadata[offsets[1759] + 2] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1759, 0, 0, 0 } } };
  metadata[offsets[1773] + 3] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1773, 0, 0, 0 } } };
  metadata[offsets[1787] + 2] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1787, 0, 0, 0 } } };
  metadata[offsets[1790] + 2] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1790, 0, 0, 0 } } };
  metadata[offsets[1791] + 7] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1901] + 3] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1901, 0, 0, 0 } } };
  metadata[offsets[1903] + 2] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1903, 0, 0, 0 } } };
  metadata[offsets[1912] + 3] = { 20, 191, false, { { -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1912, 0, 0, 0 } } };
  metadata[offsets[1913] + 3] = { 20, 191, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1913, 0, 0, 0 } } };
  metadata[offsets[1915] + 3] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[1917] + 3] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[1918] + 4] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[1919] + 8] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[1999] + 5] = { 20, 191, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2010] + 1] = { 20, 191, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2010, 0, 0, 0 } } };
  metadata[offsets[2011] + 4] = { 20, 191, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2013] + 6] = { 20, 191, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2014] + 3] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2015] + 8] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2029] + 3] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[2030] + 4] = { 20, 191, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[2031] + 7] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2042] + 5] = { 20, 191, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2043] + 7] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2044] + 5] = { 20, 191, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2045] + 8] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2046] + 7] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 13] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 191, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3178] + 2] = { 20, 191, false, { { -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3178, 0, 0, 0 } } };
  metadata[offsets[3199] + 5] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3279] + 5] = { 20, 191, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3279, 0, 0, 0 } } };
  metadata[offsets[3290] + 1] = { 20, 191, false, { { -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3290, 0, 0, 0 } } };
  metadata[offsets[3293] + 1] = { 20, 191, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3293, 0, 0, 0 } } };
  metadata[offsets[3295] + 6] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3311] + 6] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3322] + 4] = { 20, 191, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[3325] + 6] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3327] + 9] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3438] + 3] = { 20, 191, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3438, 0, 0, 0 } } };
  metadata[offsets[3439] + 2] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3439, 0, 0, 0 } } };
  metadata[offsets[3451] + 6] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3452] + 1] = { 20, 191, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3452, 0, 0, 0 } } };
  metadata[offsets[3453] + 4] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3454] + 3] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[3455] + 8] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3531] + 1] = { 20, 191, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3531, 0, 0, 0 } } };
  metadata[offsets[3532] + 1] = { 20, 191, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3532, 0, 0, 0 } } };
  metadata[offsets[3535] + 6] = { 20, 191, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3545] + 4] = { 20, 191, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3545, 0, 0, 0 } } };
  metadata[offsets[3547] + 4] = { 20, 191, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[3549] + 4] = { 20, 191, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3550] + 7] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 9] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3563] + 5] = { 20, 191, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3565] + 5] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3566] + 4] = { 20, 191, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  metadata[offsets[3567] + 8] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3577] + 3] = { 20, 191, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3577, 0, 0, 0 } } };
  metadata[offsets[3579] + 9] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3580] + 6] = { 20, 191, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3581] + 8] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3582] + 9] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 14] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3659] + 3] = { 20, 191, false, { { 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3659, 0, 0, 0 } } };
  metadata[offsets[3678] + 1] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3678, 0, 0, 0 } } };
  metadata[offsets[3679] + 5] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3691] + 3] = { 20, 191, false, { { 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3691, 0, 0, 0 } } };
  metadata[offsets[3707] + 3] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[3710] + 4] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3711] + 7] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3791] + 6] = { 20, 191, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3803] + 5] = { 20, 191, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3805] + 5] = { 20, 191, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3806] + 4] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[3807] + 8] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3817] + 4] = { 20, 191, false, { { 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3817, 0, 0, 0 } } };
  metadata[offsets[3819] + 4] = { 20, 191, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[3821] + 4] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[3822] + 7] = { 20, 191, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 9] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3833] + 3] = { 20, 191, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3833, 0, 0, 0 } } };
  metadata[offsets[3835] + 9] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3836] + 6] = { 20, 191, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3837] + 8] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3838] + 9] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 14] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3919] + 6] = { 20, 191, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3930] + 4] = { 20, 191, false, { { -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3930, 0, 0, 0 } } };
  metadata[offsets[3931] + 5] = { 20, 191, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3933] + 5] = { 20, 191, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3934] + 5] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3935] + 8] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3949] + 2] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3949, 0, 0, 0 } } };
  metadata[offsets[3950] + 2] = { 20, 191, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3950, 0, 0, 0 } } };
  metadata[offsets[3951] + 7] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3962] + 5] = { 20, 191, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3963] + 8] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3964] + 5] = { 20, 191, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3965] + 7] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3966] + 7] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 13] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4043] + 6] = { 20, 191, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4044] + 4] = { 20, 191, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  metadata[offsets[4047] + 9] = { 20, 191, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4057] + 3] = { 20, 191, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4057, 0, 0, 0 } } };
  metadata[offsets[4058] + 6] = { 20, 191, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4059] + 8] = { 20, 191, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4061] + 9] = { 20, 191, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4062] + 9] = { 20, 191, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 14] = { 20, 191, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4073] + 3] = { 20, 191, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4073, 0, 0, 0 } } };
  metadata[offsets[4074] + 6] = { 20, 191, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4075] + 8] = { 20, 191, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4077] + 9] = { 20, 191, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4078] + 9] = { 20, 191, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 14] = { 20, 191, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4088] + 7] = { 20, 191, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4089] + 8] = { 20, 191, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4090] + 9] = { 20, 191, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 14] = { 20, 191, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4092] + 9] = { 20, 191, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 14] = { 20, 191, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4094] + 14] = { 20, 191, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 27] = { 20, 191, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 191, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[734] + 3] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 734, 0, 0, 0 } } };
  metadata[offsets[767] + 6] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[917] + 2] = { 20, 223, false, { { 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 917, 0, 0, 0 } } };
  metadata[offsets[959] + 5] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[975] + 4] = { 20, 223, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 975, 0, 0, 0 } } };
  metadata[offsets[991] + 5] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[997] + 1] = { 20, 223, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 997, 0, 0, 0 } } };
  metadata[offsets[1006] + 1] = { 20, 223, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 1006, 0, 0, 0 } } };
  metadata[offsets[1007] + 5] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[1013] + 3] = { 20, 223, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1022] + 5] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1023] + 8] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 223, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1686] + 2] = { 20, 223, false, { { -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1686, 0, 0, 0 } } };
  metadata[offsets[1719] + 3] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1719, 0, 0, 0 } } };
  metadata[offsets[1727] + 2] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1727, 0, 0, 0 } } };
  metadata[offsets[1740] + 3] = { 20, 223, false, { { -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1740, 0, 0, 0 } } };
  metadata[offsets[1758] + 4] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1758, 0, 0, 0 } } };
  metadata[offsets[1773] + 4] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1773, 0, 0, 0 } } };
  metadata[offsets[1775] + 2] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1775, 0, 0, 0 } } };
  metadata[offsets[1783] + 2] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1783, 0, 0, 0 } } };
  metadata[offsets[1789] + 2] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1789, 0, 0, 0 } } };
  metadata[offsets[1791] + 8] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1927] + 3] = { 20, 223, false, { { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1927, 0, 0, 0 } } };
  metadata[offsets[1943] + 3] = { 20, 223, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1943, 0, 0, 0 } } };
  metadata[offsets[1965] + 1] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1965, 0, 0, 0 } } };
  metadata[offsets[1967] + 5] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1975] + 3] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[1981] + 4] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[1983] + 7] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[1999] + 6] = { 20, 223, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2006] + 3] = { 20, 223, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2006, 0, 0, 0 } } };
  metadata[offsets[2007] + 3] = { 20, 223, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2013] + 7] = { 20, 223, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2014] + 4] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2015] + 9] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2023] + 4] = { 20, 223, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2029] + 4] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[2030] + 5] = { 20, 223, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[2031] + 8] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2038] + 2] = { 20, 223, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2038, 0, 0, 0 } } };
  metadata[offsets[2039] + 8] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2044] + 6] = { 20, 223, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2045] + 9] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2046] + 8] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 14] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 223, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2726] + 2] = { 20, 223, false, { { -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2726, 0, 0, 0 } } };
  metadata[offsets[2733] + 1] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2733, 0, 0, 0 } } };
  metadata[offsets[2735] + 4] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[2781] + 1] = { 20, 223, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2781, 0, 0, 0 } } };
  metadata[offsets[2783] + 6] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2807] + 5] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[2812] + 5] = { 20, 223, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2812, 0, 0, 0 } } };
  metadata[offsets[2813] + 6] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2814] + 6] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[2815] + 9] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2973] + 4] = { 20, 223, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2973, 0, 0, 0 } } };
  metadata[offsets[2975] + 3] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2975, 0, 0, 0 } } };
  metadata[offsets[2999] + 6] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3004] + 1] = { 20, 223, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3004, 0, 0, 0 } } };
  metadata[offsets[3005] + 4] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3006] + 5] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3007] + 9] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3015] + 1] = { 20, 223, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3015, 0, 0, 0 } } };
  metadata[offsets[3020] + 1] = { 20, 223, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3020, 0, 0, 0 } } };
  metadata[offsets[3023] + 6] = { 20, 223, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3031] + 4] = { 20, 223, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3037] + 4] = { 20, 223, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3038] + 5] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3039] + 8] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3046] + 3] = { 20, 223, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3046, 0, 0, 0 } } };
  metadata[offsets[3047] + 3] = { 20, 223, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[3053] + 7] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3054] + 4] = { 20, 223, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[3055] + 9] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3062] + 2] = { 20, 223, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3062, 0, 0, 0 } } };
  metadata[offsets[3063] + 8] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3068] + 6] = { 20, 223, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3069] + 9] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3070] + 8] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 14] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 223, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3742] + 4] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3742, 0, 0, 0 } } };
  metadata[offsets[3743] + 3] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3743, 0, 0, 0 } } };
  metadata[offsets[3764] + 3] = { 20, 223, false, { { -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3764, 0, 0, 0 } } };
  metadata[offsets[3766] + 3] = { 20, 223, false, { { -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3766, 0, 0, 0 } } };
  metadata[offsets[3767] + 3] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[3773] + 5] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3774] + 4] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[3775] + 9] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3791] + 7] = { 20, 223, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3805] + 6] = { 20, 223, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3806] + 5] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[3807] + 9] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3813] + 1] = { 20, 223, false, { { 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3813, 0, 0, 0 } } };
  metadata[offsets[3815] + 4] = { 20, 223, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3821] + 5] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[3822] + 8] = { 20, 223, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 10] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3829] + 5] = { 20, 223, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3831] + 7] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3836] + 7] = { 20, 223, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3837] + 9] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3838] + 10] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 15] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3983] + 7] = { 20, 223, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[3997] + 3] = { 20, 223, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3997, 0, 0, 0 } } };
  metadata[offsets[3998] + 3] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3998, 0, 0, 0 } } };
  metadata[offsets[3999] + 8] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4005] + 4] = { 20, 223, false, { { 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4005, 0, 0, 0 } } };
  metadata[offsets[4007] + 5] = { 20, 223, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4013] + 6] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4014] + 6] = { 20, 223, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4015] + 9] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4021] + 5] = { 20, 223, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4023] + 8] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4028] + 6] = { 20, 223, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4029] + 8] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4030] + 8] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 14] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4039] + 5] = { 20, 223, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4044] + 5] = { 20, 223, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  metadata[offsets[4047] + 10] = { 20, 223, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4053] + 5] = { 20, 223, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4054] + 2] = { 20, 223, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4054, 0, 0, 0 } } };
  metadata[offsets[4055] + 7] = { 20, 223, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4061] + 10] = { 20, 223, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4062] + 10] = { 20, 223, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 15] = { 20, 223, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4069] + 5] = { 20, 223, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4070] + 2] = { 20, 223, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4070, 0, 0, 0 } } };
  metadata[offsets[4071] + 7] = { 20, 223, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4077] + 10] = { 20, 223, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4078] + 10] = { 20, 223, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 15] = { 20, 223, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4084] + 6] = { 20, 223, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4085] + 8] = { 20, 223, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4086] + 7] = { 20, 223, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 13] = { 20, 223, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4092] + 10] = { 20, 223, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 15] = { 20, 223, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4094] + 15] = { 20, 223, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 28] = { 20, 223, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 223, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[494] + 3] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 494, 0, 0, 0 } } };
  metadata[offsets[511] + 6] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[869] + 2] = { 20, 239, false, { { 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 869, 0, 0, 0 } } };
  metadata[offsets[895] + 5] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[975] + 5] = { 20, 239, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 975, 0, 0, 0 } } };
  metadata[offsets[981] + 1] = { 20, 239, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 981, 0, 0, 0 } } };
  metadata[offsets[990] + 1] = { 20, 239, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 990, 0, 0, 0 } } };
  metadata[offsets[991] + 6] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[1007] + 6] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[1013] + 4] = { 20, 239, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1022] + 6] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1023] + 9] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 239, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1366] + 2] = { 20, 239, false, { { -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1366, 0, 0, 0 } } };
  metadata[offsets[1373] + 1] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1373, 0, 0, 0 } } };
  metadata[offsets[1375] + 4] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[1517] + 1] = { 20, 239, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1517, 0, 0, 0 } } };
  metadata[offsets[1519] + 6] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1527] + 5] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1532] + 5] = { 20, 239, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1532, 0, 0, 0 } } };
  metadata[offsets[1533] + 6] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1534] + 6] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[1535] + 9] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1901] + 4] = { 20, 239, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1901, 0, 0, 0 } } };
  metadata[offsets[1903] + 3] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1903, 0, 0, 0 } } };
  metadata[offsets[1911] + 6] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1916] + 1] = { 20, 239, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1916, 0, 0, 0 } } };
  metadata[offsets[1917] + 4] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[1918] + 5] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[1919] + 9] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[1991] + 1] = { 20, 239, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1991, 0, 0, 0 } } };
  metadata[offsets[1996] + 1] = { 20, 239, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1996, 0, 0, 0 } } };
  metadata[offsets[1999] + 7] = { 20, 239, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2006] + 4] = { 20, 239, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2006, 0, 0, 0 } } };
  metadata[offsets[2007] + 4] = { 20, 239, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2013] + 8] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2014] + 5] = { 20, 239, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2015] + 10] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2023] + 5] = { 20, 239, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2029] + 5] = { 20, 239, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[2030] + 6] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[2031] + 9] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2038] + 3] = { 20, 239, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2038, 0, 0, 0 } } };
  metadata[offsets[2039] + 9] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2044] + 7] = { 20, 239, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2045] + 10] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2046] + 9] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 15] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 239, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2406] + 2] = { 20, 239, false, { { -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2406, 0, 0, 0 } } };
  metadata[offsets[2423] + 3] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2423, 0, 0, 0 } } };
  metadata[offsets[2431] + 2] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2431, 0, 0, 0 } } };
  metadata[offsets[2508] + 3] = { 20, 239, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2508, 0, 0, 0 } } };
  metadata[offsets[2525] + 4] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2525, 0, 0, 0 } } };
  metadata[offsets[2527] + 2] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2527, 0, 0, 0 } } };
  metadata[offsets[2542] + 4] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2542, 0, 0, 0 } } };
  metadata[offsets[2551] + 2] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2551, 0, 0, 0 } } };
  metadata[offsets[2557] + 2] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2557, 0, 0, 0 } } };
  metadata[offsets[2559] + 8] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2887] + 3] = { 20, 239, false, { { 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2887, 0, 0, 0 } } };
  metadata[offsets[2909] + 1] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2909, 0, 0, 0 } } };
  metadata[offsets[2911] + 5] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[2919] + 3] = { 20, 239, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2919, 0, 0, 0 } } };
  metadata[offsets[2935] + 3] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[2941] + 4] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[2943] + 7] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[3023] + 7] = { 20, 239, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3031] + 5] = { 20, 239, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3037] + 5] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3038] + 6] = { 20, 239, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3039] + 9] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3046] + 4] = { 20, 239, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3046, 0, 0, 0 } } };
  metadata[offsets[3047] + 4] = { 20, 239, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[3053] + 8] = { 20, 239, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3054] + 5] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[3055] + 10] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3062] + 3] = { 20, 239, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3062, 0, 0, 0 } } };
  metadata[offsets[3063] + 9] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3068] + 7] = { 20, 239, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3069] + 10] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3070] + 9] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 15] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 239, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3438] + 4] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3438, 0, 0, 0 } } };
  metadata[offsets[3439] + 3] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3439, 0, 0, 0 } } };
  metadata[offsets[3444] + 3] = { 20, 239, false, { { -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3444, 0, 0, 0 } } };
  metadata[offsets[3446] + 3] = { 20, 239, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3446, 0, 0, 0 } } };
  metadata[offsets[3447] + 3] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[3453] + 5] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3454] + 4] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[3455] + 9] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3535] + 7] = { 20, 239, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3541] + 1] = { 20, 239, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3541, 0, 0, 0 } } };
  metadata[offsets[3543] + 4] = { 20, 239, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3549] + 5] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3550] + 8] = { 20, 239, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 10] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3565] + 6] = { 20, 239, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3566] + 5] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  metadata[offsets[3567] + 9] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3573] + 5] = { 20, 239, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3575] + 7] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3580] + 7] = { 20, 239, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3581] + 9] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3582] + 10] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 15] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3919] + 7] = { 20, 239, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3925] + 4] = { 20, 239, false, { { 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3925, 0, 0, 0 } } };
  metadata[offsets[3927] + 5] = { 20, 239, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3933] + 6] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3934] + 6] = { 20, 239, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3935] + 9] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3949] + 3] = { 20, 239, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3949, 0, 0, 0 } } };
  metadata[offsets[3950] + 3] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3950, 0, 0, 0 } } };
  metadata[offsets[3951] + 8] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3957] + 5] = { 20, 239, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3959] + 8] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3964] + 6] = { 20, 239, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3965] + 8] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3966] + 8] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 14] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4039] + 6] = { 20, 239, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4044] + 6] = { 20, 239, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  metadata[offsets[4047] + 11] = { 20, 239, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4053] + 6] = { 20, 239, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4054] + 3] = { 20, 239, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4054, 0, 0, 0 } } };
  metadata[offsets[4055] + 8] = { 20, 239, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4061] + 11] = { 20, 239, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4062] + 11] = { 20, 239, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 16] = { 20, 239, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4069] + 6] = { 20, 239, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4070] + 3] = { 20, 239, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4070, 0, 0, 0 } } };
  metadata[offsets[4071] + 8] = { 20, 239, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4077] + 11] = { 20, 239, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4078] + 11] = { 20, 239, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 16] = { 20, 239, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4084] + 7] = { 20, 239, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4085] + 9] = { 20, 239, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4086] + 8] = { 20, 239, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 14] = { 20, 239, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4092] + 11] = { 20, 239, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 16] = { 20, 239, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4094] + 16] = { 20, 239, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 29] = { 20, 239, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 239, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[2167] + 3] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2167, 0, 0, 0 } } };
  metadata[offsets[2303] + 7] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2355] + 2] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2355, 0, 0, 0 } } };
  metadata[offsets[2406] + 3] = { 20, 247, false, { { -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2406, 0, 0, 0 } } };
  metadata[offsets[2423] + 4] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2423, 0, 0, 0 } } };
  metadata[offsets[2491] + 4] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2491, 0, 0, 0 } } };
  metadata[offsets[2495] + 3] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2495, 0, 0, 0 } } };
  metadata[offsets[2542] + 5] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2542, 0, 0, 0 } } };
  metadata[offsets[2543] + 3] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2543, 0, 0, 0 } } };
  metadata[offsets[2555] + 3] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2555, 0, 0, 0 } } };
  metadata[offsets[2558] + 3] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2558, 0, 0, 0 } } };
  metadata[offsets[2559] + 9] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2683] + 1] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2683, 0, 0, 0 } } };
  metadata[offsets[2687] + 5] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2726] + 3] = { 20, 247, false, { { -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2726, 0, 0, 0 } } };
  metadata[offsets[2731] + 1] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2731, 0, 0, 0 } } };
  metadata[offsets[2735] + 5] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[2803] + 4] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2803, 0, 0, 0 } } };
  metadata[offsets[2807] + 6] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[2811] + 6] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[2814] + 7] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[2815] + 10] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2879] + 5] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2919] + 4] = { 20, 247, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2919, 0, 0, 0 } } };
  metadata[offsets[2927] + 2] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2927, 0, 0, 0 } } };
  metadata[offsets[2935] + 4] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[2939] + 4] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[2943] + 8] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[2999] + 7] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3002] + 1] = { 20, 247, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3002, 0, 0, 0 } } };
  metadata[offsets[3003] + 4] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[3006] + 6] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3007] + 10] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3042] + 3] = { 20, 247, false, { { -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3042, 0, 0, 0 } } };
  metadata[offsets[3046] + 5] = { 20, 247, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3046, 0, 0, 0 } } };
  metadata[offsets[3047] + 5] = { 20, 247, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[3051] + 5] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3054] + 6] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[3055] + 11] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3059] + 5] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3063] + 10] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3066] + 6] = { 20, 247, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3067] + 8] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3070] + 10] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 16] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 247, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3135] + 4] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3135, 0, 0, 0 } } };
  metadata[offsets[3178] + 3] = { 20, 247, false, { { -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3178, 0, 0, 0 } } };
  metadata[offsets[3199] + 6] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3255] + 1] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3255, 0, 0, 0 } } };
  metadata[offsets[3258] + 1] = { 20, 247, false, { { -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3258, 0, 0, 0 } } };
  metadata[offsets[3263] + 6] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3311] + 7] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3319] + 5] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3322] + 5] = { 20, 247, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[3327] + 10] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3374] + 3] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3374, 0, 0, 0 } } };
  metadata[offsets[3391] + 5] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3438] + 5] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3438, 0, 0, 0 } } };
  metadata[offsets[3446] + 4] = { 20, 247, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3446, 0, 0, 0 } } };
  metadata[offsets[3447] + 4] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[3451] + 7] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3454] + 5] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[3455] + 10] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3499] + 1] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3499, 0, 0, 0 } } };
  metadata[offsets[3503] + 6] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3511] + 4] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3515] + 4] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[3518] + 5] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3519] + 8] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3563] + 6] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3566] + 6] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  metadata[offsets[3567] + 10] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3571] + 5] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3574] + 2] = { 20, 247, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3574, 0, 0, 0 } } };
  metadata[offsets[3575] + 8] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3579] + 10] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3582] + 11] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 16] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3635] + 1] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3635, 0, 0, 0 } } };
  metadata[offsets[3646] + 1] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3646, 0, 0, 0 } } };
  metadata[offsets[3647] + 5] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3691] + 4] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3691, 0, 0, 0 } } };
  metadata[offsets[3695] + 2] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3695, 0, 0, 0 } } };
  metadata[offsets[3703] + 4] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3707] + 4] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[3710] + 5] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3711] + 8] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3766] + 4] = { 20, 247, false, { { -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3766, 0, 0, 0 } } };
  metadata[offsets[3767] + 4] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[3771] + 7] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3774] + 5] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[3775] + 10] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3811] + 1] = { 20, 247, false, { { 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3811, 0, 0, 0 } } };
  metadata[offsets[3815] + 5] = { 20, 247, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3819] + 5] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[3822] + 9] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 11] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3827] + 5] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3830] + 2] = { 20, 247, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3830, 0, 0, 0 } } };
  metadata[offsets[3831] + 8] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3835] + 10] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3838] + 11] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 16] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3887] + 6] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3891] + 3] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[3902] + 5] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3903] + 8] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3943] + 2] = { 20, 247, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3943, 0, 0, 0 } } };
  metadata[offsets[3947] + 2] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3947, 0, 0, 0 } } };
  metadata[offsets[3951] + 9] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3958] + 2] = { 20, 247, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3958, 0, 0, 0 } } };
  metadata[offsets[3959] + 9] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3962] + 6] = { 20, 247, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3963] + 9] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3966] + 9] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 15] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4007] + 6] = { 20, 247, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4010] + 5] = { 20, 247, false, { { -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4010, 0, 0, 0 } } };
  metadata[offsets[4011] + 6] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4014] + 7] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4015] + 10] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4022] + 2] = { 20, 247, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4022, 0, 0, 0 } } };
  metadata[offsets[4023] + 9] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4026] + 6] = { 20, 247, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4027] + 9] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4030] + 9] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 15] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4067] + 5] = { 20, 247, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4071] + 9] = { 20, 247, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4074] + 7] = { 20, 247, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4075] + 9] = { 20, 247, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4078] + 12] = { 20, 247, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 17] = { 20, 247, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4082] + 6] = { 20, 247, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4083] + 8] = { 20, 247, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4086] + 9] = { 20, 247, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 15] = { 20, 247, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4090] + 10] = { 20, 247, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 15] = { 20, 247, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4094] + 17] = { 20, 247, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 30] = { 20, 247, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 247, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[1207] + 3] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1207, 0, 0, 0 } } };
  metadata[offsets[1279] + 7] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1366] + 3] = { 20, 251, false, { { -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1366, 0, 0, 0 } } };
  metadata[offsets[1371] + 1] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1371, 0, 0, 0 } } };
  metadata[offsets[1375] + 5] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[1467] + 1] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1467, 0, 0, 0 } } };
  metadata[offsets[1471] + 5] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1523] + 4] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1523, 0, 0, 0 } } };
  metadata[offsets[1527] + 6] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1531] + 6] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[1534] + 7] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[1535] + 10] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1587] + 2] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1587, 0, 0, 0 } } };
  metadata[offsets[1659] + 4] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1659, 0, 0, 0 } } };
  metadata[offsets[1663] + 3] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1663, 0, 0, 0 } } };
  metadata[offsets[1686] + 3] = { 20, 251, false, { { -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1686, 0, 0, 0 } } };
  metadata[offsets[1719] + 4] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1719, 0, 0, 0 } } };
  metadata[offsets[1758] + 5] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1758, 0, 0, 0 } } };
  metadata[offsets[1759] + 3] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1759, 0, 0, 0 } } };
  metadata[offsets[1787] + 3] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1787, 0, 0, 0 } } };
  metadata[offsets[1790] + 3] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1790, 0, 0, 0 } } };
  metadata[offsets[1791] + 9] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1855] + 5] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1911] + 7] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1914] + 1] = { 20, 251, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1914, 0, 0, 0 } } };
  metadata[offsets[1915] + 4] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[1918] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[1919] + 10] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[1943] + 4] = { 20, 251, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1943, 0, 0, 0 } } };
  metadata[offsets[1951] + 2] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1951, 0, 0, 0 } } };
  metadata[offsets[1975] + 4] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[1979] + 4] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[1983] + 8] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2002] + 3] = { 20, 251, false, { { -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2002, 0, 0, 0 } } };
  metadata[offsets[2006] + 5] = { 20, 251, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2006, 0, 0, 0 } } };
  metadata[offsets[2007] + 5] = { 20, 251, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2011] + 5] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2014] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2015] + 11] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2035] + 5] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2039] + 10] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2042] + 6] = { 20, 251, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2043] + 8] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2046] + 10] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 16] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 251, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[3135] + 5] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3135, 0, 0, 0 } } };
  metadata[offsets[3191] + 1] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3191, 0, 0, 0 } } };
  metadata[offsets[3194] + 1] = { 20, 251, false, { { -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3194, 0, 0, 0 } } };
  metadata[offsets[3199] + 7] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3226] + 3] = { 20, 251, false, { { -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3226, 0, 0, 0 } } };
  metadata[offsets[3263] + 7] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3295] + 7] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3319] + 6] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3322] + 6] = { 20, 251, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[3327] + 11] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3379] + 1] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3379, 0, 0, 0 } } };
  metadata[offsets[3390] + 1] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3390, 0, 0, 0 } } };
  metadata[offsets[3391] + 6] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3446] + 5] = { 20, 251, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3446, 0, 0, 0 } } };
  metadata[offsets[3447] + 5] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[3451] + 8] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3454] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[3455] + 11] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3483] + 4] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3483, 0, 0, 0 } } };
  metadata[offsets[3487] + 2] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3487, 0, 0, 0 } } };
  metadata[offsets[3511] + 5] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3515] + 5] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[3518] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3519] + 9] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3539] + 1] = { 20, 251, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3539, 0, 0, 0 } } };
  metadata[offsets[3543] + 5] = { 20, 251, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3547] + 5] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[3550] + 9] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 11] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3571] + 6] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3574] + 3] = { 20, 251, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3574, 0, 0, 0 } } };
  metadata[offsets[3575] + 9] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3579] + 11] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3582] + 12] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 17] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3614] + 3] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3614, 0, 0, 0 } } };
  metadata[offsets[3647] + 6] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3675] + 1] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3675, 0, 0, 0 } } };
  metadata[offsets[3679] + 6] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3703] + 5] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3707] + 5] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[3710] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3711] + 9] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3742] + 5] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3742, 0, 0, 0 } } };
  metadata[offsets[3766] + 5] = { 20, 251, false, { { -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3766, 0, 0, 0 } } };
  metadata[offsets[3767] + 5] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[3771] + 8] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3774] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[3775] + 11] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3803] + 6] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3806] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[3807] + 10] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3827] + 6] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3830] + 3] = { 20, 251, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3830, 0, 0, 0 } } };
  metadata[offsets[3831] + 9] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3835] + 11] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3838] + 12] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 17] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3871] + 6] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3891] + 4] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[3902] + 6] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3903] + 9] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3927] + 6] = { 20, 251, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3930] + 5] = { 20, 251, false, { { -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3930, 0, 0, 0 } } };
  metadata[offsets[3931] + 6] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3934] + 7] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3935] + 10] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3958] + 3] = { 20, 251, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3958, 0, 0, 0 } } };
  metadata[offsets[3959] + 10] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3962] + 7] = { 20, 251, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3963] + 10] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3966] + 10] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 16] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[3991] + 2] = { 20, 251, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3991, 0, 0, 0 } } };
  metadata[offsets[3995] + 2] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3995, 0, 0, 0 } } };
  metadata[offsets[3999] + 9] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4022] + 3] = { 20, 251, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4022, 0, 0, 0 } } };
  metadata[offsets[4023] + 10] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4026] + 7] = { 20, 251, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4027] + 10] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4030] + 10] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 16] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4051] + 5] = { 20, 251, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4055] + 9] = { 20, 251, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4058] + 7] = { 20, 251, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4059] + 9] = { 20, 251, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4062] + 12] = { 20, 251, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 17] = { 20, 251, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4082] + 7] = { 20, 251, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4083] + 9] = { 20, 251, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4086] + 10] = { 20, 251, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 16] = { 20, 251, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4090] + 11] = { 20, 251, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 16] = { 20, 251, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4094] + 18] = { 20, 251, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 31] = { 20, 251, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 251, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[635] + 3] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 635, 0, 0, 0 } } };
  metadata[offsets[767] + 7] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[831] + 4] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 831, 0, 0, 0 } } };
  metadata[offsets[869] + 3] = { 20, 253, false, { { 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 869, 0, 0, 0 } } };
  metadata[offsets[895] + 6] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[949] + 1] = { 20, 253, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 949, 0, 0, 0 } } };
  metadata[offsets[955] + 1] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 955, 0, 0, 0 } } };
  metadata[offsets[959] + 6] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[1007] + 7] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[1013] + 5] = { 20, 253, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1019] + 5] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[1023] + 10] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 253, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1587] + 3] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1587, 0, 0, 0 } } };
  metadata[offsets[1641] + 3] = { 20, 253, false, { { 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1641, 0, 0, 0 } } };
  metadata[offsets[1659] + 5] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1659, 0, 0, 0 } } };
  metadata[offsets[1719] + 5] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1719, 0, 0, 0 } } };
  metadata[offsets[1727] + 3] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1727, 0, 0, 0 } } };
  metadata[offsets[1773] + 5] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1773, 0, 0, 0 } } };
  metadata[offsets[1775] + 3] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1775, 0, 0, 0 } } };
  metadata[offsets[1783] + 3] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1783, 0, 0, 0 } } };
  metadata[offsets[1789] + 3] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1789, 0, 0, 0 } } };
  metadata[offsets[1791] + 10] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1837] + 3] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1837, 0, 0, 0 } } };
  metadata[offsets[1855] + 6] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1901] + 5] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1901, 0, 0, 0 } } };
  metadata[offsets[1911] + 8] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1913] + 4] = { 20, 253, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1913, 0, 0, 0 } } };
  metadata[offsets[1915] + 5] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[1917] + 5] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[1919] + 11] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[1959] + 1] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1959, 0, 0, 0 } } };
  metadata[offsets[1967] + 6] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1975] + 5] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[1979] + 5] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[1981] + 5] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[1983] + 9] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2023] + 6] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2029] + 6] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[2031] + 10] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2035] + 6] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2039] + 11] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2041] + 2] = { 20, 253, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2041, 0, 0, 0 } } };
  metadata[offsets[2043] + 9] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2045] + 11] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2047] + 17] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 253, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2679] + 1] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2679, 0, 0, 0 } } };
  metadata[offsets[2687] + 6] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2727] + 1] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2727, 0, 0, 0 } } };
  metadata[offsets[2729] + 3] = { 20, 253, false, { { 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2729, 0, 0, 0 } } };
  metadata[offsets[2735] + 6] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[2803] + 5] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2803, 0, 0, 0 } } };
  metadata[offsets[2807] + 7] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[2811] + 7] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[2813] + 7] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2815] + 11] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2867] + 1] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2867, 0, 0, 0 } } };
  metadata[offsets[2877] + 1] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2877, 0, 0, 0 } } };
  metadata[offsets[2879] + 6] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2919] + 5] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2919, 0, 0, 0 } } };
  metadata[offsets[2927] + 3] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2927, 0, 0, 0 } } };
  metadata[offsets[2935] + 5] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[2939] + 5] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[2941] + 5] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[2943] + 9] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[2999] + 8] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3001] + 4] = { 20, 253, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3001, 0, 0, 0 } } };
  metadata[offsets[3003] + 5] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[3005] + 5] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3007] + 11] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3043] + 1] = { 20, 253, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3043, 0, 0, 0 } } };
  metadata[offsets[3047] + 6] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[3051] + 6] = { 20, 253, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3053] + 9] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3055] + 12] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3059] + 6] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3063] + 11] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3065] + 2] = { 20, 253, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3065, 0, 0, 0 } } };
  metadata[offsets[3067] + 9] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3069] + 11] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3071] + 17] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 253, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3647] + 7] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3691] + 5] = { 20, 253, false, { { 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3691, 0, 0, 0 } } };
  metadata[offsets[3695] + 3] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3695, 0, 0, 0 } } };
  metadata[offsets[3703] + 6] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3707] + 6] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[3711] + 10] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3765] + 1] = { 20, 253, false, { { 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3765, 0, 0, 0 } } };
  metadata[offsets[3767] + 6] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[3771] + 9] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3773] + 6] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3775] + 12] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3809] + 3] = { 20, 253, false, { { 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3809, 0, 0, 0 } } };
  metadata[offsets[3815] + 6] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3817] + 5] = { 20, 253, false, { { 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3817, 0, 0, 0 } } };
  metadata[offsets[3819] + 6] = { 20, 253, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[3821] + 6] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[3823] + 12] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3827] + 7] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3829] + 6] = { 20, 253, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3831] + 10] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3835] + 12] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3837] + 10] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3839] + 18] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3887] + 7] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3891] + 5] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[3901] + 5] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3903] + 10] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3943] + 3] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3943, 0, 0, 0 } } };
  metadata[offsets[3947] + 3] = { 20, 253, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3947, 0, 0, 0 } } };
  metadata[offsets[3951] + 10] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3957] + 6] = { 20, 253, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3959] + 11] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3961] + 2] = { 20, 253, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3961, 0, 0, 0 } } };
  metadata[offsets[3963] + 11] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3965] + 9] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3967] + 17] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[4005] + 5] = { 20, 253, false, { { 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4005, 0, 0, 0 } } };
  metadata[offsets[4007] + 7] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4011] + 7] = { 20, 253, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4013] + 7] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4015] + 11] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4021] + 6] = { 20, 253, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4023] + 11] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4025] + 2] = { 20, 253, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4025, 0, 0, 0 } } };
  metadata[offsets[4027] + 11] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4029] + 9] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4031] + 17] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4067] + 6] = { 20, 253, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4069] + 7] = { 20, 253, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4071] + 10] = { 20, 253, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4075] + 10] = { 20, 253, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4077] + 12] = { 20, 253, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4079] + 18] = { 20, 253, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4081] + 6] = { 20, 253, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4083] + 10] = { 20, 253, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4085] + 10] = { 20, 253, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4087] + 17] = { 20, 253, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4089] + 9] = { 20, 253, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4091] + 17] = { 20, 253, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4093] + 17] = { 20, 253, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 32] = { 20, 253, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 253, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  metadata[offsets[443] + 3] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 443, 0, 0, 0 } } };
  metadata[offsets[511] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[831] + 5] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 831, 0, 0, 0 } } };
  metadata[offsets[885] + 1] = { 20, 254, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 885, 0, 0, 0 } } };
  metadata[offsets[891] + 1] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 891, 0, 0, 0 } } };
  metadata[offsets[895] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[917] + 3] = { 20, 254, false, { { 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 917, 0, 0, 0 } } };
  metadata[offsets[959] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[991] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[1013] + 6] = { 20, 254, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1019] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[1023] + 11] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 254, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1367] + 1] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1367, 0, 0, 0 } } };
  metadata[offsets[1369] + 3] = { 20, 254, false, { { 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1369, 0, 0, 0 } } };
  metadata[offsets[1375] + 6] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[1463] + 1] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1463, 0, 0, 0 } } };
  metadata[offsets[1471] + 6] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1523] + 5] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1523, 0, 0, 0 } } };
  metadata[offsets[1527] + 7] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1531] + 7] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[1533] + 7] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1535] + 11] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1843] + 1] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1843, 0, 0, 0 } } };
  metadata[offsets[1853] + 1] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1853, 0, 0, 0 } } };
  metadata[offsets[1855] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1911] + 9] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1913] + 5] = { 20, 254, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1913, 0, 0, 0 } } };
  metadata[offsets[1915] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[1917] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[1919] + 12] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[1943] + 5] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1943, 0, 0, 0 } } };
  metadata[offsets[1951] + 3] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1951, 0, 0, 0 } } };
  metadata[offsets[1975] + 6] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[1979] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[1981] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[1983] + 10] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[2003] + 1] = { 20, 254, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2003, 0, 0, 0 } } };
  metadata[offsets[2007] + 6] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2011] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2013] + 9] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2015] + 12] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2035] + 7] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2039] + 12] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2041] + 3] = { 20, 254, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2041, 0, 0, 0 } } };
  metadata[offsets[2043] + 10] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2045] + 12] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2047] + 18] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 254, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2355] + 3] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2355, 0, 0, 0 } } };
  metadata[offsets[2423] + 5] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2423, 0, 0, 0 } } };
  metadata[offsets[2431] + 3] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2431, 0, 0, 0 } } };
  metadata[offsets[2457] + 3] = { 20, 254, false, { { 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2457, 0, 0, 0 } } };
  metadata[offsets[2491] + 5] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2491, 0, 0, 0 } } };
  metadata[offsets[2525] + 5] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2525, 0, 0, 0 } } };
  metadata[offsets[2527] + 3] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2527, 0, 0, 0 } } };
  metadata[offsets[2551] + 3] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2551, 0, 0, 0 } } };
  metadata[offsets[2557] + 3] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2557, 0, 0, 0 } } };
  metadata[offsets[2559] + 10] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2845] + 3] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2845, 0, 0, 0 } } };
  metadata[offsets[2879] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2903] + 1] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2903, 0, 0, 0 } } };
  metadata[offsets[2911] + 6] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[2935] + 6] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[2939] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[2941] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[2943] + 10] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[2973] + 5] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2973, 0, 0, 0 } } };
  metadata[offsets[2999] + 9] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3001] + 5] = { 20, 254, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3001, 0, 0, 0 } } };
  metadata[offsets[3003] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[3005] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3007] + 12] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3031] + 6] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3037] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3039] + 10] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3059] + 7] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3063] + 12] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3065] + 3] = { 20, 254, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3065, 0, 0, 0 } } };
  metadata[offsets[3067] + 10] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3069] + 12] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3071] + 18] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 254, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3391] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3445] + 1] = { 20, 254, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3445, 0, 0, 0 } } };
  metadata[offsets[3447] + 6] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[3451] + 9] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3453] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3455] + 12] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3483] + 5] = { 20, 254, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3483, 0, 0, 0 } } };
  metadata[offsets[3487] + 3] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3487, 0, 0, 0 } } };
  metadata[offsets[3511] + 6] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3515] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[3519] + 10] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3537] + 3] = { 20, 254, false, { { 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3537, 0, 0, 0 } } };
  metadata[offsets[3543] + 6] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3545] + 5] = { 20, 254, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3545, 0, 0, 0 } } };
  metadata[offsets[3547] + 6] = { 20, 254, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[3549] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3551] + 12] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3571] + 7] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3573] + 6] = { 20, 254, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3575] + 10] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3579] + 12] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3581] + 10] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3583] + 18] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3871] + 7] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3891] + 6] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[3901] + 6] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3903] + 11] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3925] + 5] = { 20, 254, false, { { 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3925, 0, 0, 0 } } };
  metadata[offsets[3927] + 7] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3931] + 7] = { 20, 254, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3933] + 7] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3935] + 11] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3957] + 7] = { 20, 254, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3959] + 12] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3961] + 3] = { 20, 254, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3961, 0, 0, 0 } } };
  metadata[offsets[3963] + 12] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3965] + 10] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3967] + 18] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[3991] + 3] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3991, 0, 0, 0 } } };
  metadata[offsets[3995] + 3] = { 20, 254, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3995, 0, 0, 0 } } };
  metadata[offsets[3999] + 10] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4021] + 7] = { 20, 254, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4023] + 12] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4025] + 3] = { 20, 254, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4025, 0, 0, 0 } } };
  metadata[offsets[4027] + 12] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4029] + 10] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4031] + 18] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4051] + 6] = { 20, 254, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4053] + 7] = { 20, 254, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4055] + 10] = { 20, 254, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4059] + 10] = { 20, 254, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4061] + 12] = { 20, 254, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4063] + 18] = { 20, 254, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4081] + 7] = { 20, 254, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4083] + 11] = { 20, 254, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4085] + 11] = { 20, 254, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4087] + 18] = { 20, 254, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4089] + 10] = { 20, 254, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4091] + 18] = { 20, 254, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4093] + 18] = { 20, 254, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4095] + 33] = { 20, 254, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 254, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  // Processing non-manifold case 21 with 1 voxel cases
  metadata[offsets[255] + 6] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 255, 0, 0, 0 } } };
  metadata[offsets[443] + 4] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 443, 0, 0, 0 } } };
  metadata[offsets[447] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 447, 0, 0, 0 } } };
  metadata[offsets[494] + 4] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 494, 0, 0, 0 } } };
  metadata[offsets[495] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 495, 0, 0, 0 } } };
  metadata[offsets[507] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 507, 0, 0, 0 } } };
  metadata[offsets[510] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 510, 0, 0, 0 } } };
  metadata[offsets[511] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 511, 0, 0, 0 } } };
  metadata[offsets[635] + 4] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 635, 0, 0, 0 } } };
  metadata[offsets[639] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 639, 0, 0, 0 } } };
  metadata[offsets[734] + 4] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 734, 0, 0, 0 } } };
  metadata[offsets[735] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 735, 0, 0, 0 } } };
  metadata[offsets[763] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 763, 0, 0, 0 } } };
  metadata[offsets[766] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 766, 0, 0, 0 } } };
  metadata[offsets[767] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 767, 0, 0, 0 } } };
  metadata[offsets[831] + 6] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 831, 0, 0, 0 } } };
  metadata[offsets[869] + 4] = { 21, 255, false, { { 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 869, 0, 0, 0 } } };
  metadata[offsets[879] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 879, 0, 0, 0 } } };
  metadata[offsets[885] + 2] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 885, 0, 0, 0 } } };
  metadata[offsets[891] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 891, 0, 0, 0 } } };
  metadata[offsets[895] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 895, 0, 0, 0 } } };
  metadata[offsets[917] + 4] = { 21, 255, false, { { 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 917, 0, 0, 0 } } };
  metadata[offsets[927] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 927, 0, 0, 0 } } };
  metadata[offsets[949] + 2] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 949, 0, 0, 0 } } };
  metadata[offsets[955] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 955, 0, 0, 0 } } };
  metadata[offsets[959] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 959, 0, 0, 0 } } };
  metadata[offsets[975] + 6] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 975, 0, 0, 0 } } };
  metadata[offsets[981] + 2] = { 21, 255, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 981, 0, 0, 0 } } };
  metadata[offsets[990] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 990, 0, 0, 0 } } };
  metadata[offsets[991] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 991, 0, 0, 0 } } };
  metadata[offsets[997] + 2] = { 21, 255, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 997, 0, 0, 0 } } };
  metadata[offsets[1006] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 1006, 0, 0, 0 } } };
  metadata[offsets[1007] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 1007, 0, 0, 0 } } };
  metadata[offsets[1013] + 7] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 1013, 0, 0, 0 } } };
  metadata[offsets[1019] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 1019, 0, 0, 0 } } };
  metadata[offsets[1022] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 1022, 0, 0, 0 } } };
  metadata[offsets[1023] + 12] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1 } }, { { 255, 0, 0, 0 } }, { { 1023, 0, 0, 0 } } };
  metadata[offsets[1207] + 4] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1207, 0, 0, 0 } } };
  metadata[offsets[1215] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1215, 0, 0, 0 } } };
  metadata[offsets[1261] + 4] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1261, 0, 0, 0 } } };
  metadata[offsets[1263] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1263, 0, 0, 0 } } };
  metadata[offsets[1271] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1271, 0, 0, 0 } } };
  metadata[offsets[1277] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1277, 0, 0, 0 } } };
  metadata[offsets[1279] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1279, 0, 0, 0 } } };
  metadata[offsets[1366] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1366, 0, 0, 0 } } };
  metadata[offsets[1367] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1367, 0, 0, 0 } } };
  metadata[offsets[1369] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1369, 0, 0, 0 } } };
  metadata[offsets[1371] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1371, 0, 0, 0 } } };
  metadata[offsets[1373] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1373, 0, 0, 0 } } };
  metadata[offsets[1374] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1374, 0, 0, 0 } } };
  metadata[offsets[1375] + 7] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1375, 0, 0, 0 } } };
  metadata[offsets[1463] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1463, 0, 0, 0 } } };
  metadata[offsets[1467] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1467, 0, 0, 0 } } };
  metadata[offsets[1471] + 7] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1471, 0, 0, 0 } } };
  metadata[offsets[1517] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1517, 0, 0, 0 } } };
  metadata[offsets[1518] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1518, 0, 0, 0 } } };
  metadata[offsets[1519] + 7] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1519, 0, 0, 0 } } };
  metadata[offsets[1523] + 6] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1523, 0, 0, 0 } } };
  metadata[offsets[1526] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1526, 0, 0, 0 } } };
  metadata[offsets[1527] + 8] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1527, 0, 0, 0 } } };
  metadata[offsets[1529] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1529, 0, 0, 0 } } };
  metadata[offsets[1531] + 8] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1531, 0, 0, 0 } } };
  metadata[offsets[1532] + 6] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1532, 0, 0, 0 } } };
  metadata[offsets[1533] + 8] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1533, 0, 0, 0 } } };
  metadata[offsets[1534] + 8] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1534, 0, 0, 0 } } };
  metadata[offsets[1535] + 12] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1535, 0, 0, 0 } } };
  metadata[offsets[1587] + 4] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1587, 0, 0, 0 } } };
  metadata[offsets[1599] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1599, 0, 0, 0 } } };
  metadata[offsets[1641] + 4] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, 0, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1641, 0, 0, 0 } } };
  metadata[offsets[1647] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1647, 0, 0, 0 } } };
  metadata[offsets[1655] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1655, 0, 0, 0 } } };
  metadata[offsets[1659] + 6] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1659, 0, 0, 0 } } };
  metadata[offsets[1661] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1661, 0, 0, 0 } } };
  metadata[offsets[1663] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1663, 0, 0, 0 } } };
  metadata[offsets[1686] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1686, 0, 0, 0 } } };
  metadata[offsets[1695] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1695, 0, 0, 0 } } };
  metadata[offsets[1719] + 6] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1719, 0, 0, 0 } } };
  metadata[offsets[1723] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1723, 0, 0, 0 } } };
  metadata[offsets[1726] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1726, 0, 0, 0 } } };
  metadata[offsets[1727] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1727, 0, 0, 0 } } };
  metadata[offsets[1740] + 4] = { 21, 255, false, { { -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1740, 0, 0, 0 } } };
  metadata[offsets[1743] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1743, 0, 0, 0 } } };
  metadata[offsets[1751] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1751, 0, 0, 0 } } };
  metadata[offsets[1757] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1757, 0, 0, 0 } } };
  metadata[offsets[1758] + 6] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1758, 0, 0, 0 } } };
  metadata[offsets[1759] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1759, 0, 0, 0 } } };
  metadata[offsets[1771] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1771, 0, 0, 0 } } };
  metadata[offsets[1773] + 6] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1773, 0, 0, 0 } } };
  metadata[offsets[1774] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1774, 0, 0, 0 } } };
  metadata[offsets[1775] + 4] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1775, 0, 0, 0 } } };
  metadata[offsets[1779] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1779, 0, 0, 0 } } };
  metadata[offsets[1782] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1782, 0, 0, 0 } } };
  metadata[offsets[1783] + 4] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1783, 0, 0, 0 } } };
  metadata[offsets[1785] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1785, 0, 0, 0 } } };
  metadata[offsets[1787] + 4] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1787, 0, 0, 0 } } };
  metadata[offsets[1788] + 2] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1788, 0, 0, 0 } } };
  metadata[offsets[1789] + 4] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1789, 0, 0, 0 } } };
  metadata[offsets[1790] + 4] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1790, 0, 0, 0 } } };
  metadata[offsets[1791] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1791, 0, 0, 0 } } };
  metadata[offsets[1837] + 4] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1837, 0, 0, 0 } } };
  metadata[offsets[1839] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1839, 0, 0, 0 } } };
  metadata[offsets[1843] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1843, 0, 0, 0 } } };
  metadata[offsets[1853] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1853, 0, 0, 0 } } };
  metadata[offsets[1855] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1855, 0, 0, 0 } } };
  metadata[offsets[1895] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1895, 0, 0, 0 } } };
  metadata[offsets[1899] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1899, 0, 0, 0 } } };
  metadata[offsets[1901] + 6] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1901, 0, 0, 0 } } };
  metadata[offsets[1903] + 4] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1903, 0, 0, 0 } } };
  metadata[offsets[1910] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1910, 0, 0, 0 } } };
  metadata[offsets[1911] + 10] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1911, 0, 0, 0 } } };
  metadata[offsets[1912] + 4] = { 21, 255, false, { { -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1912, 0, 0, 0 } } };
  metadata[offsets[1913] + 6] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1913, 0, 0, 0 } } };
  metadata[offsets[1914] + 2] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1914, 0, 0, 0 } } };
  metadata[offsets[1915] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1915, 0, 0, 0 } } };
  metadata[offsets[1916] + 2] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1916, 0, 0, 0 } } };
  metadata[offsets[1917] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1917, 0, 0, 0 } } };
  metadata[offsets[1918] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1918, 0, 0, 0 } } };
  metadata[offsets[1919] + 13] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1919, 0, 0, 0 } } };
  metadata[offsets[1927] + 4] = { 21, 255, false, { { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1927, 0, 0, 0 } } };
  metadata[offsets[1935] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1935, 0, 0, 0 } } };
  metadata[offsets[1943] + 6] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1943, 0, 0, 0 } } };
  metadata[offsets[1949] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1949, 0, 0, 0 } } };
  metadata[offsets[1950] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1950, 0, 0, 0 } } };
  metadata[offsets[1951] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1951, 0, 0, 0 } } };
  metadata[offsets[1959] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1959, 0, 0, 0 } } };
  metadata[offsets[1965] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1965, 0, 0, 0 } } };
  metadata[offsets[1967] + 7] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1967, 0, 0, 0 } } };
  metadata[offsets[1974] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1974, 0, 0, 0 } } };
  metadata[offsets[1975] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1975, 0, 0, 0 } } };
  metadata[offsets[1979] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1979, 0, 0, 0 } } };
  metadata[offsets[1981] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1981, 0, 0, 0 } } };
  metadata[offsets[1982] + 3] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1982, 0, 0, 0 } } };
  metadata[offsets[1983] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1983, 0, 0, 0 } } };
  metadata[offsets[1991] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1991, 0, 0, 0 } } };
  metadata[offsets[1996] + 2] = { 21, 255, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1996, 0, 0, 0 } } };
  metadata[offsets[1999] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 1999, 0, 0, 0 } } };
  metadata[offsets[2002] + 4] = { 21, 255, false, { { -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2002, 0, 0, 0 } } };
  metadata[offsets[2003] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2003, 0, 0, 0 } } };
  metadata[offsets[2006] + 6] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2006, 0, 0, 0 } } };
  metadata[offsets[2007] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2007, 0, 0, 0 } } };
  metadata[offsets[2009] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2009, 0, 0, 0 } } };
  metadata[offsets[2010] + 2] = { 21, 255, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2010, 0, 0, 0 } } };
  metadata[offsets[2011] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2011, 0, 0, 0 } } };
  metadata[offsets[2013] + 10] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2013, 0, 0, 0 } } };
  metadata[offsets[2014] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2014, 0, 0, 0 } } };
  metadata[offsets[2015] + 13] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2015, 0, 0, 0 } } };
  metadata[offsets[2023] + 7] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2023, 0, 0, 0 } } };
  metadata[offsets[2025] + 2] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2025, 0, 0, 0 } } };
  metadata[offsets[2027] + 3] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2027, 0, 0, 0 } } };
  metadata[offsets[2029] + 7] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2029, 0, 0, 0 } } };
  metadata[offsets[2030] + 7] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2030, 0, 0, 0 } } };
  metadata[offsets[2031] + 11] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2031, 0, 0, 0 } } };
  metadata[offsets[2034] + 2] = { 21, 255, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2034, 0, 0, 0 } } };
  metadata[offsets[2035] + 8] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2035, 0, 0, 0 } } };
  metadata[offsets[2038] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2038, 0, 0, 0 } } };
  metadata[offsets[2039] + 13] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2039, 0, 0, 0 } } };
  metadata[offsets[2040] + 2] = { 21, 255, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2040, 0, 0, 0 } } };
  metadata[offsets[2041] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2041, 0, 0, 0 } } };
  metadata[offsets[2042] + 7] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2042, 0, 0, 0 } } };
  metadata[offsets[2043] + 11] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2043, 0, 0, 0 } } };
  metadata[offsets[2044] + 8] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2044, 0, 0, 0 } } };
  metadata[offsets[2045] + 13] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2045, 0, 0, 0 } } };
  metadata[offsets[2046] + 11] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2046, 0, 0, 0 } } };
  metadata[offsets[2047] + 19] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1 } }, { { 255, 0, 0, 0 } }, { { 2047, 0, 0, 0 } } };
  metadata[offsets[2167] + 4] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2167, 0, 0, 0 } } };
  metadata[offsets[2175] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2175, 0, 0, 0 } } };
  metadata[offsets[2269] + 4] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2269, 0, 0, 0 } } };
  metadata[offsets[2271] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2271, 0, 0, 0 } } };
  metadata[offsets[2295] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2295, 0, 0, 0 } } };
  metadata[offsets[2301] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2301, 0, 0, 0 } } };
  metadata[offsets[2303] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2303, 0, 0, 0 } } };
  metadata[offsets[2355] + 4] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2355, 0, 0, 0 } } };
  metadata[offsets[2367] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2367, 0, 0, 0 } } };
  metadata[offsets[2406] + 4] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, 0, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2406, 0, 0, 0 } } };
  metadata[offsets[2415] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2415, 0, 0, 0 } } };
  metadata[offsets[2423] + 6] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2423, 0, 0, 0 } } };
  metadata[offsets[2427] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2427, 0, 0, 0 } } };
  metadata[offsets[2430] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2430, 0, 0, 0 } } };
  metadata[offsets[2431] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2431, 0, 0, 0 } } };
  metadata[offsets[2457] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2457, 0, 0, 0 } } };
  metadata[offsets[2463] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2463, 0, 0, 0 } } };
  metadata[offsets[2487] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2487, 0, 0, 0 } } };
  metadata[offsets[2491] + 6] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2491, 0, 0, 0 } } };
  metadata[offsets[2493] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2493, 0, 0, 0 } } };
  metadata[offsets[2495] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2495, 0, 0, 0 } } };
  metadata[offsets[2508] + 4] = { 21, 255, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2508, 0, 0, 0 } } };
  metadata[offsets[2511] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2511, 0, 0, 0 } } };
  metadata[offsets[2523] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2523, 0, 0, 0 } } };
  metadata[offsets[2525] + 6] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2525, 0, 0, 0 } } };
  metadata[offsets[2526] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2526, 0, 0, 0 } } };
  metadata[offsets[2527] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2527, 0, 0, 0 } } };
  metadata[offsets[2535] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2535, 0, 0, 0 } } };
  metadata[offsets[2541] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2541, 0, 0, 0 } } };
  metadata[offsets[2542] + 6] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2542, 0, 0, 0 } } };
  metadata[offsets[2543] + 4] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2543, 0, 0, 0 } } };
  metadata[offsets[2547] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2547, 0, 0, 0 } } };
  metadata[offsets[2550] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2550, 0, 0, 0 } } };
  metadata[offsets[2551] + 4] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2551, 0, 0, 0 } } };
  metadata[offsets[2553] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2553, 0, 0, 0 } } };
  metadata[offsets[2555] + 4] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2555, 0, 0, 0 } } };
  metadata[offsets[2556] + 2] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2556, 0, 0, 0 } } };
  metadata[offsets[2557] + 4] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2557, 0, 0, 0 } } };
  metadata[offsets[2558] + 4] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2558, 0, 0, 0 } } };
  metadata[offsets[2559] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2559, 0, 0, 0 } } };
  metadata[offsets[2679] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2679, 0, 0, 0 } } };
  metadata[offsets[2683] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2683, 0, 0, 0 } } };
  metadata[offsets[2687] + 7] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2687, 0, 0, 0 } } };
  metadata[offsets[2726] + 4] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2726, 0, 0, 0 } } };
  metadata[offsets[2727] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2727, 0, 0, 0 } } };
  metadata[offsets[2729] + 4] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2729, 0, 0, 0 } } };
  metadata[offsets[2731] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2731, 0, 0, 0 } } };
  metadata[offsets[2733] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2733, 0, 0, 0 } } };
  metadata[offsets[2734] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2734, 0, 0, 0 } } };
  metadata[offsets[2735] + 7] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, -1, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2735, 0, 0, 0 } } };
  metadata[offsets[2781] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2781, 0, 0, 0 } } };
  metadata[offsets[2782] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2782, 0, 0, 0 } } };
  metadata[offsets[2783] + 7] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2783, 0, 0, 0 } } };
  metadata[offsets[2803] + 6] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2803, 0, 0, 0 } } };
  metadata[offsets[2806] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2806, 0, 0, 0 } } };
  metadata[offsets[2807] + 8] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2807, 0, 0, 0 } } };
  metadata[offsets[2809] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2809, 0, 0, 0 } } };
  metadata[offsets[2811] + 8] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2811, 0, 0, 0 } } };
  metadata[offsets[2812] + 6] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2812, 0, 0, 0 } } };
  metadata[offsets[2813] + 8] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2813, 0, 0, 0 } } };
  metadata[offsets[2814] + 8] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2814, 0, 0, 0 } } };
  metadata[offsets[2815] + 12] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2815, 0, 0, 0 } } };
  metadata[offsets[2845] + 4] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2845, 0, 0, 0 } } };
  metadata[offsets[2847] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2847, 0, 0, 0 } } };
  metadata[offsets[2867] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2867, 0, 0, 0 } } };
  metadata[offsets[2877] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2877, 0, 0, 0 } } };
  metadata[offsets[2879] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2879, 0, 0, 0 } } };
  metadata[offsets[2887] + 4] = { 21, 255, false, { { 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2887, 0, 0, 0 } } };
  metadata[offsets[2895] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2895, 0, 0, 0 } } };
  metadata[offsets[2903] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2903, 0, 0, 0 } } };
  metadata[offsets[2909] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2909, 0, 0, 0 } } };
  metadata[offsets[2911] + 7] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2911, 0, 0, 0 } } };
  metadata[offsets[2919] + 6] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2919, 0, 0, 0 } } };
  metadata[offsets[2925] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2925, 0, 0, 0 } } };
  metadata[offsets[2926] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2926, 0, 0, 0 } } };
  metadata[offsets[2927] + 4] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2927, 0, 0, 0 } } };
  metadata[offsets[2934] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2934, 0, 0, 0 } } };
  metadata[offsets[2935] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2935, 0, 0, 0 } } };
  metadata[offsets[2939] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2939, 0, 0, 0 } } };
  metadata[offsets[2941] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2941, 0, 0, 0 } } };
  metadata[offsets[2942] + 3] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2942, 0, 0, 0 } } };
  metadata[offsets[2943] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2943, 0, 0, 0 } } };
  metadata[offsets[2967] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2967, 0, 0, 0 } } };
  metadata[offsets[2971] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2971, 0, 0, 0 } } };
  metadata[offsets[2973] + 6] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2973, 0, 0, 0 } } };
  metadata[offsets[2975] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2975, 0, 0, 0 } } };
  metadata[offsets[2998] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2998, 0, 0, 0 } } };
  metadata[offsets[2999] + 10] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 2999, 0, 0, 0 } } };
  metadata[offsets[3000] + 4] = { 21, 255, false, { { -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3000, 0, 0, 0 } } };
  metadata[offsets[3001] + 6] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3001, 0, 0, 0 } } };
  metadata[offsets[3002] + 2] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3002, 0, 0, 0 } } };
  metadata[offsets[3003] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3003, 0, 0, 0 } } };
  metadata[offsets[3004] + 2] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3004, 0, 0, 0 } } };
  metadata[offsets[3005] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3005, 0, 0, 0 } } };
  metadata[offsets[3006] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3006, 0, 0, 0 } } };
  metadata[offsets[3007] + 13] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3007, 0, 0, 0 } } };
  metadata[offsets[3015] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3015, 0, 0, 0 } } };
  metadata[offsets[3020] + 2] = { 21, 255, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3020, 0, 0, 0 } } };
  metadata[offsets[3023] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3023, 0, 0, 0 } } };
  metadata[offsets[3031] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3031, 0, 0, 0 } } };
  metadata[offsets[3033] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3033, 0, 0, 0 } } };
  metadata[offsets[3035] + 3] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3035, 0, 0, 0 } } };
  metadata[offsets[3037] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3037, 0, 0, 0 } } };
  metadata[offsets[3038] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3038, 0, 0, 0 } } };
  metadata[offsets[3039] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3039, 0, 0, 0 } } };
  metadata[offsets[3042] + 4] = { 21, 255, false, { { -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3042, 0, 0, 0 } } };
  metadata[offsets[3043] + 2] = { 21, 255, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3043, 0, 0, 0 } } };
  metadata[offsets[3046] + 6] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3046, 0, 0, 0 } } };
  metadata[offsets[3047] + 7] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3047, 0, 0, 0 } } };
  metadata[offsets[3049] + 2] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3049, 0, 0, 0 } } };
  metadata[offsets[3050] + 2] = { 21, 255, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3050, 0, 0, 0 } } };
  metadata[offsets[3051] + 7] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3051, 0, 0, 0 } } };
  metadata[offsets[3053] + 10] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3053, 0, 0, 0 } } };
  metadata[offsets[3054] + 7] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3054, 0, 0, 0 } } };
  metadata[offsets[3055] + 13] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3055, 0, 0, 0 } } };
  metadata[offsets[3058] + 2] = { 21, 255, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3058, 0, 0, 0 } } };
  metadata[offsets[3059] + 8] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3059, 0, 0, 0 } } };
  metadata[offsets[3062] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3062, 0, 0, 0 } } };
  metadata[offsets[3063] + 13] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3063, 0, 0, 0 } } };
  metadata[offsets[3064] + 2] = { 21, 255, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3064, 0, 0, 0 } } };
  metadata[offsets[3065] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3065, 0, 0, 0 } } };
  metadata[offsets[3066] + 7] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3066, 0, 0, 0 } } };
  metadata[offsets[3067] + 11] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3067, 0, 0, 0 } } };
  metadata[offsets[3068] + 8] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3068, 0, 0, 0 } } };
  metadata[offsets[3069] + 13] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3069, 0, 0, 0 } } };
  metadata[offsets[3070] + 11] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3070, 0, 0, 0 } } };
  metadata[offsets[3071] + 19] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0 } }, { { 255, 0, 0, 0 } }, { { 3071, 0, 0, 0 } } };
  metadata[offsets[3135] + 6] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3135, 0, 0, 0 } } };
  metadata[offsets[3178] + 4] = { 21, 255, false, { { -1, 0, -1, 0, -1, 0, 0, -1, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3178, 0, 0, 0 } } };
  metadata[offsets[3183] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3183, 0, 0, 0 } } };
  metadata[offsets[3191] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3191, 0, 0, 0 } } };
  metadata[offsets[3194] + 2] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3194, 0, 0, 0 } } };
  metadata[offsets[3199] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3199, 0, 0, 0 } } };
  metadata[offsets[3226] + 4] = { 21, 255, false, { { -1, 0, -1, 0, 0, -1, -1, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3226, 0, 0, 0 } } };
  metadata[offsets[3231] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3231, 0, 0, 0 } } };
  metadata[offsets[3255] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3255, 0, 0, 0 } } };
  metadata[offsets[3258] + 2] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3258, 0, 0, 0 } } };
  metadata[offsets[3263] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3263, 0, 0, 0 } } };
  metadata[offsets[3279] + 6] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3279, 0, 0, 0 } } };
  metadata[offsets[3290] + 2] = { 21, 255, false, { { -1, 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3290, 0, 0, 0 } } };
  metadata[offsets[3293] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3293, 0, 0, 0 } } };
  metadata[offsets[3295] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3295, 0, 0, 0 } } };
  metadata[offsets[3306] + 2] = { 21, 255, false, { { -1, 0, -1, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3306, 0, 0, 0 } } };
  metadata[offsets[3309] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3309, 0, 0, 0 } } };
  metadata[offsets[3311] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3311, 0, 0, 0 } } };
  metadata[offsets[3319] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3319, 0, 0, 0 } } };
  metadata[offsets[3322] + 7] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3322, 0, 0, 0 } } };
  metadata[offsets[3325] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3325, 0, 0, 0 } } };
  metadata[offsets[3327] + 12] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3327, 0, 0, 0 } } };
  metadata[offsets[3374] + 4] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3374, 0, 0, 0 } } };
  metadata[offsets[3375] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3375, 0, 0, 0 } } };
  metadata[offsets[3379] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3379, 0, 0, 0 } } };
  metadata[offsets[3390] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3390, 0, 0, 0 } } };
  metadata[offsets[3391] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3391, 0, 0, 0 } } };
  metadata[offsets[3431] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3431, 0, 0, 0 } } };
  metadata[offsets[3435] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3435, 0, 0, 0 } } };
  metadata[offsets[3438] + 6] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3438, 0, 0, 0 } } };
  metadata[offsets[3439] + 4] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3439, 0, 0, 0 } } };
  metadata[offsets[3444] + 4] = { 21, 255, false, { { -1, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3444, 0, 0, 0 } } };
  metadata[offsets[3445] + 2] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3445, 0, 0, 0 } } };
  metadata[offsets[3446] + 6] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3446, 0, 0, 0 } } };
  metadata[offsets[3447] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3447, 0, 0, 0 } } };
  metadata[offsets[3449] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3449, 0, 0, 0 } } };
  metadata[offsets[3451] + 10] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3451, 0, 0, 0 } } };
  metadata[offsets[3452] + 2] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3452, 0, 0, 0 } } };
  metadata[offsets[3453] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3453, 0, 0, 0 } } };
  metadata[offsets[3454] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3454, 0, 0, 0 } } };
  metadata[offsets[3455] + 13] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3455, 0, 0, 0 } } };
  metadata[offsets[3467] + 4] = { 21, 255, false, { { 0, 0, -1, 0, -1, -1, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3467, 0, 0, 0 } } };
  metadata[offsets[3471] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3471, 0, 0, 0 } } };
  metadata[offsets[3483] + 6] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3483, 0, 0, 0 } } };
  metadata[offsets[3485] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3485, 0, 0, 0 } } };
  metadata[offsets[3486] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3486, 0, 0, 0 } } };
  metadata[offsets[3487] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3487, 0, 0, 0 } } };
  metadata[offsets[3499] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3499, 0, 0, 0 } } };
  metadata[offsets[3502] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3502, 0, 0, 0 } } };
  metadata[offsets[3503] + 7] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3503, 0, 0, 0 } } };
  metadata[offsets[3511] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3511, 0, 0, 0 } } };
  metadata[offsets[3513] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3513, 0, 0, 0 } } };
  metadata[offsets[3515] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3515, 0, 0, 0 } } };
  metadata[offsets[3517] + 3] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3517, 0, 0, 0 } } };
  metadata[offsets[3518] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3518, 0, 0, 0 } } };
  metadata[offsets[3519] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3519, 0, 0, 0 } } };
  metadata[offsets[3531] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3531, 0, 0, 0 } } };
  metadata[offsets[3532] + 2] = { 21, 255, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3532, 0, 0, 0 } } };
  metadata[offsets[3535] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3535, 0, 0, 0 } } };
  metadata[offsets[3537] + 4] = { 21, 255, false, { { 0, -1, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3537, 0, 0, 0 } } };
  metadata[offsets[3539] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3539, 0, 0, 0 } } };
  metadata[offsets[3541] + 2] = { 21, 255, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3541, 0, 0, 0 } } };
  metadata[offsets[3542] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3542, 0, 0, 0 } } };
  metadata[offsets[3543] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3543, 0, 0, 0 } } };
  metadata[offsets[3545] + 6] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3545, 0, 0, 0 } } };
  metadata[offsets[3547] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3547, 0, 0, 0 } } };
  metadata[offsets[3549] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3549, 0, 0, 0 } } };
  metadata[offsets[3550] + 10] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3550, 0, 0, 0 } } };
  metadata[offsets[3551] + 13] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3551, 0, 0, 0 } } };
  metadata[offsets[3558] + 2] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3558, 0, 0, 0 } } };
  metadata[offsets[3559] + 3] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3559, 0, 0, 0 } } };
  metadata[offsets[3563] + 7] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3563, 0, 0, 0 } } };
  metadata[offsets[3565] + 7] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3565, 0, 0, 0 } } };
  metadata[offsets[3566] + 7] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3566, 0, 0, 0 } } };
  metadata[offsets[3567] + 11] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3567, 0, 0, 0 } } };
  metadata[offsets[3569] + 2] = { 21, 255, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3569, 0, 0, 0 } } };
  metadata[offsets[3571] + 8] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3571, 0, 0, 0 } } };
  metadata[offsets[3572] + 2] = { 21, 255, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3572, 0, 0, 0 } } };
  metadata[offsets[3573] + 7] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3573, 0, 0, 0 } } };
  metadata[offsets[3574] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3574, 0, 0, 0 } } };
  metadata[offsets[3575] + 11] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3575, 0, 0, 0 } } };
  metadata[offsets[3577] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3577, 0, 0, 0 } } };
  metadata[offsets[3579] + 13] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3579, 0, 0, 0 } } };
  metadata[offsets[3580] + 8] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3580, 0, 0, 0 } } };
  metadata[offsets[3581] + 11] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3581, 0, 0, 0 } } };
  metadata[offsets[3582] + 13] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3582, 0, 0, 0 } } };
  metadata[offsets[3583] + 19] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3583, 0, 0, 0 } } };
  metadata[offsets[3614] + 4] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3614, 0, 0, 0 } } };
  metadata[offsets[3615] + 2] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3615, 0, 0, 0 } } };
  metadata[offsets[3635] + 2] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3635, 0, 0, 0 } } };
  metadata[offsets[3646] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3646, 0, 0, 0 } } };
  metadata[offsets[3647] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3647, 0, 0, 0 } } };
  metadata[offsets[3659] + 4] = { 21, 255, false, { { 0, 0, -1, 0, -1, -1, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3659, 0, 0, 0 } } };
  metadata[offsets[3663] + 2] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3663, 0, 0, 0 } } };
  metadata[offsets[3675] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3675, 0, 0, 0 } } };
  metadata[offsets[3678] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3678, 0, 0, 0 } } };
  metadata[offsets[3679] + 7] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3679, 0, 0, 0 } } };
  metadata[offsets[3691] + 6] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3691, 0, 0, 0 } } };
  metadata[offsets[3693] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3693, 0, 0, 0 } } };
  metadata[offsets[3694] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3694, 0, 0, 0 } } };
  metadata[offsets[3695] + 4] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3695, 0, 0, 0 } } };
  metadata[offsets[3703] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3703, 0, 0, 0 } } };
  metadata[offsets[3705] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3705, 0, 0, 0 } } };
  metadata[offsets[3707] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3707, 0, 0, 0 } } };
  metadata[offsets[3709] + 3] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3709, 0, 0, 0 } } };
  metadata[offsets[3710] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3710, 0, 0, 0 } } };
  metadata[offsets[3711] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3711, 0, 0, 0 } } };
  metadata[offsets[3735] + 2] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3735, 0, 0, 0 } } };
  metadata[offsets[3739] + 2] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3739, 0, 0, 0 } } };
  metadata[offsets[3742] + 6] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3742, 0, 0, 0 } } };
  metadata[offsets[3743] + 4] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3743, 0, 0, 0 } } };
  metadata[offsets[3764] + 4] = { 21, 255, false, { { -1, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3764, 0, 0, 0 } } };
  metadata[offsets[3765] + 2] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3765, 0, 0, 0 } } };
  metadata[offsets[3766] + 6] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3766, 0, 0, 0 } } };
  metadata[offsets[3767] + 7] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3767, 0, 0, 0 } } };
  metadata[offsets[3769] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3769, 0, 0, 0 } } };
  metadata[offsets[3771] + 10] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3771, 0, 0, 0 } } };
  metadata[offsets[3772] + 2] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3772, 0, 0, 0 } } };
  metadata[offsets[3773] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3773, 0, 0, 0 } } };
  metadata[offsets[3774] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3774, 0, 0, 0 } } };
  metadata[offsets[3775] + 13] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3775, 0, 0, 0 } } };
  metadata[offsets[3787] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3787, 0, 0, 0 } } };
  metadata[offsets[3788] + 2] = { 21, 255, false, { { -1, -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3788, 0, 0, 0 } } };
  metadata[offsets[3791] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3791, 0, 0, 0 } } };
  metadata[offsets[3798] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3798, 0, 0, 0 } } };
  metadata[offsets[3799] + 3] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3799, 0, 0, 0 } } };
  metadata[offsets[3803] + 7] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3803, 0, 0, 0 } } };
  metadata[offsets[3805] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3805, 0, 0, 0 } } };
  metadata[offsets[3806] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3806, 0, 0, 0 } } };
  metadata[offsets[3807] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3807, 0, 0, 0 } } };
  metadata[offsets[3809] + 4] = { 21, 255, false, { { 0, -1, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3809, 0, 0, 0 } } };
  metadata[offsets[3811] + 2] = { 21, 255, false, { { 0, 0, -1, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3811, 0, 0, 0 } } };
  metadata[offsets[3813] + 2] = { 21, 255, false, { { 0, -1, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3813, 0, 0, 0 } } };
  metadata[offsets[3814] + 2] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3814, 0, 0, 0 } } };
  metadata[offsets[3815] + 7] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3815, 0, 0, 0 } } };
  metadata[offsets[3817] + 6] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3817, 0, 0, 0 } } };
  metadata[offsets[3819] + 7] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3819, 0, 0, 0 } } };
  metadata[offsets[3821] + 7] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3821, 0, 0, 0 } } };
  metadata[offsets[3822] + 10] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3822, 0, 0, 0 } } };
  metadata[offsets[3823] + 13] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3823, 0, 0, 0 } } };
  metadata[offsets[3825] + 2] = { 21, 255, false, { { 0, -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3825, 0, 0, 0 } } };
  metadata[offsets[3827] + 8] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3827, 0, 0, 0 } } };
  metadata[offsets[3828] + 2] = { 21, 255, false, { { -1, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3828, 0, 0, 0 } } };
  metadata[offsets[3829] + 7] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3829, 0, 0, 0 } } };
  metadata[offsets[3830] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3830, 0, 0, 0 } } };
  metadata[offsets[3831] + 11] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3831, 0, 0, 0 } } };
  metadata[offsets[3833] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3833, 0, 0, 0 } } };
  metadata[offsets[3835] + 13] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3835, 0, 0, 0 } } };
  metadata[offsets[3836] + 8] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3836, 0, 0, 0 } } };
  metadata[offsets[3837] + 11] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3837, 0, 0, 0 } } };
  metadata[offsets[3838] + 13] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3838, 0, 0, 0 } } };
  metadata[offsets[3839] + 19] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3839, 0, 0, 0 } } };
  metadata[offsets[3855] + 6] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3855, 0, 0, 0 } } };
  metadata[offsets[3869] + 2] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3869, 0, 0, 0 } } };
  metadata[offsets[3870] + 2] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3870, 0, 0, 0 } } };
  metadata[offsets[3871] + 8] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3871, 0, 0, 0 } } };
  metadata[offsets[3885] + 2] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3885, 0, 0, 0 } } };
  metadata[offsets[3886] + 2] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3886, 0, 0, 0 } } };
  metadata[offsets[3887] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3887, 0, 0, 0 } } };
  metadata[offsets[3891] + 7] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3891, 0, 0, 0 } } };
  metadata[offsets[3901] + 7] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3901, 0, 0, 0 } } };
  metadata[offsets[3902] + 7] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3902, 0, 0, 0 } } };
  metadata[offsets[3903] + 12] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3903, 0, 0, 0 } } };
  metadata[offsets[3911] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3911, 0, 0, 0 } } };
  metadata[offsets[3915] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3915, 0, 0, 0 } } };
  metadata[offsets[3919] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3919, 0, 0, 0 } } };
  metadata[offsets[3925] + 6] = { 21, 255, false, { { 0, -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3925, 0, 0, 0 } } };
  metadata[offsets[3926] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3926, 0, 0, 0 } } };
  metadata[offsets[3927] + 8] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3927, 0, 0, 0 } } };
  metadata[offsets[3929] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3929, 0, 0, 0 } } };
  metadata[offsets[3930] + 6] = { 21, 255, false, { { -1, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3930, 0, 0, 0 } } };
  metadata[offsets[3931] + 8] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3931, 0, 0, 0 } } };
  metadata[offsets[3933] + 8] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3933, 0, 0, 0 } } };
  metadata[offsets[3934] + 8] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3934, 0, 0, 0 } } };
  metadata[offsets[3935] + 12] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3935, 0, 0, 0 } } };
  metadata[offsets[3941] + 2] = { 21, 255, false, { { 0, -1, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3941, 0, 0, 0 } } };
  metadata[offsets[3942] + 2] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3942, 0, 0, 0 } } };
  metadata[offsets[3943] + 4] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3943, 0, 0, 0 } } };
  metadata[offsets[3945] + 2] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3945, 0, 0, 0 } } };
  metadata[offsets[3946] + 2] = { 21, 255, false, { { -1, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3946, 0, 0, 0 } } };
  metadata[offsets[3947] + 4] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3947, 0, 0, 0 } } };
  metadata[offsets[3949] + 4] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3949, 0, 0, 0 } } };
  metadata[offsets[3950] + 4] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3950, 0, 0, 0 } } };
  metadata[offsets[3951] + 11] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3951, 0, 0, 0 } } };
  metadata[offsets[3956] + 2] = { 21, 255, false, { { -1, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3956, 0, 0, 0 } } };
  metadata[offsets[3957] + 8] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3957, 0, 0, 0 } } };
  metadata[offsets[3958] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3958, 0, 0, 0 } } };
  metadata[offsets[3959] + 13] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3959, 0, 0, 0 } } };
  metadata[offsets[3960] + 2] = { 21, 255, false, { { -1, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3960, 0, 0, 0 } } };
  metadata[offsets[3961] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3961, 0, 0, 0 } } };
  metadata[offsets[3962] + 8] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3962, 0, 0, 0 } } };
  metadata[offsets[3963] + 13] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3963, 0, 0, 0 } } };
  metadata[offsets[3964] + 7] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3964, 0, 0, 0 } } };
  metadata[offsets[3965] + 11] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3965, 0, 0, 0 } } };
  metadata[offsets[3966] + 11] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3966, 0, 0, 0 } } };
  metadata[offsets[3967] + 19] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3967, 0, 0, 0 } } };
  metadata[offsets[3975] + 2] = { 21, 255, false, { { 0, 0, 0, -1, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3975, 0, 0, 0 } } };
  metadata[offsets[3979] + 2] = { 21, 255, false, { { 0, 0, -1, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3979, 0, 0, 0 } } };
  metadata[offsets[3983] + 8] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3983, 0, 0, 0 } } };
  metadata[offsets[3989] + 2] = { 21, 255, false, { { 0, -1, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3989, 0, 0, 0 } } };
  metadata[offsets[3990] + 2] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3990, 0, 0, 0 } } };
  metadata[offsets[3991] + 4] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3991, 0, 0, 0 } } };
  metadata[offsets[3993] + 2] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3993, 0, 0, 0 } } };
  metadata[offsets[3994] + 2] = { 21, 255, false, { { -1, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3994, 0, 0, 0 } } };
  metadata[offsets[3995] + 4] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3995, 0, 0, 0 } } };
  metadata[offsets[3997] + 4] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3997, 0, 0, 0 } } };
  metadata[offsets[3998] + 4] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3998, 0, 0, 0 } } };
  metadata[offsets[3999] + 11] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 3999, 0, 0, 0 } } };
  metadata[offsets[4005] + 6] = { 21, 255, false, { { 0, -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4005, 0, 0, 0 } } };
  metadata[offsets[4006] + 2] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4006, 0, 0, 0 } } };
  metadata[offsets[4007] + 8] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4007, 0, 0, 0 } } };
  metadata[offsets[4009] + 2] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4009, 0, 0, 0 } } };
  metadata[offsets[4010] + 6] = { 21, 255, false, { { -1, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4010, 0, 0, 0 } } };
  metadata[offsets[4011] + 8] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4011, 0, 0, 0 } } };
  metadata[offsets[4013] + 8] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4013, 0, 0, 0 } } };
  metadata[offsets[4014] + 8] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4014, 0, 0, 0 } } };
  metadata[offsets[4015] + 12] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4015, 0, 0, 0 } } };
  metadata[offsets[4020] + 2] = { 21, 255, false, { { -1, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4020, 0, 0, 0 } } };
  metadata[offsets[4021] + 8] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4021, 0, 0, 0 } } };
  metadata[offsets[4022] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4022, 0, 0, 0 } } };
  metadata[offsets[4023] + 13] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4023, 0, 0, 0 } } };
  metadata[offsets[4024] + 2] = { 21, 255, false, { { -1, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4024, 0, 0, 0 } } };
  metadata[offsets[4025] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4025, 0, 0, 0 } } };
  metadata[offsets[4026] + 8] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4026, 0, 0, 0 } } };
  metadata[offsets[4027] + 13] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4027, 0, 0, 0 } } };
  metadata[offsets[4028] + 7] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4028, 0, 0, 0 } } };
  metadata[offsets[4029] + 11] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4029, 0, 0, 0 } } };
  metadata[offsets[4030] + 11] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4030, 0, 0, 0 } } };
  metadata[offsets[4031] + 19] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4031, 0, 0, 0 } } };
  metadata[offsets[4039] + 7] = { 21, 255, false, { { 0, 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4039, 0, 0, 0 } } };
  metadata[offsets[4043] + 7] = { 21, 255, false, { { 0, 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4043, 0, 0, 0 } } };
  metadata[offsets[4044] + 7] = { 21, 255, false, { { -1, -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4044, 0, 0, 0 } } };
  metadata[offsets[4047] + 12] = { 21, 255, false, { { 0, 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4047, 0, 0, 0 } } };
  metadata[offsets[4049] + 2] = { 21, 255, false, { { 0, -1, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4049, 0, 0, 0 } } };
  metadata[offsets[4050] + 2] = { 21, 255, false, { { -1, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4050, 0, 0, 0 } } };
  metadata[offsets[4051] + 7] = { 21, 255, false, { { 0, 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4051, 0, 0, 0 } } };
  metadata[offsets[4053] + 8] = { 21, 255, false, { { 0, -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4053, 0, 0, 0 } } };
  metadata[offsets[4054] + 4] = { 21, 255, false, { { -1, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4054, 0, 0, 0 } } };
  metadata[offsets[4055] + 11] = { 21, 255, false, { { 0, 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4055, 0, 0, 0 } } };
  metadata[offsets[4057] + 4] = { 21, 255, false, { { 0, -1, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4057, 0, 0, 0 } } };
  metadata[offsets[4058] + 8] = { 21, 255, false, { { -1, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4058, 0, 0, 0 } } };
  metadata[offsets[4059] + 11] = { 21, 255, false, { { 0, 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4059, 0, 0, 0 } } };
  metadata[offsets[4061] + 13] = { 21, 255, false, { { 0, -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4061, 0, 0, 0 } } };
  metadata[offsets[4062] + 13] = { 21, 255, false, { { -1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4062, 0, 0, 0 } } };
  metadata[offsets[4063] + 19] = { 21, 255, false, { { 0, 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4063, 0, 0, 0 } } };
  metadata[offsets[4065] + 2] = { 21, 255, false, { { 0, -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4065, 0, 0, 0 } } };
  metadata[offsets[4066] + 2] = { 21, 255, false, { { -1, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4066, 0, 0, 0 } } };
  metadata[offsets[4067] + 7] = { 21, 255, false, { { 0, 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4067, 0, 0, 0 } } };
  metadata[offsets[4069] + 8] = { 21, 255, false, { { 0, -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4069, 0, 0, 0 } } };
  metadata[offsets[4070] + 4] = { 21, 255, false, { { -1, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4070, 0, 0, 0 } } };
  metadata[offsets[4071] + 11] = { 21, 255, false, { { 0, 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4071, 0, 0, 0 } } };
  metadata[offsets[4073] + 4] = { 21, 255, false, { { 0, -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4073, 0, 0, 0 } } };
  metadata[offsets[4074] + 8] = { 21, 255, false, { { -1, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4074, 0, 0, 0 } } };
  metadata[offsets[4075] + 11] = { 21, 255, false, { { 0, 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4075, 0, 0, 0 } } };
  metadata[offsets[4077] + 13] = { 21, 255, false, { { 0, -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4077, 0, 0, 0 } } };
  metadata[offsets[4078] + 13] = { 21, 255, false, { { -1, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4078, 0, 0, 0 } } };
  metadata[offsets[4079] + 19] = { 21, 255, false, { { 0, 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4079, 0, 0, 0 } } };
  metadata[offsets[4080] + 6] = { 21, 255, false, { { -1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4080, 0, 0, 0 } } };
  metadata[offsets[4081] + 8] = { 21, 255, false, { { 0, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4081, 0, 0, 0 } } };
  metadata[offsets[4082] + 8] = { 21, 255, false, { { -1, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4082, 0, 0, 0 } } };
  metadata[offsets[4083] + 12] = { 21, 255, false, { { 0, 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4083, 0, 0, 0 } } };
  metadata[offsets[4084] + 8] = { 21, 255, false, { { -1, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4084, 0, 0, 0 } } };
  metadata[offsets[4085] + 12] = { 21, 255, false, { { 0, -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4085, 0, 0, 0 } } };
  metadata[offsets[4086] + 11] = { 21, 255, false, { { -1, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4086, 0, 0, 0 } } };
  metadata[offsets[4087] + 19] = { 21, 255, false, { { 0, 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4087, 0, 0, 0 } } };
  metadata[offsets[4088] + 8] = { 21, 255, false, { { -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4088, 0, 0, 0 } } };
  metadata[offsets[4089] + 11] = { 21, 255, false, { { 0, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4089, 0, 0, 0 } } };
  metadata[offsets[4090] + 12] = { 21, 255, false, { { -1, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4090, 0, 0, 0 } } };
  metadata[offsets[4091] + 19] = { 21, 255, false, { { 0, 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4091, 0, 0, 0 } } };
  metadata[offsets[4092] + 12] = { 21, 255, false, { { -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4092, 0, 0, 0 } } };
  metadata[offsets[4093] + 19] = { 21, 255, false, { { 0, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4093, 0, 0, 0 } } };
  metadata[offsets[4094] + 19] = { 21, 255, false, { { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4094, 0, 0, 0 } } };
  metadata[offsets[4095] + 34] = { 21, 255, false, { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } }, { { 255, 0, 0, 0 } }, { { 4095, 0, 0, 0 } } };
  // clang-format on
  return { offsets, metadata };
}
constexpr auto Result = CreateNonManifoldMetaDataPerEdgeCase();
constexpr auto& Offsets = Result.first;
constexpr auto& Metadata = Result.second;
#endif
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN

//------------------------------------------------------------------------------
const std::array<uint16_t, 4097>& vtkSurfaceNets3DNonManifoldCases::EdgeCaseOffsets = Offsets;

//------------------------------------------------------------------------------
const std::array<vtkSurfaceNets3DNonManifoldCases::NonManifoldCaseMetadata, 3736>&
  vtkSurfaceNets3DNonManifoldCases::NonManifoldMetadata = Metadata;

//------------------------------------------------------------------------------
template <class T>
auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<T>& voxelNeighborhood) -> int8_t
{
  const EdgeCaseType& edgeCase = voxelNeighborhood.EdgeCase;
  const VoxelCaseType& voxelCase = voxelNeighborhood.VoxelCase;
  const auto& labels = voxelNeighborhood.Labels;
  const int8_t numNonManifoldCases = static_cast<int8_t>(GetNumberOfNonManifoldCases(edgeCase));
  if (numNonManifoldCases == 0)
  {
    return ManifoldIndex;
  }
  // check is voxel case is homogeneous based on its material configuration.
  const bool isVoxelCaseHomogeneous = AreAllMaterialsTheSame(voxelCase, labels);
  // extract the color encoding voxel case by taking only the labels of the active corners in
  // the voxel case.
  const auto colorEncodedVoxelCase = ExtractColorEncodedVoxelCase(labels, voxelCase);
  // and check if it's actually manifold or not based on its material configuration.
  const auto nonManifoldCaseMatches =
    GetNonManifoldCasesForColorEncodedVoxelCase(colorEncodedVoxelCase);
  NonManifoldCaseType subNonManifoldCase = 0;
  VoxelCaseType subVoxelCase = 0;
  EdgeCaseType subEdgeCase = 0;
  if (nonManifoldCaseMatches.GetSize() != 0)
  {
    const auto subVoxelCaseMatch = nonManifoldCaseMatches.GetMaxNonManifoldCaseMatch();
    subVoxelCase = subVoxelCaseMatch.VoxelCase;
    subNonManifoldCase = subVoxelCaseMatch.NonManifoldCase;
    subEdgeCase = ComputeHomogeneousEdgeCase(subVoxelCase);
  }

  // iterate all non-manifold entries for this edge case
  for (int8_t i = 0; i < numNonManifoldCases; ++i)
  {
    const VoxelCaseType basicVoxelCase = GetNonManifoldBasicVoxelCase(edgeCase, i);
    // check if the basic voxel case is the same as the given one
    if (voxelCase == basicVoxelCase)
    {
      const NonManifoldCaseType nonManifoldCase = GetNonManifoldCase(edgeCase, i);
      // if the voxel configuration matches one of the 11 non-manifold cases
      if (nonManifoldCase <= 11)
      {
#ifdef VTK_SURFACE_NETS_3D_ALWAYS_SPLIT_CASES_1_3
        // For cases 1-3, we always consider them non-manifold regardless of the material
        // configuration, because if the materials are different, most probably they are sampling
        // artifacts with no meaningful continuous analog. Every voxel of a voxel case needs to
        // connect with at least 2 other voxels via a face or an edge to be considered a real
        // connection, and cases 1-3 don't satisfy that condition.
        if (nonManifoldCase <= 3)
        {
          // always return the normal split of basic voxel case
          return 0;
        }
#endif
        if (nonManifoldCaseMatches.GetSize() == 0) // if it is a manifold-case
        {
          return ManifoldIndex;
        }
        // if it is indeed a non-manifold case
        if (isVoxelCaseHomogeneous && IsNonManifoldCaseHomogeneous(edgeCase, i))
        {
          // If basic is homogeneous and the complementary split exists, we want to return the
          // table index of its complementary non-manifold case to support contiguous material
          // flow across the non-manifold junction. Non-manifold that have all the labels the same
          // can only be in position 0 or 1. But being in position 0 or 1 doesn't necessarily mean
          // that all the labels are the same, so we need to check the actual labels to be sure.
          // if it's not, the classification is kept as is to maintain clean separation between
          // material boundaries, preserving open-manifold guarantees per label region.
          if (nonManifoldCase >= 4) // complementarySplitExists
          {
            assert(i == 0 || i == 1);
            // return the complementary split of basic voxel case
            return i == 0 ? 1 : 0;
          }
          if (nonManifoldCase <= 8) // normalSplitExists
          {
            assert(i == 0 || i == 1);
            // return the normal split of basic voxel case
            return i;
          }
        }
        else if (!isVoxelCaseHomogeneous)
        {
          // same edge case and subComplementarySplitExists
          if (edgeCase == subEdgeCase && subNonManifoldCase >= 4)
          {
            // find the table index of the sub voxel case
            int8_t subTableIndex = -1;
            for (int8_t j = 0; j < i; ++j)
            {
              if (GetNonManifoldCase(edgeCase, j) == subNonManifoldCase &&
                GetNonManifoldBasicVoxelCase(edgeCase, j) == subVoxelCase)
              {
                subTableIndex = j;
                break;
              }
            }
            assert(subTableIndex != -1 && "Sub non-manifold case should be found in the table");
            // return the complementary split of sub voxel case
            assert(subTableIndex == 0 || subTableIndex == 1);
            return subTableIndex == 0 ? 1 : 0;
          }
          // superset edge case and subNormalSplitExists and normalSplitExists
          if ((edgeCase & subEdgeCase) == subEdgeCase && subNonManifoldCase <= 8 &&
            nonManifoldCase <= 8)
          {
            const auto subVoxelCases = GetManifoldSubVoxelCases(edgeCase, i);
            // the edge case is a superset of the homogeneous edge case — additional faces are
            // generated due to material variations within the active voxels. the primary split
            // is sufficient for this material configuration when no face group contains an
            // unsolvable sub-non-manifold of cases 1-3
            if (IsSplitSufficientForColorEncodedVoxelCase(subVoxelCases, colorEncodedVoxelCase))
            {
              assert((nonManifoldCase != 8 && i <= 1) || (nonManifoldCase == 8 && i <= 9));
#ifdef VTK_SURFACE_NETS_3D_SUPPORT_ALL_SOLVABLE_VARIATIONS_FOR_CASE_8
              // return normal split of basic voxel case
              return i;
#else
              return i <= vtkSurfaceNets3DNonManifoldCases::MaxTableIndex
                ? i
                : static_cast<int8_t>(TableIndexValues::NonManifoldIndexUnsolvable);
#endif
            }
            return TableIndexValues::NonManifoldIndexUnsolvable;
          }
          return TableIndexValues::NonManifoldIndexUnsolvable;
        }
      }
      // if the voxel configuration matches one of the 10 manifold cases that can be non-manifold
      else
      {
        if (nonManifoldCaseMatches.GetSize() == 0) // if it is a manifold-case
        {
          return ManifoldIndex;
        }
        // if it is indeed a non-manifold case
        assert(!IsNonManifoldCaseHomogeneous(edgeCase, i) &&
          "Manifold cases are non-manifold only when they are not homogeneous");
        if (!isVoxelCaseHomogeneous)
        {
          // same edge case and subComplementarySplitExists
          if (edgeCase == subEdgeCase && subNonManifoldCase >= 4)
          {
            // find the table index of the sub voxel case
            int8_t subTableIndex = -1;
            for (int8_t j = 0; j < i; ++j)
            {
              if (GetNonManifoldCase(edgeCase, j) == subNonManifoldCase &&
                GetNonManifoldBasicVoxelCase(edgeCase, j) == subVoxelCase)
              {
                subTableIndex = j;
                break;
              }
            }
            assert(subTableIndex != -1 && "Sub non-manifold case should be found in the table");
            assert(subTableIndex == 0 || subTableIndex == 1);
            return subTableIndex == 0 ? 1 : 0;
          }
          return TableIndexValues::NonManifoldIndexUnsolvable;
        }
      }
    }
  }
  assert(nonManifoldCaseMatches.GetSize() == 0 && "The given edge case and voxel case is manifold");
  return ManifoldIndex;
}

//------------------------------------------------------------------------------
// Explicit template instantiations
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<char>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<int>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<long>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<long long>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<short>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<signed char>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<unsigned char>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<unsigned int>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<unsigned long>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<unsigned long long>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<unsigned short>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<float>& voxelNeighborhood) -> int8_t;
template auto vtkSurfaceNets3DNonManifoldCases::GetNonManifoldTableIndex(
  const VoxelNeighborhood<double>& voxelNeighborhood) -> int8_t;

VTK_ABI_NAMESPACE_END
