/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackballActor.h
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
// .NAME vtkInteractorStyleTrackballActor - manipulate objects in the scene independent of each other
// .SECTION Description
// vtkInteractorStyleTrackballActor allows the user to interact with (rotate,
// pan, etc.) objects in the scene indendent of each other.  The position
// of the mouse relative to the center of the object determine's the speed
// at which the object moves.  When the mouse stops moving, so does the
// object being manipulated.
// For a 3-button mouse, the left button is for rotation, the right button
// for zooming, the middle button for panning, and ctrl + left button for
// spinning.  (With fewer mouse buttons, ctrl + shift + left button is
// for zooming, and shift + left button is for panning.)

// .SECTION See Also
// vtkInteractorStyleTrackballCamera vtkInteractorStyleJoystickActor
// vtkInteractorStyleJoystickCamera

#ifndef __vtkInteractorStyleTrackballActor_h
#define __vtkInteractorStyleTrackballActor_h

#include "vtkInteractorStyle.h"
#include "vtkCellPicker.h"

#define VTK_INTERACTOR_STYLE_ACTOR_NONE    0
#define VTK_INTERACTOR_STYLE_ACTOR_ROTATE  1
#define VTK_INTERACTOR_STYLE_ACTOR_PAN     2
#define VTK_INTERACTOR_STYLE_ACTOR_ZOOM    3
#define VTK_INTERACTOR_STYLE_ACTOR_SPIN    4
#define VTK_INTERACTOR_STYLE_ACTOR_SCALE   5

class VTK_RENDERING_EXPORT vtkInteractorStyleTrackballActor : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleTrackballActor *New();
  vtkTypeMacro(vtkInteractorStyleTrackballActor, vtkObject);
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

protected:
  vtkInteractorStyleTrackballActor();
  ~vtkInteractorStyleTrackballActor();

  void RotateXY(int x, int y, int oldX, int oldY);
  void PanXY(int x, int y, int oldX, int oldY);
  void DollyXY(int dx, int dy);
  void SpinXY(int dx, int dy, int oldX, int oldY);
  void ScaleXY(int x, int y, int oldX, int oldY);
  void FindPickedActor(int x, int y);
  void Prop3DTransform(vtkProp3D *prop3D, double *boxCenter,
                      int numRotation, double **rotate,
                      double *scale);
  void Prop3DTransform(vtkProp3D *prop3D,float *boxCenter,
                      int NumRotation,double **rotate,
                      double *scale);
  
  int State;
  float MotionFactor;
  float RadianToDegree;                 // constant: for conv from deg to rad
  vtkProp3D *InteractionProp;
  double ViewUp[3];
  double ViewLook[3];
  double ViewRight[3];
  float ObjCenter[3];
  float DispObjCenter[3];
  float Radius;
  float NewPickPoint[4];
  float OldPickPoint[4];
  float MotionVector[3];
  double ViewPoint[3];
  double ViewFocus[3];
//  vtkAbstractPropPicker *InteractionPicker;
  vtkCellPicker *InteractionPicker;
private:
  vtkInteractorStyleTrackballActor(const vtkInteractorStyleTrackballActor&);  // Not implemented.
  void operator=(const vtkInteractorStyleTrackballActor&);  // Not implemented.
};

#endif
