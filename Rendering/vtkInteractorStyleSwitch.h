/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitch.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleSwitch - class to swap between interactory styles
// .SECTION Description
// The class vtkInteractorStyleSwitch allows handles interactively switching
// between four interactor styles -- joystick actor, joystick camera,
// trackball actor, and trackball camera.  Type 'j' or 't' to select
// joystick or trackball, and type 'c' or 'a' to select camera or actor.
// The default interactor style is joystick camera.
// .SECTION See Also
// vtkInteractorStyleJoystickActor vtkInteractorStyleJoystickCamera
// vtkInteractorStyleTrackballActor vtkInteractorStyleTrackballCamera

#ifndef __vtkInteractorStyleSwitch_h
#define __vtkInteractorStyleSwitch_h

#define VTKIS_JOYSTICK 0
#define VTKIS_TRACKBALL 1
#define VTKIS_CAMERA 0
#define VTKIS_ACTOR 1

#include "vtkInteractorStyle.h"
class vtkInteractorStyleJoystickActor;
class vtkInteractorStyleJoystickCamera;
class vtkInteractorStyleTrackballActor;
class vtkInteractorStyleTrackballCamera;

class VTK_RENDERING_EXPORT vtkInteractorStyleSwitch : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleSwitch *New();
  vtkTypeRevisionMacro(vtkInteractorStyleSwitch, vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);
  // Description:
  // Only care about the char event, which is used to switch between
  // different styles.
  void OnChar   (int ctrl, int shift, char keycode, int repeatcount);
  
  // Description:
  // The sub styles need the interactor too.
  void SetInteractor(vtkRenderWindowInteractor *iren);
  
  // Description:
  // We must override this method in order to pass the setting down to
  // the underlying styles
  void SetAutoAdjustCameraClippingRange( int value );
  
  // Description:
  // Set/Get current style
  vtkGetObjectMacro(CurrentStyle, vtkInteractorStyle);
  void SetCurrentStyleToJoystickActor();
  void SetCurrentStyleToJoystickCamera();
  void SetCurrentStyleToTrackballActor();
  void SetCurrentStyleToTrackballCamera();

protected:
  vtkInteractorStyleSwitch();
  ~vtkInteractorStyleSwitch();
  
  void SetCurrentStyle();
  
  vtkInteractorStyleJoystickActor *JoystickActor;
  vtkInteractorStyleJoystickCamera *JoystickCamera;
  vtkInteractorStyleTrackballActor *TrackballActor;
  vtkInteractorStyleTrackballCamera *TrackballCamera;
  vtkInteractorStyle* CurrentStyle;
  int JoystickOrTrackball;
  int CameraOrActor;
private:
  vtkInteractorStyleSwitch(const vtkInteractorStyleSwitch&);  // Not implemented.
  void operator=(const vtkInteractorStyleSwitch&);  // Not implemented.
};

#endif
