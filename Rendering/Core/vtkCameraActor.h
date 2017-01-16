/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkCamera;
class vtkFrustumSource;
class vtkPolyDataMapper;
class vtkActor;
class vtkProperty;

class VTKRENDERINGCORE_EXPORT vtkCameraActor : public vtkProp3D
{
public:
  static vtkCameraActor *New();
  vtkTypeMacro(vtkCameraActor, vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * The camera to represent. Initial value is NULL.
   */
  void SetCamera(vtkCamera *camera);
  vtkGetObjectMacro(Camera, vtkCamera);
  //@}

  //@{
  /**
   * Ratio between the width and the height of the frustum. Initial value is
   * 1.0 (square)
   */
  vtkSetMacro(WidthByHeightRatio, double);
  vtkGetMacro(WidthByHeightRatio, double);
  //@}

  /**
   * Support the standard render methods.
   */
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;

  /**
   * Does this prop have some translucent polygonal geometry? No.
   */
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this actor.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  /**
   * Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
   */
  double *GetBounds() VTK_OVERRIDE;

  /**
   * Get the actors mtime plus consider its properties and texture if set.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  /**
   * Get property of the internal actor.
   */
  vtkProperty *GetProperty();

  /**
   * Set property of the internal actor.
   */
  void SetProperty(vtkProperty *p);

protected:
  vtkCameraActor();
  ~vtkCameraActor() VTK_OVERRIDE;

  void UpdateViewProps();

  vtkCamera *Camera;
  double WidthByHeightRatio;

  vtkFrustumSource *FrustumSource;
  vtkPolyDataMapper *FrustumMapper;
  vtkActor *FrustumActor;

private:
  vtkCameraActor(const vtkCameraActor&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCameraActor&) VTK_DELETE_FUNCTION;
};

#endif
