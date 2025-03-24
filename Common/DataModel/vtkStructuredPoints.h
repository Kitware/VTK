// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkStructuredPoints
 * @brief   A subclass of ImageData.
 *
 * StructuredPoints is a subclass of ImageData that requires the data extent
 * to exactly match the update extent. Normal image data allows that the
 * data extent may be larger than the update extent.
 * StructuredPoints also defines the origin differently that vtkImageData.
 * For structured points the origin is the location of first point.
 * Whereas images define the origin as the location of point 0, 0, 0.
 * Image Origin is stored in ivar, and structured points
 * have special methods for setting/getting the origin/extents.
 */

#ifndef vtkStructuredPoints_h
#define vtkStructuredPoints_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImageData.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONDATAMODEL_EXPORT vtkStructuredPoints : public vtkImageData
{
public:
  static vtkStructuredPoints* New();
  vtkTypeMacro(vtkStructuredPoints, vtkImageData);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * To simplify filter superclasses,
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_STRUCTURED_POINTS; }

protected:
  vtkStructuredPoints();
  ~vtkStructuredPoints() override = default;

private:
  vtkStructuredPoints(const vtkStructuredPoints&) = delete;
  void operator=(const vtkStructuredPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
