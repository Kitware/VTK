// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolyDataToUnstructuredGrid
 * @brief   Filter which converts a polydata to unstructured grid.
 *
 * This filter converts a polydata to an unstructured grid. The output is
 * a vtkUnstructuredGrid with the same points as the input vtkPolyData.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 */

#ifndef vtkPolyDataToUnstructuredGrid_h
#define vtkPolyDataToUnstructuredGrid_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;

class VTKFILTERSCORE_EXPORT vtkPolyDataToUnstructuredGrid : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkPolyDataToUnstructuredGrid* New();
  vtkTypeMacro(vtkPolyDataToUnstructuredGrid, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Check if a polydata has only 1 cell array populated, therefore it can be just shallow copied
   */
  static bool CanBeProcessedFast(vtkPolyData* polyData);

protected:
  vtkPolyDataToUnstructuredGrid();
  ~vtkPolyDataToUnstructuredGrid() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkPolyDataToUnstructuredGrid(const vtkPolyDataToUnstructuredGrid&) = delete;
  void operator=(const vtkPolyDataToUnstructuredGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkFastPolyDataToUnstructuredGrid_h
