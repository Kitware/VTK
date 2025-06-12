// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#ifndef vtkFollowerTextPropertyAdaptor_h
#define vtkFollowerTextPropertyAdaptor_h

#include "vtkCallbackCommand.h"
#include "vtkNew.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkAxisFollower;
class vtkProperty;
class vtkProp3DAxisFollower;
class vtkTextProperty;

/**
 * vtkFollowerTextPropertyAdaptor is a helper class to mimic vtkTextProperty
 * behavior for a vtkAxisFollower.
 *
 * The vtkAxisFollower does not have a vtkTextProperty.
 * Mimic it by copying the relevant properties (like color and opacity)
 * Also connect the ModifiedEvent of the text property to the Modified
 * method of the followers, to ensure updates.
 */
class vtkFollowerTextPropertyAdaptor
{
public:
  vtkFollowerTextPropertyAdaptor(vtkAxisFollower* follower, vtkProp3DAxisFollower* propFollower);
  virtual ~vtkFollowerTextPropertyAdaptor();

  /**
   * Call Modified on the vtkAxisActor of the follower, to trigger a render.
   * This should be connected to the vtkTextProperty ModifiedEvent, with
   * clientData set to be a vtkAxisFollower.
   */
  static void OnModified(vtkObject*, unsigned long, void* clientData, void*);

  /**
   * DeepCopy actorProperty. Override the relevant properties with the ones
   * form the vtkTextProeprty (like color and opacity).
   *
   * Also connect the ModifiedEvent of the text property to the Modified
   * method of the follower, to ensure updates.
   */
  void UpdateProperty(vtkTextProperty* textProp, vtkProperty* actorProperty);

  /**
   * Update the scale, taking the FontScale into account.
   *
   * As vtkAxisFollower as no text property, the font size is set as part
   * of the global scale, and should be taken into account when changing global scale.
   */
  void SetScale(double scale);

private:
  vtkAxisFollower* Follower;
  vtkProp3DAxisFollower* PropFollower;
  double FontScale = 1;

  unsigned int TextPropObserverId = 0;
  vtkNew<vtkCallbackCommand> ModifiedCallback;
};

VTK_ABI_NAMESPACE_END
#endif
