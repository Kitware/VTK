// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGlyph2D
 * @brief   copy oriented and scaled glyph geometry to every input point (2D specialization)
 *
 * This subclass of vtkGlyph3D is a specialization to 2D. Transformations
 * (i.e., translation, scaling, and rotation) are constrained to the plane.
 * For example, rotations due to a vector are computed from the x-y
 * coordinates of the vector only, and are assumed to occur around the
 * z-axis. (See vtkGlyph3D for documentation on the interface to this
 * class.)
 *
 * Frequently this class is used in combination with vtkGlyphSource.
 * vtkGlyphSource2D can produce a family of 2D glyphs.
 *
 * @sa
 * vtkTensorGlyph vtkGlyph3D vtkProgrammableGlyphFilter vtkGlyphSource2D
 */

#ifndef vtkGlyph2D_h
#define vtkGlyph2D_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkGlyph3D.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkGlyph2D : public vtkGlyph3D
{
public:
  ///@{
  /**
   * Standard methods for obtaining type information and printing.
   */
  vtkTypeMacro(vtkGlyph2D, vtkGlyph3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Construct object with scaling on, scaling mode is by scalar value,
   * scale factor = 1.0, the range is (0,1), orient geometry is on, and
   * orientation is by vector. Clamping and indexing are turned off. No
   * initial sources are defined.
   */
  static vtkGlyph2D* New();

protected:
  vtkGlyph2D() = default;
  ~vtkGlyph2D() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGlyph2D(const vtkGlyph2D&) = delete;
  void operator=(const vtkGlyph2D&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
