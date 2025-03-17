// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkTextActorInterfacePrivate_h
#define vtkTextActorInterfacePrivate_h

#include "vtkNew.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN
class vtkAxisFollower;
class vtkCamera;
class vtkPolyDataMapper;
class vtkProp;
class vtkProp3DAxisFollower;
class vtkPropCollection;
class vtkProperty;
class vtkTextActor3D;
class vtkTextActor;
class vtkTextProperty;
class vtkVectorText;

/**
 * VTK Private interface to manipulate text actors from vtkAxisActor.
 * The generated text is either a vector or a rasterized version.
 * Display can be done screen space or in 3D scene, using the appropriate (Axis)Follower.
 *
 * @see GetActiveProp
 */
class vtkTextActorInterfacePrivate
{
public:
  vtkTextActorInterfacePrivate();
  virtual ~vtkTextActorInterfacePrivate();

  /**
   * Set the text to be displayed.
   */
  void SetInputText(const std::string& text);

  /**
   * Set the current camera. Useful when using a Follower.
   */
  void SetCamera(vtkCamera* camera);

  /**
   * Get the vtkProp to be used in the given mode:
   * if overlay is true, return the vtkTextActor (rasterized text in screen space)
   * else if vector is true, return the vtkAxisFollower (vectorized text in 3D scene)
   * else return the vtkProp3DAxisFollower (rasterized text in 3D scene)
   */
  vtkProp* GetActiveProp(bool overlay, bool vector);

  /**
   * Update the actors from the given text property.
   */
  void UpdateProperty(vtkTextProperty* textProp, vtkProperty* prop);

  /**
   * Fill the collection with all vtkProp members.
   */
  void GetActors(vtkPropCollection* collection);

  vtkNew<vtkVectorText> Vector;

  vtkNew<vtkAxisFollower> Follower;
  vtkNew<vtkProp3DAxisFollower> Follower3D;
  vtkNew<vtkTextActor> Actor2D;
  vtkNew<vtkTextActor3D> Actor3D;

  vtkTextActorInterfacePrivate(vtkTextActorInterfacePrivate&&);

private:
  void operator=(const vtkTextActorInterfacePrivate&) = delete;
  vtkTextActorInterfacePrivate(const vtkTextActorInterfacePrivate&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
