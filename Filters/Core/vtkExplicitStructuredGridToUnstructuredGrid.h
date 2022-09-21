/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExplicitStructuredGridToUnstructuredGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExplicitStructuredGridToUnstructuredGrid
 * @brief   Filter which converts an explicit structured grid into an unstructured grid.
 */

#ifndef vtkExplicitStructuredGridToUnstructuredGrid_h
#define vtkExplicitStructuredGridToUnstructuredGrid_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkExplicitStructuredGridToUnstructuredGrid
  : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkExplicitStructuredGridToUnstructuredGrid* New();
  vtkTypeMacro(vtkExplicitStructuredGridToUnstructuredGrid, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkExplicitStructuredGridToUnstructuredGrid() = default;
  ~vtkExplicitStructuredGridToUnstructuredGrid() override = default;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkExplicitStructuredGridToUnstructuredGrid(
    const vtkExplicitStructuredGridToUnstructuredGrid&) = delete;
  void operator=(const vtkExplicitStructuredGridToUnstructuredGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
