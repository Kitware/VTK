/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenVRRenderer
 * @brief   OpenVR renderer
 *
 * vtkOpenVRRenderer is a concrete implementation of the abstract class
 * vtkRenderer.  vtkOpenVRRenderer interfaces to the OpenVR rendering library.
*/

#ifndef vtkOpenVRRenderer_h
#define vtkOpenVRRenderer_h

#include "vtkRenderingOpenVRModule.h" // For export macro
#include "vtkOpenGLRenderer.h"

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderer : public vtkOpenGLRenderer
{
public:
  static vtkOpenVRRenderer *New();
  vtkTypeMacro(vtkOpenVRRenderer,vtkOpenGLRenderer);

  /**
   * Automatically set up the camera based on the visible actors.
   * The camera will reposition itself to view the center point of the actors,
   * and move along its initial view plane normal (i.e., vector defined from
   * camera position to focal point) so that all of the actors can be seen.
   */
  virtual void ResetCamera();

  /**
   * Automatically set up the camera based on a specified bounding box
   * (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
   * that its focal point is the center of the bounding box, and adjust its
   * distance and position to preserve its initial view plane normal
   * (i.e., vector defined from camera position to focal point). Note: is
   * the view plane is parallel to the view up axis, the view up axis will
   * be reset to one of the three coordinate axes.
   */
  virtual void ResetCamera(double bounds[6]);

  /**
   * Alternative version of ResetCamera(bounds[6]);
   */
  virtual void ResetCamera(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax);

protected:
  vtkOpenVRRenderer();
  ~vtkOpenVRRenderer();

private:
  vtkOpenVRRenderer(const vtkOpenVRRenderer&);  // Not implemented.
  void operator=(const vtkOpenVRRenderer&);  // Not implemented.
};


#endif
