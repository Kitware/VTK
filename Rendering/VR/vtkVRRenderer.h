/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkVRRenderer.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVRRenderer
 * @brief   VR renderer
 *
 * vtkVRRenderer is an abstract vtkRenderer class that is meant to be used in VR context.
 * It defines a floor actor with a grid fading with the distance.
 *
 * Subclasses must define MakeCamera()
 */

#ifndef vtkVRRenderer_h
#define vtkVRRenderer_h

#include "vtkNew.h" // for ivar
#include "vtkOpenGLRenderer.h"
#include "vtkRenderingVRModule.h" // For export macro

class vtkActor;

class VTKRENDERINGVR_EXPORT vtkVRRenderer : public vtkOpenGLRenderer
{
public:
  vtkTypeMacro(vtkVRRenderer, vtkOpenGLRenderer);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using vtkRenderer::ResetCamera;

  /**
   * Automatically set up the camera based on a specified bounding box
   * (xmin,xmax, ymin,ymax, zmin,zmax). Camera will reposition itself so
   * that its focal point is the center of the bounding box, and adjust its
   * distance and position to preserve its initial view plane normal
   * (i.e., vector defined from camera position to focal point). Note: if
   * the view plane is parallel to the view up axis, the view up axis will
   * be reset to one of the three coordinate axes.
   */
  void ResetCamera(const double bounds[6]) override;

  using vtkRenderer::ResetCameraClippingRange;

  //@{
  /**
   * Reset the camera clipping range based on a bounding box.
   */
  void ResetCameraClippingRange() override;
  void ResetCameraClippingRange(const double bounds[6]) override;
  //@}

  /**
   * Abstract function that creates a new Camera suitable for use with this type of Renderer.
   */
  vtkCamera* MakeCamera() override = 0;

  /**
   * Store in \p transform the floor transform.
   */
  virtual void GetFloorTransform(vtkTransform* transform);

  /**
   * Render the floor using GetFloorTransform
   */
  void DeviceRender() override;

  /**
   * Show the floor of the VR world
   */
  virtual void SetShowFloor(bool);
  virtual bool GetShowFloor() { return this->ShowFloor; }
  //@}

protected:
  vtkVRRenderer();
  ~vtkVRRenderer() override = default;

  vtkNew<vtkActor> FloorActor;
  bool ShowFloor;

private:
  vtkVRRenderer(const vtkVRRenderer&) = delete;
  void operator=(const vtkVRRenderer&) = delete;
};

#endif
