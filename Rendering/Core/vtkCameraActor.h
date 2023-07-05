// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCameraActor
 * @brief   a frustum to represent a camera.
 *
 * vtkCameraActor is an actor used to represent a camera by its wireframe
 * frustum.
 *
 * @sa
 * vtkLight vtkConeSource vtkFrustumSource vtkCameraActor
 */

#ifndef vtkCameraActor_h
#define vtkCameraActor_h

#include "vtkProp3D.h"
#include "vtkRenderingCoreModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCamera;
class vtkFrustumSource;
class vtkPolyDataMapper;
class vtkActor;
class vtkProperty;

class VTKRENDERINGCORE_EXPORT vtkCameraActor : public vtkProp3D
{
public:
  static vtkCameraActor* New();
  vtkTypeMacro(vtkCameraActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The camera to represent. Initial value is NULL.
   */
  void SetCamera(vtkCamera* camera);
  vtkGetObjectMacro(Camera, vtkCamera);
  ///@}

  ///@{
  /**
   * Ratio between the width and the height of the frustum. Initial value is
   * 1.0 (square)
   */
  vtkSetMacro(WidthByHeightRatio, double);
  vtkGetMacro(WidthByHeightRatio, double);
  ///@}

  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(vtkViewport* viewport) override;

  /**
   * Does this prop have some translucent polygonal geometry? No.
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  double* GetBounds() override;

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  vtkMTimeType GetMTime() override;

  /**
   * Get property of the internal actor.
   */
  vtkProperty* GetProperty();

  /**
   * Set property of the internal actor.
   */
  void SetProperty(vtkProperty* p);

protected:
  vtkCameraActor();
  ~vtkCameraActor() override;

  void UpdateViewProps();

  vtkCamera* Camera;
  double WidthByHeightRatio;

  vtkFrustumSource* FrustumSource;
  vtkPolyDataMapper* FrustumMapper;
  vtkActor* FrustumActor;

private:
  vtkCameraActor(const vtkCameraActor&) = delete;
  void operator=(const vtkCameraActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
