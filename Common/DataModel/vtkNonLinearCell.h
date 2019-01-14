/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkNonLinearCell.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkNonLinearCell
 * @brief   abstract superclass for non-linear cells
 *
 * vtkNonLinearCell is an abstract superclass for non-linear cell types.
 * Cells that are a direct subclass of vtkCell or vtkCell3D are linear;
 * cells that are a subclass of vtkNonLinearCell have non-linear interpolation
 * functions. Non-linear cells require special treatment when tessellating
 * or converting to graphics primitives. Note that the linearity of the cell
 * is a function of whether the cell needs tessellation, which does not
 * strictly correlate with interpolation order (e.g., vtkHexahedron has
 * non-linear interpolation functions (a product of three linear functions
 * in r-s-t) even thought vtkHexahedron is considered linear.)
*/

#ifndef vtkNonLinearCell_h
#define vtkNonLinearCell_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class VTKCOMMONDATAMODEL_EXPORT vtkNonLinearCell : public vtkCell
{
public:
  vtkTypeMacro(vtkNonLinearCell,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Non-linear cells require special treatment (tessellation) when
   * converting to graphics primitives (during mapping). The vtkCell
   * API IsLinear() is modified to indicate this requirement.
   */
  int IsLinear() override {return 0;}

protected:
  vtkNonLinearCell();
  ~vtkNonLinearCell() override {}

private:
  vtkNonLinearCell(const vtkNonLinearCell&) = delete;
  void operator=(const vtkNonLinearCell&) = delete;
};

#endif


