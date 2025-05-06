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
#ifndef viskores_cont_BoundsGlobalCompute_h
#define viskores_cont_BoundsGlobalCompute_h

#include <viskores/Bounds.h>
#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

class DataSet;
class PartitionedDataSet;

//@{
/// \brief Functions to compute bounds for a single dataset or partitioned
/// dataset globally
///
/// These are utility functions that compute bounds for a single dataset or
/// partitioned dataset globally i.e. across all ranks when operating in a
/// distributed environment. When Viskores not operating in an distributed
/// environment, these behave same as `viskores::cont::BoundsCompute`.
///
/// Note that if the provided CoordinateSystem does not exists, empty bounds
/// are returned. Likewise, for PartitionedDataSet, partitions without the
/// chosen CoordinateSystem are skipped.
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::DataSet& dataset,
                                     viskores::Id coordinate_system_index = 0);

VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::PartitionedDataSet& pds,
                                     viskores::Id coordinate_system_index = 0);

VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::DataSet& dataset,
                                     const std::string& coordinate_system_name);

VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::Bounds BoundsGlobalCompute(const viskores::cont::PartitionedDataSet& pds,
                                     const std::string& coordinate_system_name);
//@}
}
}
#endif
