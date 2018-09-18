/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLightActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLightActor
 * @brief   a cone and a frustum to represent a spotlight.
 *
 * vtkLightActor is a composite actor used to represent a spotlight. The cone
 * angle is equal to the spotlight angle, the cone apex is at the position of
 * the light, the direction of the light goes from the cone apex to the center
 * of the base of the cone. The square frustum position is the light position,
 * the frustum focal point is in the direction of the light direction. The
 * frustum vertical view angle (aperture) (this is also the horizontal view
 * angle as the frustum is square) is equal to twice the cone angle. The
 * clipping range of the frustum is arbitrary set by the user
 * (initially at 0.5,11.0).
 *
 * @warning
 * Right now only spotlight are supported but directional light might be
 * supported in the future.
 *
 * @sa
 * vtkLight vtkConeSource vtkFrustumSource vtkCameraActor
*/

#ifndef vtkLightActor_h
#define vtkLightActor_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkLight;
class vtkConeSource;
class vtkPolyDataMapper;
class vtkActor;
class vtkCamera;
class vtkCameraActor;
class vtkBoundingBox;

class VTKRENDERINGCORE_EXPORT vtkLightActor : public vtkProp3D
{
public:
  static vtkLightActor *New();
  vtkTypeMacro(vtkLightActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The spotlight to represent. Initial value is NULL.
   */
  void SetLight(vtkLight *light);
  vtkGetObjectMacro(Light, vtkLight);
  //@}

  //@{
  /**
   * Set/Get the location of the near and far clipping planes along the
   * direction of projection.  Both of these values must be positive.
   * Initial values are  (0.5,11.0)
   */
  void SetClippingRange(double dNear, double dFar);
  void SetClippingRange(const double a[2]);
  vtkGetVector2Macro(ClippingRange, double);
  //@}

  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(vtkViewport *viewport) override;

  /**
   * Does this prop have some translucent polygonal geometry? No.
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) override;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  double *GetBounds() override;

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkLightActor();
  ~vtkLightActor() override;

  void UpdateViewProps();

  vtkLight *Light;
  double ClippingRange[2];

  vtkConeSource *ConeSource;
  vtkPolyDataMapper *ConeMapper;
  vtkActor *ConeActor;

  vtkCamera *CameraLight;
  vtkCameraActor *FrustumActor;

  vtkBoundingBox *BoundingBox;

private:
  vtkLightActor(const vtkLightActor&) = delete;
  void operator=(const vtkLightActor&) = delete;
};

#endif
