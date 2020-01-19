// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkCell.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkNonLinearCell : public vtkCell
{
public:
  vtkTypeMacro(vtkNonLinearCell, vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Non-linear cells require special treatment (tessellation) when
   * converting to graphics primitives (during mapping). The vtkCell
   * API IsLinear() is modified to indicate this requirement.
   */
  int IsLinear() VTK_FUTURE_CONST override { return 0; }

  /**
   * Clip the cell based on the input cellScalars and the
   * specified value. The output of the clip operation will be one or
   * more cells of the same topological dimension as the original cell.
   * For more information see vtkCell::Clip.
   *
   * This method differs from the vtkCell::Clip function in such a way
   * that it tells more information about how the clipped cell was
   * handled. For 2D cells vtkCell::Clip is expected to return either
   * triangles, quads or polygon, and for 3D cells either tetras or
   * wedges. However, it is interesting (especially for non linear cells)
   * to be able to return the same cell type when the cell is totally
   * inside or outside the clipping function. This Clip function
   * resolves exactly this issue by returning a bool : if StableClip
   * returns true then the newly inserted cell is the same type as the
   * current cell, otherwise it will be one of the "expected" types
   * as returned by vtkCell::Clip.
   *
   * Note: This function could also benefit being moved to vtkCell but
   * requires much more work to be supported by all cell types.
   *
   * @see vtkCell::Clip
   */
  virtual bool StableClip(double value, vtkDataArray* cellScalars,
    vtkIncrementalPointLocator* locator, vtkCellArray* connectivity, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd, int insideOut)
  {
    this->Clip(
      value, cellScalars, locator, connectivity, inPd, outPd, inCd, cellId, outCd, insideOut);
    return false;
  }

protected:
  vtkNonLinearCell();
  ~vtkNonLinearCell() override = default;

private:
  vtkNonLinearCell(const vtkNonLinearCell&) = delete;
  void operator=(const vtkNonLinearCell&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
