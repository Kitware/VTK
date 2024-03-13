// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleTrackballActor
 * @brief   manipulate objects in the scene independent of each other
 *
 * vtkInteractorStyleTrackballActor allows the user to interact with (rotate,
 * pan, etc.) objects in the scene independent of each other.  In trackball
 * interaction, the magnitude of the mouse motion is proportional to the
 * actor motion associated with a particular mouse binding. For example,
 * small left-button motions cause small changes in the rotation of the
 * actor around its center point.
 *
 * The mouse bindings are as follows. For a 3-button mouse, the left button
 * is for rotation, the right button for zooming, the middle button for
 * panning, and ctrl + left button for spinning.  (With fewer mouse buttons,
 * ctrl + shift + left button is for zooming, and shift + left button is for
 * panning.)
 *
 * @sa
 * vtkInteractorStyleTrackballCamera vtkInteractorStyleJoystickActor
 * vtkInteractorStyleJoystickCamera
 */

#ifndef vtkInteractorStyleTrackballActor_h
#define vtkInteractorStyleTrackballActor_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCellPicker;

class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALAUTO vtkInteractorStyleTrackballActor
  : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballActor* New();
  vtkTypeMacro(vtkInteractorStyleTrackballActor, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Event bindings controlling the effects of pressing mouse buttons
   * or moving the mouse.
   */
  void OnMouseMove() override;
  void OnLeftButtonDown() override;
  void OnLeftButtonUp() override;
  void OnMiddleButtonDown() override;
  void OnMiddleButtonUp() override;
  void OnRightButtonDown() override;
  void OnRightButtonUp() override;
  ///@}

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  void Rotate() override;
  void Spin() override;
  void Pan() override;
  void Dolly() override;
  void UniformScale() override;

protected:
  vtkInteractorStyleTrackballActor();
  ~vtkInteractorStyleTrackballActor() override;

  void FindPickedActor(int x, int y);

  void Prop3DTransform(
    vtkProp3D* prop3D, double* boxCenter, int NumRotation, double** rotate, double* scale);

  double MotionFactor;

  vtkProp3D* InteractionProp;
  vtkCellPicker* InteractionPicker;

private:
  vtkInteractorStyleTrackballActor(const vtkInteractorStyleTrackballActor&) = delete;
  void operator=(const vtkInteractorStyleTrackballActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
