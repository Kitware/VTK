/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStylePlane.h
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

// .NAME vtkInteractorStylePlane - Manipulation of plane source.

// .SECTION Description
// vtkInteractorStylePlane Allows interactive definition of a plane
// by manipulating a vtkPlaneSources parameters.  
//

// SetNormal(), SetCenter() and Push()
//

#ifndef __vtkInteractorStylePlane_h
#define __vtkInteractorStylePlane_h

#include "vtkInteractorStyle.h"
#include "vtkPlaneSource.h"
#include "vtkSphereSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkTransform.h"


#define VTK_INTERACTOR_STYLE_PLANE_NONE    0
#define VTK_INTERACTOR_STYLE_PLANE_CENTER  1
#define VTK_INTERACTOR_STYLE_PLANE_CORNER  2



class VTK_EXPORT vtkInteractorStylePlane : public vtkInteractorStyle
{
public:
  // Description:
  // This class must be supplied with a vtkRenderWindowInteractor wrapper or
  // parent. This class should not normally be instantiated by application
  // programmers.
  static vtkInteractorStylePlane *New();

  vtkTypeMacro(vtkInteractorStylePlane,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Parameters are accessed through the plane source.
  vtkGetObjectMacro(PlaneSource, vtkPlaneSource);
  vtkGetObjectMacro(PlaneActor, vtkActor);

  // Generic event bindings must be overridden in subclasses
  void OnMouseMove  (int ctrl, int shift, int x, int y);
  void OnLeftButtonDown(int ctrl, int shift, int x, int y);
  void OnLeftButtonUp  (int ctrl, int shift, int x, int y);
  void OnMiddleButtonDown(int ctrl, int shift, int x, int y);
  void OnMiddleButtonUp  (int ctrl, int shift, int x, int y);
  void OnRightButtonDown(int ctrl, int shift, int x, int y);
  void OnRightButtonUp  (int ctrl, int shift, int x, int y);

  // Description:
  // OnChar implements keybaord functions, but subclasses can override this 
  // behaviour
  //void OnChar   (int ctrl, int shift, char keycode, int repeatcount);
  //void OnKeyDown(int ctrl, int shift, char keycode, int repeatcount);
  //void OnKeyUp  (int ctrl, int shift, char keycode, int repeatcount);

  // Description:
  // Specify function to be called when a significant event occurs.
  // the instance variable "CallbackType" will be set to one of the 
  // following strings: "InteractiveChange" "ButtonRelease"
  void SetCallbackMethod(void (*f)(void *), void *arg);
  vtkGetStringMacro(CallbackType);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetCallbackMethodArgDelete(void (*f)(void *));

  // Description:
  // The CallbackMethod
  void DefaultCallback(char *type);

protected:
  vtkInteractorStylePlane();
  ~vtkInteractorStylePlane();
  vtkInteractorStylePlane(const vtkInteractorStylePlane&) {};
  void operator=(const vtkInteractorStylePlane&) {};

  vtkSphereSource    *SphereSource;
  vtkPolyDataMapper  *SphereMapper;
  vtkActor           *SphereActor;

  vtkPlaneSource     *PlaneSource;
  vtkPolyDataMapper  *PlaneMapper;
  vtkActor           *PlaneActor;

  // State of the button -1 => none pressed
  int Button;
  // Indicates which type of hot spot is active (corner or center).
  int State;
  // Indicates which corner is active.
  int ActiveCornerId;
  // The poisiton of the active corner
  float ActiveCorner[3];

  vtkTransform *Transform;

  void (*CallbackMethod)(void *);
  void *CallbackMethodArg;
  char *CallbackType;
  void (*CallbackMethodArgDelete)(void *);

  // Ideally we would make the CallbackType a 
  // parameter of the Callback method.
  vtkSetStringMacro(CallbackType);

  // Actions of the three buttons on the center hot spot.
  void TranslateXY(int dx, int dy);
  void TranslateZ(int dx, int dy);
  void RotateXY(int dx, int dy);

  void ResizeCorner(int dx, int dy);

  void HandleIndicator(int x, int y);

};

#endif
