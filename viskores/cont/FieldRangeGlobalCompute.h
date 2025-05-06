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
#ifndef viskores_cont_FieldRangeGlobalCompute_h
#define viskores_cont_FieldRangeGlobalCompute_h

#include <viskores/cont/FieldRangeCompute.h>

namespace viskores
{
namespace cont
{

namespace detail
{

VISKORES_CONT_EXPORT VISKORES_CONT viskores::cont::ArrayHandle<viskores::Range> MergeRangesGlobal(
  const viskores::cont::ArrayHandle<viskores::Range>& range);

} // namespace detail

/// \brief utility functions to compute global ranges for dataset fields.
///
/// These functions compute global ranges for fields in a single DataSet or a
/// PartitionedDataSet.
/// In non-distributed environments, this is exactly same as `FieldRangeCompute`. In
/// distributed environments, however, the range is computed locally on each rank
/// and then a reduce-all collective is performed to reduces the ranges on all ranks.

//{@
/// Returns the range for a field from a dataset. If the field is not present, an empty
/// ArrayHandle will be returned.
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeGlobalCompute(
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
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::cont::ArrayHandle<viskores::Range> FieldRangeGlobalCompute(
  const viskores::cont::PartitionedDataSet& pds,
  const std::string& name,
  viskores::cont::Field::Association assoc = viskores::cont::Field::Association::Any);
//@}

}
} // namespace viskores::cont

#endif
