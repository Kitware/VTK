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
#ifndef viskores_cont_MergePartitionedDataset_h
#define viskores_cont_MergePartitionedDataset_h

#include <viskores/Bounds.h>
#include <viskores/cont/viskores_cont_export.h>

namespace viskores
{
namespace cont
{

class DataSet;
class PartitionedDataSet;

//@{
/// \brief This function can merge multiple data sets into on data set.
/// This function assume all input partitions have the same coordinates systems.
/// If a field does not exist in a specific partition but exists in other partitions,
/// the invalide value will be used to fill the coresponding region of that field in the merged data set.
VISKORES_CONT_EXPORT
VISKORES_CONT
viskores::cont::DataSet MergePartitionedDataSet(
  const viskores::cont::PartitionedDataSet& partitionedDataSet,
  viskores::Float64 invalidValue = viskores::Nan64());

//@}
}
}

#endif
