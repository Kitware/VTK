/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleSphere.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

// .NAME vtkInteractorStyleSphere - provide event driven interface to rendering window

// .SECTION Description
// vtkInteractorStyleSphere is a base class performing the majority of motion control
// routines and am event driven interface to RenderWindowInteractor which
// implements platform dependent key/m0ouse routing and timer control.
//
// vtkInteractorStyleSphere can be subclassed to provide new interaction styles and
// a facility to override any of the default mouse/key operations which
// currently handle trackball or joystick styles is provided
//

#ifndef __vtkInteractorStyleSphere_h
#define __vtkInteractorStyleSphere_h

#include "vtkInteractorStyle.h"
#include "vtkSphereSource.h"
#include "vtkAxes.h"
#include "vtkPolyDataMapper.h"


#define VTK_INTERACTOR_STYLE_SPHERE_NONE    0
#define VTK_INTERACTOR_STYLE_SPHERE_RADIUS  1
#define VTK_INTERACTOR_STYLE_SPHERE_CENTER  2



class VTK_EXPORT vtkInteractorStyleSphere : public vtkInteractorStyle
{
public:
  // Description:
  // This class must be supplied with a vtkRenderWindowInteractor wrapper or
  // parent. This class should not normally be instantiated by application
  // programmers.
  static vtkInteractorStyleSphere *New();

  vtkTypeMacro(vtkInteractorStyleSphere,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Set/Get the sphere parameters.
  void SetCenter(float x, float y, float z);
  float *GetCenter();
  void SetRadius(float rad);
  float GetRadius();
  // until we have a hint.
  float GetCenterX() { return this->GetCenter()[0];}
  float GetCenterY() { return this->GetCenter()[1];}
  float GetCenterZ() { return this->GetCenter()[2];}

  // Generic event bindings must be overridden in subclasses
  void OnMouseMove  (int ctrl, int shift, int X, int Y);
  void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  void OnLeftButtonUp  (int ctrl, int shift, int X, int Y);
  void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  void OnMiddleButtonUp  (int ctrl, int shift, int X, int Y);
  void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  void OnRightButtonUp  (int ctrl, int shift, int X, int Y);

  // Description:
  // OnChar implements keybaord functions, but subclasses can override this 
  // behaviour
  //void OnChar   (int ctrl, int shift, char keycode, int repeatcount);
  //void OnKeyDown(int ctrl, int shift, char keycode, int repeatcount);
  //void OnKeyUp  (int ctrl, int shift, char keycode, int repeatcount);

  // Description:
  // Specify function to be called when the sphere changes.
  void SetChangeMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetChangeMethodArgDelete(void (*f)(void *));

protected:
  vtkInteractorStyleSphere();
  ~vtkInteractorStyleSphere();
  vtkInteractorStyleSphere(const vtkInteractorStyleSphere&) {};
  void operator=(const vtkInteractorStyleSphere&) {};

  vtkSphereSource    *SphereSource;
  vtkPolyDataMapper  *SphereMapper;
  vtkActor           *SphereActor;

  vtkAxes            *CenterSource;
  vtkPolyDataMapper  *CenterMapper;
  vtkActor           *CenterActor;

  // State of the button -1 => none pressed
  int Button;
  int State;

  void (*ChangeMethod)(void *);
  void (*ChangeMethodArgDelete)(void *);
  void *ChangeMethodArg;

  void MoveCenterXY(int dx, int dy);
  void MoveCenterZ(int dx, int dy);
  void MoveRadius(int x, int y);
  void HandleIndicator(int x, int y);

};

#endif
