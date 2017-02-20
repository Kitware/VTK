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

class vtkActor;

class VTKRENDERINGOPENVR_EXPORT vtkOpenVRRenderer : public vtkOpenGLRenderer
{
public:
  static vtkOpenVRRenderer *New();
  vtkTypeMacro(vtkOpenVRRenderer,vtkOpenGLRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Automatically set up the camera based on the visible actors.
   * The camera will reposition itself to view the center point of the actors,
   * and move along its initial view plane normal (i.e., vector defined from
   * camera position to focal point) so that all of the actors can be seen.
   */
  void ResetCamera() VTK_OVERRIDE;

  /**
   * Automatically set up the camera based on a specified bounding box
   * (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
   * that its focal point is the center of the bounding box, and adjust its
   * distance and position to preserve its initial view plane normal
   * (i.e., vector defined from camera position to focal point). Note: is
   * the view plane is parallel to the view up axis, the view up axis will
   * be reset to one of the three coordinate axes.
   */
  void ResetCamera(double bounds[6]) VTK_OVERRIDE;

  /**
   * Alternative version of ResetCamera(bounds[6]);
   */
  void ResetCamera(double xmin, double xmax, double ymin, double ymax,
                   double zmin, double zmax) VTK_OVERRIDE;

  using vtkRenderer::ResetCameraClippingRange;

  //@{
  /**
   * Reset the camera clipping range based on a bounding box.
   * This method is called from ResetCameraClippingRange()
   * If Deering frustrum is used then the bounds get expanded
   * by the camera's modelview matrix.
   */
  void ResetCameraClippingRange( double bounds[6] ) VTK_OVERRIDE;
  //@}

  /**
   * Concrete open gl render method.
   */
  void DeviceRender(void);

  /**
   * SHow the floor of the VR world
   */
  virtual void SetShowFloor(bool);
  virtual bool GetShowFloor() {
    return this->ShowFloor; }

protected:
  vtkOpenVRRenderer();
  ~vtkOpenVRRenderer();

  vtkActor *FloorActor;
  bool ShowFloor;

private:
  vtkOpenVRRenderer(const vtkOpenVRRenderer&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenVRRenderer&) VTK_DELETE_FUNCTION;
};


#endif
