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
 * @class   vtkOculusRenderer
 * @brief   Oculus renderer
 *
 * vtkOculusRenderer is a concrete implementation of the abstract class
 * vtkRenderer.  vtkOculusRenderer interfaces to the Oculus rendering library.
*/

#ifndef vtkOculusRenderer_h
#define vtkOculusRenderer_h

#include "vtkRenderingOculusModule.h" // For export macro
#include "vtkOpenGLRenderer.h"


class VTKRENDERINGOCULUS_EXPORT vtkOculusRenderer : public vtkOpenGLRenderer
{
public:
  static vtkOculusRenderer *New();
  vtkTypeMacro(vtkOculusRenderer,vtkOpenGLRenderer);

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
  vtkOculusRenderer();
  ~vtkOculusRenderer();

private:
  vtkOculusRenderer(const vtkOculusRenderer&);  // Not implemented.
  void operator=(const vtkOculusRenderer&);  // Not implemented.
};


#endif
