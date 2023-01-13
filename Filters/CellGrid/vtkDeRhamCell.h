// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDeRhamCell
 * @brief   Base class for cells that form 2-d or 3-d DeRham complexes.
 *
 * This class exists so that cell-attributes specific to DeRham sequences
 * can register responders and calculators.
 * Only 2-d and 3-d cell-shapes are allowed to subclass vtkDeRhamCell
 * since H(Curl) and H(Div) function spaces require cells of parametric
 * dimension 2 or greater.
 *
 * This class inherits the API of vtkDGCell.
 */

#ifndef vtkDeRhamCell_h
#define vtkDeRhamCell_h
#include "vtkDGCell.h"

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkDeRhamCell : public vtkDGCell
{
public:
  vtkTypeMacro(vtkDeRhamCell, vtkDGCell);
  vtkInheritanceHierarchyOverrideMacro(vtkDeRhamCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkDeRhamCell() = default;
  ~vtkDeRhamCell() override = default;

private:
  vtkDeRhamCell(const vtkDeRhamCell&) = delete;
  void operator=(const vtkDeRhamCell&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
