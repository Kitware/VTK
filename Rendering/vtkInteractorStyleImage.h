/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleImage.h
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
// .NAME vtkInteractorStyleImage - interactive manipulation of the camera
// .SECTION Description
// vtkInteractorStyleImage allows the user to interactively
// manipulate (rotate, pan, etc.) the camera, the viewpoint of the scene.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)

// .SECTION See Also
// vtkInteractorStyleTrackballActor vtkInteractorStyleJoystickCamera
// vtkInteractorStyleJoystickActor

#ifndef __vtkInteractorStyleImage_h
#define __vtkInteractorStyleImage_h

#include "vtkInteractorStyle.h"


#define VTK_INTERACTOR_STYLE_IMAGE_NONE    0
#define VTK_INTERACTOR_STYLE_IMAGE_WINDOW_LEVEL  1
#define VTK_INTERACTOR_STYLE_IMAGE_PAN     2
#define VTK_INTERACTOR_STYLE_IMAGE_ZOOM    3
#define VTK_INTERACTOR_STYLE_IMAGE_SPIN    4

class VTK_RENDERING_EXPORT vtkInteractorStyleImage : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleImage *New();
  vtkTypeMacro(vtkInteractorStyleImage, vtkObject);
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

  // Description:
  // Some useful information for handling window level
  vtkGetVector2Macro(WindowLevelStartPosition,int);
  vtkGetVector2Macro(WindowLevelCurrentPosition,int);
  
protected:
  vtkInteractorStyleImage();
  ~vtkInteractorStyleImage();
  vtkInteractorStyleImage(const vtkInteractorStyleImage&);
  void operator=(const vtkInteractorStyleImage&);

  void WindowLevelXY(int dx, int dy);
  void PanXY(int x, int y, int oldX, int oldY);
  void DollyXY(int dx, int dy);
  void SpinXY(int dx, int dy, int oldX, int oldY);
  
  int WindowLevelStartPosition[2];
  int WindowLevelCurrentPosition[2];
  int State;
  float MotionFactor;
  float RadianToDegree; // constant: for conv from deg to rad
};

#endif
