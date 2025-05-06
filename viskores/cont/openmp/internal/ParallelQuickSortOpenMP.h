//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/openmp/internal/DeviceAdapterTagOpenMP.h>
#include <viskores/cont/openmp/internal/FunctorsOpenMP.h>

#include <viskores/cont/internal/FunctorsGeneral.h>

#include <viskores/Types.h>
#include <viskores/cont/ArrayHandle.h>

#include <omp.h>

#include <iterator>

namespace viskores
{
namespace cont
{
namespace openmp
{
namespace sort
{
namespace quick
{

template <typename IterType, typename RawBinaryCompare>
struct QuickSorter
{
  using BinaryCompare = viskores::cont::internal::WrappedBinaryOperator<bool, RawBinaryCompare>;
  using ValueType = typename std::iterator_traits<IterType>::value_type;

  IterType Data;
  BinaryCompare Compare;
  viskores::Id SerialSize;

  QuickSorter(IterType iter, RawBinaryCompare comp)
    : Data(iter)
    , Compare(comp)
    , SerialSize(0)
  {
  }

  void Execute(const viskores::Id2 range)
  {
    VISKORES_OPENMP_DIRECTIVE(parallel default(shared))
    {
      VISKORES_OPENMP_DIRECTIVE(single)
      {
        this->Prepare(range);
        this->Sort(range);
      }
    }
  }

private:
  void Prepare(const viskores::Id2 /*range*/)
  {
    // Rough benchmarking on an 4-core+4HT processor shows that this sort is
    // most efficient (within 5% of TBB sort) when we switch to a serial
    // implementation once a partition is less than 32K keys
    this->SerialSize = 32768;
  }

  viskores::Pair<viskores::Id, ValueType> MedianOf3(
    const viskores::Pair<viskores::Id, ValueType>& v1,
    const viskores::Pair<viskores::Id, ValueType>& v2,
    const viskores::Pair<viskores::Id, ValueType>& v3) const
  {
    if (this->Compare(v1.second, v2.second))
    { // v1 < v2
      if (this->Compare(v1.second, v3.second))
      { // v1 < v3
        if (this->Compare(v2.second, v3.second))
        { // v1 < v2 < v3
          return v2;
        }
        else // v3 < v2
        {    // v1 < v3 < v2
          return v3;
        }
      }
      else // v3 < v1
      {    // v3 < v1 < v2
        return v1;
      }
    }
    else
    { // v2 < v1
      if (this->Compare(v2.second, v3.second))
      { // v2 < v3
        if (this->Compare(v1.second, v3.second))
        { // v2 < v1 < v3
          return v1;
        }
        else
        { // v2 < v3 < v1
          return v3;
        }
      }
      else
      { // v3 < v2 < v1
        return v2;
      }
    }
  }

  viskores::Pair<viskores::Id, ValueType> MedianOf3(const viskores::Id ids[3]) const
  {
    return this->MedianOf3(viskores::Pair<viskores::Id, ValueType>(ids[0], this->Data[ids[0]]),
                           viskores::Pair<viskores::Id, ValueType>(ids[1], this->Data[ids[1]]),
                           viskores::Pair<viskores::Id, ValueType>(ids[2], this->Data[ids[2]]));
  }

  viskores::Pair<viskores::Id, ValueType> PseudoMedianOf9(const viskores::Id ids[9]) const
  {
    return this->MedianOf3(
      this->MedianOf3(ids), this->MedianOf3(ids + 3), this->MedianOf3(ids + 6));
  }

  // Approximate the median of the range and return its index.
  viskores::Pair<viskores::Id, ValueType> SelectPivot(const viskores::Id2 range) const
  {
    const viskores::Id numVals = range[1] - range[0];
    assert(numVals >= 9);

    // Pseudorandomize the pivot locations to avoid issues with periodic data
    // (evenly sampling inputs with periodic values tends to cause the same
    // value to be obtained for all samples)
    const viskores::Id seed = range[0] * 3 / 2 + range[1] * 11 / 3 + numVals * 10 / 7;
    const viskores::Id delta = (numVals / 9) * 4 / 3;

    viskores::Id sampleLocations[9] = {
      range[0] + ((seed + 0 * delta) % numVals), range[0] + ((seed + 1 * delta) % numVals),
      range[0] + ((seed + 2 * delta) % numVals), range[0] + ((seed + 3 * delta) % numVals),
      range[0] + ((seed + 4 * delta) % numVals), range[0] + ((seed + 5 * delta) % numVals),
      range[0] + ((seed + 6 * delta) % numVals), range[0] + ((seed + 7 * delta) % numVals),
      range[0] + ((seed + 8 * delta) % numVals)
    };

    return this->PseudoMedianOf9(sampleLocations);
  }

  // Select a pivot and partition data with it, returning the final location of
  // the pivot element(s). We use Bentley-McIlroy three-way partitioning to
  // improve handling of duplicate keys, so the pivot "location" is actually
  // a range of identical keys, hence the viskores::Id2 return type, which mark
  // the [begin, end) of the pivot range.
  viskores::Id2 PartitionData(const viskores::Id2 range)
  {
    using namespace std; // For ADL swap

    const viskores::Pair<viskores::Id, ValueType> pivotData = this->SelectPivot(range);
    const viskores::Id& origPivotIdx = pivotData.first;
    const ValueType& pivotVal = pivotData.second;

    // Move the pivot to the end of the block while we partition the rest:
    swap(this->Data[origPivotIdx], this->Data[range[1] - 1]);

    // Indices of the last partitioned keys:
    viskores::Id2 dataCursors(range[0] - 1, range[1] - 1);

    // Indices of the start/end of the keys equal to the pivot:
    viskores::Id2 pivotCursors(dataCursors);

    for (;;)
    {
      // Advance the data cursors past all keys that are already partitioned:
      while (this->Compare(this->Data[++dataCursors[0]], pivotVal))
        ;
      while (this->Compare(pivotVal, this->Data[--dataCursors[1]]) && dataCursors[1] > range[0])
        ;

      // Range is partitioned the cursors have crossed:
      if (dataCursors[0] >= dataCursors[1])
      {
        break;
      }

      // Both dataCursors are pointing at incorrectly partitioned keys. Swap
      // them to place them in the proper partitions:
      swap(this->Data[dataCursors[0]], this->Data[dataCursors[1]]);

      // If the elements we just swapped are actually equivalent to the pivot
      // value, move them to the pivot storage locations:
      if (!this->Compare(this->Data[dataCursors[0]], pivotVal))
      {
        ++pivotCursors[0];
        swap(this->Data[pivotCursors[0]], this->Data[dataCursors[0]]);
      }
      if (!this->Compare(pivotVal, this->Data[dataCursors[1]]))
      {
        --pivotCursors[1];
        swap(this->Data[pivotCursors[1]], this->Data[dataCursors[1]]);
      }
    }

    // Data is now partitioned as:
    // | Equal | Less | Greater | Equal |
    // Move the equal keys to the middle for the final partitioning:
    // | Less | Equal | Greater |
    // First the original pivot value at the end:
    swap(this->Data[range[1] - 1], this->Data[dataCursors[0]]);

    // Update the cursors to either side of the pivot:
    dataCursors = viskores::Id2(dataCursors[0] - 1, dataCursors[0] + 1);

    for (viskores::Id i = range[0]; i < pivotCursors[0]; ++i, --dataCursors[0])
    {
      swap(this->Data[i], this->Data[dataCursors[0]]);
    }

    for (viskores::Id i = range[1] - 2; i > pivotCursors[1]; --i, ++dataCursors[1])
    {
      swap(this->Data[i], this->Data[dataCursors[1]]);
    }

    // Adjust the cursor so we can use them to construct the regions for the
    // recursive call:
    ++dataCursors[0];

    return dataCursors;
  }

  void Sort(const viskores::Id2 range)
  {
    const viskores::Id numVals = range[1] - range[0];
    if (numVals <= this->SerialSize)
    {
      std::sort(this->Data + range[0], this->Data + range[1], this->Compare);
      return;
    }

    const viskores::Id2 pivots = this->PartitionData(range);
    const viskores::Id2 lhRange = viskores::Id2(range[0], pivots[0]);
    const viskores::Id2 rhRange = viskores::Id2(pivots[1], range[1]);

    // Intel compilers seem to have trouble following the 'this' pointer
    // when launching tasks, resulting in a corrupt task environment.
    // Explicitly copying the pointer into a local variable seems to fix this.
    auto explicitThis = this;

    VISKORES_OPENMP_DIRECTIVE(task default(none) firstprivate(rhRange, explicitThis))
    {
      explicitThis->Sort(rhRange);
    }

    this->Sort(lhRange);
  }
};
}
} // end namespace sort::quick
}
}
} // end namespace viskores::cont::openmp
