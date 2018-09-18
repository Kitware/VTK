/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProp3DFollower.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkProp3DFollower
 * @brief   a vtkProp3D that always faces the camera
 *
 * vtkProp3DFollower is a type of vtkProp3D that always faces the camera.
 * More specifically it will not change its position or scale,
 * but it will continually update its orientation so that it is right side
 * up and facing the camera. This is typically used for complex billboards
 * or props that need to face the viewer at all times.
 *
 * Note: All of the transformations that can be made to a vtkProp3D will take
 * effect with the follower. Thus, if you change the orientation of the
 * follower by 90 degrees, then it will follow the camera, but be off by 90
 * degrees.
 *
 * @sa
 * vtkFollower vtkProp3D vtkCamera vtkProp3DAxisFollower
*/

#ifndef vtkProp3DFollower_h
#define vtkProp3DFollower_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkProp3D.h"

class vtkCamera;
class vtkMapper;


class VTKRENDERINGCORE_EXPORT vtkProp3DFollower : public vtkProp3D
{
 public:
  /**
   * Creates a follower with no camera set.
   */
  static vtkProp3DFollower *New();

  //@{
  /**
   * Standard VTK methods for type and printing.
   */
  vtkTypeMacro(vtkProp3DFollower,vtkProp3D);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Set/Get the vtkProp3D to control (i.e., face the camera).
   */
  virtual void SetProp3D(vtkProp3D *prop);
  virtual vtkProp3D *GetProp3D();
  //@}

  //@{
  /**
   * Set/Get the camera to follow. If this is not set, then the follower
   * won't know what to follow and will act like a normal vtkProp3D.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera, vtkCamera);
  //@}

  //@{
  /**
   * This causes the actor to be rendered. It in turn will render the actor's
   * property, texture map and then mapper. If a property hasn't been
   * assigned, then the actor will create one automatically.
   */
  int RenderOpaqueGeometry(vtkViewport *viewport) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) override;
  int RenderVolumetricGeometry(vtkViewport *viewport) override;
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  vtkTypeBool HasTranslucentPolygonalGeometry() override;

  /**
   * Release any graphics resources associated with this vtkProp3DFollower.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special vtkProp3DFollower matrix operations.
   */
  void ComputeMatrix() override;

  /**
   * Shallow copy of a follower. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp *prop) override;

  /**
   * Return the bounds of this vtkProp3D.
   */
  double *GetBounds() override;

  //@{
  /**
   * Overload vtkProp's method for setting up assembly paths. See
   * the documentation for vtkProp.
   */
  void InitPathTraversal() override;
  vtkAssemblyPath *GetNextPath() override;
  //@}

protected:
  vtkProp3DFollower();
  ~vtkProp3DFollower() override;

  vtkCamera *Camera;
  vtkProp3D  *Device;

  //Internal matrices to avoid New/Delete for performance reasons
  vtkMatrix4x4 *InternalMatrix;

private:
  vtkProp3DFollower(const vtkProp3DFollower&) = delete;
  void operator=(const vtkProp3DFollower&) = delete;
};

#endif
