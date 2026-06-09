//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#ifndef fides_VTKBuilder_H_
#define fides_VTKBuilder_H_

#include <fides/internal/OutputBuilder.h>

#include <vtkDataObject.h>
#include <vtkPartitionedDataSet.h>
#include <vtkSmartPointer.h>

#include <unordered_map>
#include <vector>

#include <vtkABINamespace.h>
VTK_ABI_NAMESPACE_BEGIN
class vtkDataArray;
VTK_ABI_NAMESPACE_END

namespace fides
{

/// \brief OutputBuilder implementation that produces VTK datasets.
///
/// VTKBuilder converts raw arrays and builder tokens into VTK
/// data structures (vtkImageData, vtkRectilinearGrid, vtkStructuredGrid,
/// vtkUnstructuredGrid). All DataSet assembly is deferred to Finalize()
/// because arrays are not populated until after ADIOS DoAllReads.
class VTKBuilder : public OutputBuilder
{
public:
  // --- OutputBuilder interface ---
  size_t CreateUniformCoordinates(const int64_t dims[3],
                                  const double origin[3],
                                  const double spacing[3],
                                  const int64_t start[3] = nullptr) override;
  size_t CreateStructuredCellSet(const int64_t dims[3]) override;
  void Reset() override;
  void Finalize() override;

  // --- VTK-specific accessors ---

  /// Assembles and returns the final vtkPartitionedDataSet from added partitions.
  vtkSmartPointer<vtkPartitionedDataSet> GetResult();

private:
  // --- Built during Finalize ---
  // Holds vtkDataObject (not vtkDataSet) so cell grids — which derive from
  // vtkDataObject directly, peer to vtkDataSet — can sit alongside the
  // traditional dataset variants.
  std::vector<vtkSmartPointer<vtkDataObject>> DataSetsVec;

  /// Create a vtkDataArray from a RawArray using zero-copy when possible.
  static vtkSmartPointer<vtkDataArray> MakeVTKArray(const RawArray& raw);

  /// Builds a vtkCellGrid for each cellgrid token recorded by the cellgrid
  /// model. Called from Finalize() after the traditional dataset loop.
  void FinalizeCellGrids();
};

} // namespace fides

#endif // fides_VTKBuilder_H_
