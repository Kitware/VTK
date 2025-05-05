// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRenderedAreaPicker
 * @brief   Uses graphics hardware to picks props behind
 * a selection rectangle on a viewport.
 *
 *
 * Like vtkAreaPicker, this class picks all props within a selection area
 * on the screen. The difference is in implementation. This class uses
 * graphics hardware to perform the test where the other uses software
 * bounding box/frustum intersection testing.
 *
 * This picker is more conservative than vtkAreaPicker. It will reject
 * some objects that pass the bounding box test of vtkAreaPicker. This
 * will happen, for instance, when picking through a corner of the bounding
 * box when the data set does not have any visible geometry in that corner.
 */

#ifndef vtkRenderedAreaPicker_h
#define vtkRenderedAreaPicker_h

#include "vtkAreaPicker.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALAUTO vtkRenderedAreaPicker : public vtkAreaPicker
{
public:
  static vtkRenderedAreaPicker* New();
  vtkTypeMacro(vtkRenderedAreaPicker, vtkAreaPicker);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Perform pick operation in volume behind the given screen coordinates.
   * Props intersecting the selection frustum will be accessible via GetProp3D.
   * GetPlanes returns a vtkImplicitFunction suitable for vtkExtractGeometry.
   */
  int AreaPick(double x0, double y0, double x1, double y1, vtkRenderer*) override;

protected:
  vtkRenderedAreaPicker();
  ~vtkRenderedAreaPicker() override;

private:
  vtkRenderedAreaPicker(const vtkRenderedAreaPicker&) = delete;
  void operator=(const vtkRenderedAreaPicker&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
