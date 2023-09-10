// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkFiltersCellGrid_h
#define vtkFiltersCellGrid_h

#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkObject.h"

VTK_ABI_NAMESPACE_BEGIN

/**\brief A registrar for cell types contained in this module.
 */
class VTKFILTERSCELLGRID_EXPORT vtkFiltersCellGrid : public vtkObject
{
public:
  vtkTypeMacro(vtkFiltersCellGrid, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Call this method before running constructing or running algorithms
  /// on instances of vtkCellGrid so that the discontinuoug Galerkin cells
  /// will be registered along with their responders.
  static bool RegisterCellsAndResponders();

protected:
  vtkFiltersCellGrid(const vtkFiltersCellGrid&) = delete;
  void operator=(const vtkFiltersCellGrid&) = delete;
};

VTK_ABI_NAMESPACE_END

#endif // vtkFiltersCellGrid_h
