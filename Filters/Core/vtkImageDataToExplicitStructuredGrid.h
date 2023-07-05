// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageDataToExplicitStructuredGrid
 * @brief   Filter which converts a 3D image data into an explicit structured grid.
 */

#ifndef vtkImageDataToExplicitStructuredGrid_h
#define vtkImageDataToExplicitStructuredGrid_h

#include "vtkExplicitStructuredGridAlgorithm.h"
#include "vtkFiltersCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkImageDataToExplicitStructuredGrid
  : public vtkExplicitStructuredGridAlgorithm
{
public:
  static vtkImageDataToExplicitStructuredGrid* New();
  vtkTypeMacro(vtkImageDataToExplicitStructuredGrid, vtkExplicitStructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageDataToExplicitStructuredGrid() = default;
  ~vtkImageDataToExplicitStructuredGrid() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageDataToExplicitStructuredGrid(const vtkImageDataToExplicitStructuredGrid&) = delete;
  void operator=(const vtkImageDataToExplicitStructuredGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
