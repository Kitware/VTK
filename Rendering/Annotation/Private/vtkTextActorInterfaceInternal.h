// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkTextActorInterfaceInternal_h
#define vtkTextActorInterfaceInternal_h

#include "vtkNew.h"
#include "vtkSmartPointer.h"

#include <string>

VTK_ABI_NAMESPACE_BEGIN

class vtkAxisActor;
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
 * VTK Internal interface to manipulate text actors from vtkAxisActor.
 * The generated text is either a vector or a rasterized version.
 * Display can be done screen space or in 3D scene, using the appropriate (Axis)Follower.
 *
 * @see GetActiveProp
 */
class vtkTextActorInterfaceInternal
{
public:
  vtkTextActorInterfaceInternal();
  virtual ~vtkTextActorInterfaceInternal();

  /**
   * Set the text to be displayed.
   */
  void SetInputText(const std::string& text);
  std::string GetInputText();

  /**
   * Set the current camera. Useful when using a Follower.
   */
  void SetCamera(vtkCamera* camera);

  /**
   * Set the axis to follow.
   */
  void SetAxis(vtkAxisActor* axis);

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
   * Set ambient coefficient. Should be between 0 and 1.
   */
  void SetAmbient(double amb);

  /**
   * Set diffuse coefficient. Should be between 0 and 1.
   */
  void SetDiffuse(double diffuse);

  /**
   * Fill the collection with all vtkProp members.
   */
  void GetActors(vtkPropCollection* collection);

  /**
   * Set scale on the Follower and Follower3D.
   * Note that Actor3D has its own scale, computed in AdjustScale.
   */
  void SetScale(double s);

  /**
   * Adjust the scale of Actor3D, based on Follower mapper bounds,
   * so switching them provide consistent size on screen.
   * Follower3D also has its scale, externally set in SetScale.
   */
  void AdjustScale();

  /**
   * Get the bounds of the text
   */
  void GetBounds(double bounds[6]);

  /**
   * Update internal 3D actors position.
   */
  void SetPosition(double pos[3]);

  /**
   * Get the reference position in 3D space.
   */
  void GetReferencePosition(double pos[3]);

  /**
   * Update internal screen space actors position.
   */
  void SetDisplayPosition(double x, double y);

  /**
   * Set orientation of the actor 2D to keep the axis orientation.
   * Axis is defined by p1 and p2.
   */
  void RotateActor2DFromAxisProjection(double p1[3], double p2[3]);

  ///@{
  /**
   * Set Screen offset on axis followers.
   */
  void SetScreenOffset(double offset);
  void SetScreenOffsetVector(double offset[2]);
  ///@}

  /**
   * Return the internal follower 3D.
   * This is here for backward compatibility, please do not add usage.
   */
  // VTK_DEPRECATED_IN_9_5_0
  vtkProp3DAxisFollower* GetFollower3D() const;

  /**
   * Return the internal follower.
   * This is here for backward compatibility, please do not add usage.
   */
  // VTK_DEPRECATED_IN_9_5_0
  vtkAxisFollower* GetFollower() const;

private:
  void operator=(const vtkTextActorInterfaceInternal&) = delete;
  vtkTextActorInterfaceInternal(const vtkTextActorInterfaceInternal&) = delete;

  vtkNew<vtkVectorText> Vector;

  vtkNew<vtkTextActor> Actor2D;
  vtkNew<vtkTextActor3D> Actor3D;
  vtkNew<vtkProp3DAxisFollower> Follower3D;
  vtkNew<vtkAxisFollower> Follower;

  vtkSmartPointer<vtkCamera> Camera;
};

VTK_ABI_NAMESPACE_END
#endif
