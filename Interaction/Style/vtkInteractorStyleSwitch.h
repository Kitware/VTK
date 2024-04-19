// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkInteractorStyleSwitch
 * @brief   class to swap between interactory styles
 *
 * The class vtkInteractorStyleSwitch allows handles interactively switching
 * between four interactor styles -- joystick actor, joystick camera,
 * trackball actor, and trackball camera.  Type 'j' or 't' to select
 * joystick or trackball, and type 'c' or 'a' to select camera or actor.
 * The default interactor style is joystick camera.
 * @sa
 * vtkInteractorStyleJoystickActor vtkInteractorStyleJoystickCamera
 * vtkInteractorStyleTrackballActor vtkInteractorStyleTrackballCamera
 */

#ifndef vtkInteractorStyleSwitch_h
#define vtkInteractorStyleSwitch_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyleSwitchBase.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALMANUAL

#define VTKIS_JOYSTICK 0
#define VTKIS_TRACKBALL 1

#define VTKIS_CAMERA 0
#define VTKIS_ACTOR 1

VTK_ABI_NAMESPACE_BEGIN
class vtkInteractorStyleJoystickActor;
class vtkInteractorStyleJoystickCamera;
class vtkInteractorStyleTrackballActor;
class vtkInteractorStyleTrackballCamera;
class vtkInteractorStyleMultiTouchCamera;

class VTKINTERACTIONSTYLE_EXPORT VTK_MARSHALMANUAL vtkInteractorStyleSwitch
  : public vtkInteractorStyleSwitchBase
{
public:
  static vtkInteractorStyleSwitch* New();
  vtkTypeMacro(vtkInteractorStyleSwitch, vtkInteractorStyleSwitchBase);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The sub styles need the interactor too.
   */
  void SetInteractor(vtkRenderWindowInteractor* iren) override;

  /**
   * We must override this method in order to pass the setting down to
   * the underlying styles
   */
  void SetAutoAdjustCameraClippingRange(vtkTypeBool value) override;

  ///@{
  /**
   * Set/Get current style
   */
  vtkGetObjectMacro(CurrentStyle, vtkInteractorStyle);
  void SetCurrentStyleToJoystickActor();
  void SetCurrentStyleToJoystickCamera();
  void SetCurrentStyleToTrackballActor();
  void SetCurrentStyleToTrackballCamera();
  void SetCurrentStyleToMultiTouchCamera();
  ///@}

  /**
   * Only care about the char event, which is used to switch between
   * different styles.
   */
  void OnChar() override;

  ///@{
  /**
   * Overridden from vtkInteractorObserver because the interactor styles
   * used by this class must also be updated.
   */
  void SetDefaultRenderer(vtkRenderer*) override;
  void SetCurrentRenderer(vtkRenderer*) override;
  ///@}

protected:
  vtkInteractorStyleSwitch();
  ~vtkInteractorStyleSwitch() override;

  void SetCurrentStyle();

  vtkInteractorStyleJoystickActor* JoystickActor;
  vtkInteractorStyleJoystickCamera* JoystickCamera;
  vtkInteractorStyleTrackballActor* TrackballActor;
  vtkInteractorStyleTrackballCamera* TrackballCamera;
  vtkInteractorStyleMultiTouchCamera* MultiTouchCamera;
  vtkInteractorStyle* CurrentStyle;

  int JoystickOrTrackball;
  int CameraOrActor;
  bool MultiTouch;

private:
  vtkInteractorStyleSwitch(const vtkInteractorStyleSwitch&) = delete;
  void operator=(const vtkInteractorStyleSwitch&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
