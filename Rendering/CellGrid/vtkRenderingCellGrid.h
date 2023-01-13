// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkRenderingCellGrid_h
#define vtkRenderingCellGrid_h

#include "vtkObject.h"
#include "vtkRenderingCellGridModule.h" // For export macro.

VTK_ABI_NAMESPACE_BEGIN

/**\brief A registrar for cell types contained in this module.
 */
class VTKRENDERINGCELLGRID_EXPORT vtkRenderingCellGrid : public vtkObject
{
public:
  vtkTypeMacro(vtkRenderingCellGrid, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Call this method before running constructing or running algorithms
  /// on instances of vtkCellGrid so that the discontinuoug Galerkin cells
  /// will be registered along with their responders.
  static bool RegisterCellsAndResponders();

protected:
  vtkRenderingCellGrid(const vtkRenderingCellGrid&) = delete;
  void operator=(const vtkRenderingCellGrid&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkRenderingCellGrid_h
