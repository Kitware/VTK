//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

// Internal header — not installed. Declares the VTK extraction free function
// used by FidesDataSetWriter::Write(VTKCollection).

#ifndef fides_FidesDataSetWriterVTK_H_
#define fides_FidesDataSetWriterVTK_H_

#include <fides/PartitionInfo.h>

#include <set>
#include <string>
#include <vector>

#include <vtkABINamespace.h>
VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSet;
VTK_ABI_NAMESPACE_END

namespace fides
{

/// Output of \c ExtractVTKPartitions: one vector per supported VTK
/// partition kind. Exactly one of the vectors is populated per step;
/// mixing vtkDataSet and vtkCellGrid partitions in the same input is
/// rejected.
struct VTKExtraction
{
  std::vector<PartitionInfo> DataSets;
  std::vector<CellGridPartitionInfo> CellGrids;
};

/// Extract a VTKExtraction from a VTK partitioned dataset.
/// @param dataSets     The VTK dataset to extract from.
/// @param fieldsToWrite  If non-empty, only these fields are extracted
///                       (for vtkDataSet partitions).
/// @return One PartitionInfo or CellGridPartitionInfo per non-empty
///         VTK partition; an exception is thrown if both kinds appear.
VTKExtraction ExtractVTKPartitions(vtkPartitionedDataSet* dataSets,
                                   const std::set<std::string>& fieldsToWrite);

} // namespace fides

#endif // fides_FidesDataSetWriterVTK_H_
