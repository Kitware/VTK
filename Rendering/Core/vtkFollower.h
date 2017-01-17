/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFollower.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFollower
 * @brief   a subclass of actor that always faces the camera
 *
 * vtkFollower is a subclass of vtkActor that always follows its specified
 * camera. More specifically it will not change its position or scale,
 * but it will continually update its orientation so that it is right side
 * up and facing the camera. This is typically used for text labels in a
 * scene. All of the adjustments that can be made to an actor also will
 * take effect with a follower.  So, if you change the orientation of the
 * follower by 90 degrees, then it will follow the camera, but be off by
 * 90 degrees.
 *
 * @sa
 * vtkActor vtkCamera vtkAxisFollower vtkProp3DFollower
*/

#ifndef vtkFollower_h
#define vtkFollower_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkActor.h"

class vtkCamera;

class VTKRENDERINGCORE_EXPORT vtkFollower : public vtkActor
{
 public:
  vtkTypeMacro(vtkFollower,vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a follower with no camera set
   */
  static vtkFollower *New();

  //@{
  /**
   * Set/Get the camera to follow. If this is not set, then the follower
   * won't know who to follow.
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
  int RenderOpaqueGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  int RenderTranslucentPolygonalGeometry(vtkViewport *viewport) VTK_OVERRIDE;
  virtual void Render(vtkRenderer *ren);
  //@}

  /**
   * Does this prop have some translucent polygonal geometry?
   */
  int HasTranslucentPolygonalGeometry() VTK_OVERRIDE;

  /**
   * Release any graphics resources associated with this vtkProp3DFollower.
   */
  void ReleaseGraphicsResources(vtkWindow*) VTK_OVERRIDE;

  /**
   * Generate the matrix based on ivars. This method overloads its superclasses
   * ComputeMatrix() method due to the special vtkFollower matrix operations.
   */
  void ComputeMatrix() VTK_OVERRIDE;

  /**
   * Shallow copy of a follower. Overloads the virtual vtkProp method.
   */
  void ShallowCopy(vtkProp *prop) VTK_OVERRIDE;

protected:
  vtkFollower();
  ~vtkFollower() VTK_OVERRIDE;

  vtkCamera *Camera;
  vtkActor  *Device;

private:
  vtkFollower(const vtkFollower&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFollower&) VTK_DELETE_FUNCTION;

  // hide the two parameter Render() method from the user and the compiler.
  void Render(vtkRenderer *, vtkMapper *) VTK_OVERRIDE {}

  //Internal matrices to avoid New/Delete for performance reasons
  vtkMatrix4x4 *InternalMatrix;
};

#endif
