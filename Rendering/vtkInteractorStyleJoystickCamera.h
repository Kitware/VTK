/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleJoystickCamera.h
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
// .NAME vtkInteractorStyleJoystickCamera - interactive manipulation of the camera

// .SECTION Description
// vtkInteractorStyleJoystickCamera allows the user to move (rotate, pan,
// etc.) the camera, the point of view for the scene.  The position of the
// mouse relative to the center of the scene determines the speed at which
// the camera moves, and the speed of the mouse movement determines the
// acceleration of the camera, so the camera continues to move even if the
// mouse if not moving.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)

// .SECTION See Also
// vtkInteractorStyleJoystickActor vtkInteractorStyleTrackballCamera
// vtkInteractorStyleTrackballActor

#ifndef __vtkInteractorStyleJoystickCamera_h
#define __vtkInteractorStyleJoystickCamera_h

#include "vtkInteractorStyle.h"


#define VTK_INTERACTOR_STYLE_CAMERA_NONE    0
#define VTK_INTERACTOR_STYLE_CAMERA_ROTATE  1
#define VTK_INTERACTOR_STYLE_CAMERA_PAN     2
#define VTK_INTERACTOR_STYLE_CAMERA_ZOOM    3
#define VTK_INTERACTOR_STYLE_CAMERA_SPIN    4

class VTK_EXPORT vtkInteractorStyleJoystickCamera : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleJoystickCamera *New();
  vtkTypeMacro(vtkInteractorStyleJoystickCamera, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
  void OnMouseMove  (int ctrl, int shift, int x, int y);
  void OnLeftButtonDown(int ctrl, int shift, int x, int y);
  void OnLeftButtonUp  (int ctrl, int shift, int x, int y);
  void OnMiddleButtonDown(int ctrl, int shift, int x, int y);
  void OnMiddleButtonUp  (int ctrl, int shift, int x, int y);
  void OnRightButtonDown(int ctrl, int shift, int x, int y);
  void OnRightButtonUp  (int ctrl, int shift, int x, int y);
  void OnTimer(void); 

protected:
  vtkInteractorStyleJoystickCamera();
  ~vtkInteractorStyleJoystickCamera();
  vtkInteractorStyleJoystickCamera(const vtkInteractorStyleJoystickCamera&);
  void operator=(const vtkInteractorStyleJoystickCamera&);
  
  int State;
  float MotionFactor;
};

#endif
