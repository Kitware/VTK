//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

// Internal header — not installed. Declares the Viskores extraction free
// function used by FidesDataSetWriter::Write(ViskoresCollection).

#ifndef fides_FidesDataSetWriterViskores_H_
#define fides_FidesDataSetWriterViskores_H_

#include <fides/PartitionInfo.h>

#include <set>
#include <string>
#include <vector>

namespace viskores
{
namespace cont
{
class PartitionedDataSet;
}
}

namespace fides
{

/// Extract PartitionInfo from a Viskores partitioned dataset.
/// @param dataSets       The Viskores dataset to extract from.
/// @param fieldsToWrite  If non-empty, only these fields are extracted.
/// @return A vector of PartitionInfo, one per non-empty Viskores partition.
std::vector<PartitionInfo> ExtractViskoresPartitions(
  const viskores::cont::PartitionedDataSet& dataSets,
  const std::set<std::string>& fieldsToWrite);

} // namespace fides

#endif // fides_FidesDataSetWriterViskores_H_
