// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkVRRenderer
 * @brief   Renderer class for VR/AR context.
 *
 * vtkVRRenderer is an abstract vtkRenderer class that is meant to be used in VR context.
 * It defines a floor actor with a grid fading with the distance, as well as a
 * cross-like marker that can be attached to the tip of a controller (can be
 * used e.g. to help place points).
 *
 * Subclasses must define MakeCamera().
 */

#ifndef vtkVRRenderer_h
#define vtkVRRenderer_h

#include "vtkNew.h" // for ivar
#include "vtkOpenGLRenderer.h"
#include "vtkRenderingVRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
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

  ///@{
  /**
   * Reset the camera clipping range based on a bounding box.
   */
  void ResetCameraClippingRange() override;
  void ResetCameraClippingRange(const double bounds[6]) override;
  ///@}

  /**
   * Abstract function that creates a new Camera suitable for use with this type of Renderer.
   */
  VTK_NEWINSTANCE vtkCamera* MakeCamera() override = 0;

  /**
   * Store in \p transform the floor transform.
   */
  virtual void GetFloorTransform(vtkTransform* transform);

  /**
   * Render the floor using GetFloorTransform
   */
  void DeviceRender() override;

  ///@{
  /**
   * Set/get whether to show a white floor corresponding to the physical floor.
   * Default is false.
   */
  virtual void SetShowFloor(bool value);
  vtkGetMacro(ShowFloor, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to display a white cross marker at the tip of the left controller.
   * Default is false.
   */
  virtual void SetShowLeftMarker(bool value);
  vtkGetMacro(ShowLeftMarker, bool);
  ///@}

  ///@{
  /**
   * Set/get whether to display a white cross marker at the tip of the right controller.
   * Default is false.
   */
  virtual void SetShowRightMarker(bool value);
  vtkGetMacro(ShowRightMarker, bool);
  ///@}

protected:
  vtkVRRenderer();
  ~vtkVRRenderer() override = default;

  vtkNew<vtkActor> FloorActor;
  bool ShowFloor = false;

private:
  vtkVRRenderer(const vtkVRRenderer&) = delete;
  void operator=(const vtkVRRenderer&) = delete;

  vtkNew<vtkActor> LeftMarkerActor;
  vtkNew<vtkActor> RightMarkerActor;
  bool ShowLeftMarker = false;
  bool ShowRightMarker = false;
};

VTK_ABI_NAMESPACE_END
#endif
