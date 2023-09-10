// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkIOCellGrid_h
#define vtkIOCellGrid_h

#include "vtkIOCellGridModule.h" // For export macro.
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN

/**\brief A registrar for cell types contained in this module.
 */
class VTKIOCELLGRID_EXPORT vtkIOCellGrid : public vtkObject
{
public:
  vtkTypeMacro(vtkIOCellGrid, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Call this method before running constructing or running algorithms
  /// on instances of vtkCellGrid so that the discontinuous Galerkin cells
  /// will be registered along with their responders.
  static bool RegisterCellsAndResponders();

protected:
  vtkIOCellGrid(const vtkIOCellGrid&) = delete;
  void operator=(const vtkIOCellGrid&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkIOCellGrid_h
