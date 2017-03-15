/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedAreaPicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkAreaPicker.h"

class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkRenderedAreaPicker : public vtkAreaPicker
{
public:
  static vtkRenderedAreaPicker *New();
  vtkTypeMacro(vtkRenderedAreaPicker, vtkAreaPicker);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Perform pick operation in volume behind the given screen coordinates.
   * Props intersecting the selection frustum will be accessible via GetProp3D.
   * GetPlanes returns a vtkImplicitFunction suitable for vtkExtractGeometry.
   */
  int AreaPick(double x0, double y0, double x1, double y1, vtkRenderer *) VTK_OVERRIDE;

protected:
  vtkRenderedAreaPicker();
  ~vtkRenderedAreaPicker() VTK_OVERRIDE;

private:
  vtkRenderedAreaPicker(const vtkRenderedAreaPicker&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRenderedAreaPicker&) VTK_DELETE_FUNCTION;
};

#endif
