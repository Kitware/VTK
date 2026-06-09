//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_ViskoresBuilder_H_
#define fides_ViskoresBuilder_H_

#include <fides/internal/OutputBuilder.h>

#include <viskores/cont/DataSet.h>
#include <viskores/cont/PartitionedDataSet.h>
#include <viskores/cont/UnknownArrayHandle.h>

#include <unordered_map>
#include <vector>

namespace fides
{

/// \brief OutputBuilder implementation that produces Viskores datasets.
///
/// ViskoresBuilder converts raw arrays and builder tokens into Viskores
/// data structures (ArrayHandles, CoordinateSystems, CellSets, DataSets).
/// All DataSet assembly is deferred to Finalize() because arrays are
/// not populated until after ADIOS DoAllReads.
class ViskoresBuilder : public OutputBuilder
{
public:
  // --- OutputBuilder interface ---
  void Reset() override;
  void Finalize() override;

  // --- Viskores-specific accessors ---

  /// Returns mutable reference to the internal DataSets vector.
  /// Used by PostRead hooks (XGC/GTC/GX) to modify DataSets after Finalize.
  std::vector<viskores::cont::DataSet>& GetDataSets();

  /// Assembles and returns the final PartitionedDataSet from added partitions.
  viskores::cont::PartitionedDataSet GetResult();

private:
  // --- Built during Finalize ---
  std::vector<viskores::cont::DataSet> DataSetsVec;

  template <typename T>
  static viskores::cont::UnknownArrayHandle MakeViskoresArrayHandle(const RawArray& raw);

  static viskores::cont::UnknownArrayHandle MakeHandle(const RawArray& raw);
};

} // namespace fides

#endif // fides_ViskoresBuilder_H_
