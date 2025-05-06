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
#ifndef viskores_cont_FieldRangeCompute_h
#define viskores_cont_FieldRangeCompute_h

#include <viskores/cont/DataSet.h>
#include <viskores/cont/Field.h>
#include <viskores/cont/PartitionedDataSet.h>

#include <numeric>

namespace viskores
{
namespace cont
{
/// \brief Compute ranges for fields in a DataSet or PartitionedDataSet.
///
/// These methods to compute ranges for fields in a single dataset or a
/// partitioned dataset.
/// When using Viskores in a hybrid-parallel environment with distributed processing,
/// this class uses ranges for locally available data alone. Use FieldRangeGlobalCompute
/// to compute ranges globally across all ranks even in distributed mode.

//{@
/// Returns the range for a field from a dataset. If the field is not present, an empty
/// ArrayHandle will be returned.
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeCompute(
  const viskores::cont::DataSet& dataset,
  const std::string& name,
  viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any);
//@}

//{@
/// Returns the range for a field from a PartitionedDataSet. If the field is
/// not present on any of the partitions, an empty ArrayHandle will be
/// returned. If the field is present on some partitions, but not all, those
/// partitions without the field are skipped.
///
/// The returned array handle will have as many values as the maximum number of
/// components for the selected field across all partitions.
///
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeCompute(
  const viskores::cont::PartitionedDataSet& pds,
  const std::string& name,
  viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any);
//@}

}
} // namespace viskores::cont

#endif
