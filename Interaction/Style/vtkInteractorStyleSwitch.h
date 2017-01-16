/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitch.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#define VTKIS_JOYSTICK  0
#define VTKIS_TRACKBALL 1

#define VTKIS_CAMERA    0
#define VTKIS_ACTOR     1

class vtkInteractorStyleJoystickActor;
class vtkInteractorStyleJoystickCamera;
class vtkInteractorStyleTrackballActor;
class vtkInteractorStyleTrackballCamera;
class vtkInteractorStyleMultiTouchCamera;

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleSwitch
  : public vtkInteractorStyleSwitchBase
{
public:
  static vtkInteractorStyleSwitch *New();
  vtkTypeMacro(vtkInteractorStyleSwitch, vtkInteractorStyleSwitchBase);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * The sub styles need the interactor too.
   */
  void SetInteractor(vtkRenderWindowInteractor *iren) VTK_OVERRIDE;

  /**
   * We must override this method in order to pass the setting down to
   * the underlying styles
   */
  void SetAutoAdjustCameraClippingRange( int value ) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get current style
   */
  vtkGetObjectMacro(CurrentStyle, vtkInteractorStyle);
  void SetCurrentStyleToJoystickActor();
  void SetCurrentStyleToJoystickCamera();
  void SetCurrentStyleToTrackballActor();
  void SetCurrentStyleToTrackballCamera();
  void SetCurrentStyleToMultiTouchCamera();
  //@}

  /**
   * Only care about the char event, which is used to switch between
   * different styles.
   */
  void OnChar() VTK_OVERRIDE;

  //@{
  /**
   * Overridden from vtkInteractorObserver because the interactor styles
   * used by this class must also be updated.
   */
  void SetDefaultRenderer(vtkRenderer*) VTK_OVERRIDE;
  void SetCurrentRenderer(vtkRenderer*) VTK_OVERRIDE;
  //@}

protected:
  vtkInteractorStyleSwitch();
  ~vtkInteractorStyleSwitch() VTK_OVERRIDE;

  void SetCurrentStyle();

  vtkInteractorStyleJoystickActor *JoystickActor;
  vtkInteractorStyleJoystickCamera *JoystickCamera;
  vtkInteractorStyleTrackballActor *TrackballActor;
  vtkInteractorStyleTrackballCamera *TrackballCamera;
  vtkInteractorStyleMultiTouchCamera *MultiTouchCamera;
  vtkInteractorStyle* CurrentStyle;

  int JoystickOrTrackball;
  int CameraOrActor;
  bool MultiTouch;

private:
  vtkInteractorStyleSwitch(const vtkInteractorStyleSwitch&) VTK_DELETE_FUNCTION;
  void operator=(const vtkInteractorStyleSwitch&) VTK_DELETE_FUNCTION;
};

#endif
