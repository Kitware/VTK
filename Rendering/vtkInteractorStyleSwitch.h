/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSwitch.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
#include "vtkInteractorStyleJoystickActor.h"
#include "vtkInteractorStyleJoystickCamera.h"
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkInteractorStyleTrackballCamera.h"

class VTK_RENDERING_EXPORT vtkInteractorStyleSwitch : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleSwitch *New();
  vtkTypeMacro(vtkInteractorStyleSwitch, vtkInteractorStyle);
  
  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.  The correct subclass method is called
  // depending on the current mode (trackball or joystick, camera
  // or actor).
  void OnLeftButtonDown(int ctrl, int shift, int x, int y);
  void OnLeftButtonUp(int ctrl, int shift, int x, int y);
  void OnMiddleButtonDown(int ctrl, int shift, int x, int y);
  void OnMiddleButtonUp  (int ctrl, int shift, int x, int y);
  void OnRightButtonDown(int ctrl, int shift, int x, int y);
  void OnRightButtonUp  (int ctrl, int shift, int x, int y);
  void OnMouseMove(int ctrl, int shift, int x, int y);
  void OnChar   (int ctrl, int shift, char keycode, int repeatcount);
  
  // Description:
  // The sub styles need the interactor too.
  void SetInteractor(vtkRenderWindowInteractor *iren);
  
  void OnTimer();

  
protected:
  vtkInteractorStyleSwitch();
  ~vtkInteractorStyleSwitch();
  
  vtkInteractorStyleJoystickActor *JoystickActor;
  vtkInteractorStyleJoystickCamera *JoystickCamera;
  vtkInteractorStyleTrackballActor *TrackballActor;
  vtkInteractorStyleTrackballCamera *TrackballCamera;
  
  int JoystickOrTrackball;
  int CameraOrActor;
private:
  vtkInteractorStyleSwitch(const vtkInteractorStyleSwitch&);  // Not implemented.
  void operator=(const vtkInteractorStyleSwitch&);  // Not implemented.
};

#endif
