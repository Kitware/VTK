/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle.h
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

// .NAME vtkInteractorStyle - provide event driven interface to rendering window

// .SECTION Description
// vtkInteractorStyle is a base class performing the majority of motion control
// routines and am event driven interface to RenderWindowInteractor which
// implements platform dependent key/m0ouse routing and timer control.
//
// vtkInteractorStyle can be subclassed to provide new interaction styles and
// a facility to override any of the default mouse/key operations which
// currently handle trackball or joystick styles is provided
//

#ifndef __vtkInteractorStyle_h
#define __vtkInteractorStyle_h

#include "vtkRenderWindowInteractor.h"

// motion flags
#define VTKIS_START   0
#define VTKIS_ROTATE 1
#define VTKIS_ZOOM   2
#define VTKIS_PAN    3
#define VTKIS_SPIN   4
#define VTKIS_DOLLY  5
#define VTKIS_USCALE 6
#define VTKIS_TIMER  7 
#define VTKIS_ANIM_OFF 0
#define VTKIS_ANIM_ON  1

class vtkPolyDataMapper;
class vtkOutlineSource;

class VTK_EXPORT vtkInteractorStyle : public vtkObject 
{
public:
  // Description:
  // This class must be supplied with a vtkRenderWindowInteractor wrapper or
  // parent. This class should not normally be instantiated by application
  // programmers.
  static vtkInteractorStyle *New();

  vtkTypeMacro(vtkInteractorStyle,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Interactor wrapper being controlled by this object.
  void SetInteractor(vtkRenderWindowInteractor *interactor);
  vtkGetObjectMacro(Interactor, vtkRenderWindowInteractor);

  // Description:
  // When an event occurs, we must determine which Renderer the event
  // occurred within, since one RenderWindow may contain multiple
  // renderers. We also need to know what camera to operate on.
  // This is just the ActiveCamera of the poked renderer.
  void FindPokedCamera(int,int);
  void FindPokedRenderer(int,int);

  // Description:
  // When picking successfully selects an actor, this method highlights the
  // picked prop appropriately. Currently this is done by placing a bounding 
  // box around a picked vtkProp3D, and using the PickColor to highlight a
  // vtkProp2D. 
  virtual void HighlightProp(vtkProp *prop);
  virtual void HighlightActor2D(vtkActor2D *actor2D);
  virtual void HighlightProp3D(vtkProp3D *prop3D);

  // Description:
  // Set/Get the pick color (used by default to color vtkActor2D's).
  // The color is expressed as red/green/blue values between (0.0,1.0).
  vtkSetVector3Macro(PickColor,float);
  vtkGetVectorMacro(PickColor, float, 3);

  // Description:
  // Generic event bindings must be overridden in subclasses
  virtual void OnMouseMove  (int ctrl, int shift, int X, int Y);
  virtual void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnLeftButtonUp  (int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnMiddleButtonUp  (int ctrl, int shift, int X, int Y);
  virtual void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  virtual void OnRightButtonUp  (int ctrl, int shift, int X, int Y);

  // Description:
  // OnChar implements keybaord functions, but subclasses can override this 
  // behaviour
  virtual void OnChar   (int ctrl, int shift, char keycode, int repeatcount);
  virtual void OnKeyDown(int ctrl, int shift, char keycode, int repeatcount);
  virtual void OnKeyUp  (int ctrl, int shift, char keycode, int repeatcount);

  // Description:
  // OnTimer calls RotateCamera, RotateActor etc which should be overridden by
  // style subclasses.
  virtual void OnTimer();

  // Description:
  // Callbacks so that the applicaiton can override the default behaviour.
  void SetLeftButtonPressMethod(void (*f)(void *), void *arg);
  void SetLeftButtonPressMethodArgDelete(void (*f)(void *));
  void SetLeftButtonReleaseMethod(void (*f)(void *), void *arg);
  void SetLeftButtonReleaseMethodArgDelete(void (*f)(void *));
  void SetMiddleButtonPressMethod(void (*f)(void *), void *arg);
  void SetMiddleButtonPressMethodArgDelete(void (*f)(void *));
  void SetMiddleButtonReleaseMethod(void (*f)(void *), void *arg);
  void SetMiddleButtonReleaseMethodArgDelete(void (*f)(void *));
  void SetRightButtonPressMethod(void (*f)(void *), void *arg);
  void SetRightButtonPressMethodArgDelete(void (*f)(void *));
  void SetRightButtonReleaseMethod(void (*f)(void *), void *arg);
  void SetRightButtonReleaseMethodArgDelete(void (*f)(void *));

protected:
  vtkInteractorStyle();
  ~vtkInteractorStyle();
  vtkInteractorStyle(const vtkInteractorStyle&) {};
  void operator=(const vtkInteractorStyle&) {};

  // convenience methods for converting between coordinate systems
  virtual void ComputeDisplayToWorld(double x, double y, double z,
                                     double *worldPt);
  virtual void ComputeWorldToDisplay(double x, double y, double z,
                                     double *displayPt);
  virtual void ComputeDisplayToWorld(double x, double y, double z,
                                     float *worldPt);
  virtual void ComputeWorldToDisplay(double x, double y, double z,
                                     float *displayPt);

  virtual void UpdateInternalState(int ctrl, int shift, int X, int Y);

  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion
  // This class provides a default implementation.

  virtual void RotateCamera(int x, int y);
  virtual void SpinCamera(int x, int y);
  virtual void PanCamera(int x, int y);
  virtual void DollyCamera(int x, int y);

  // utility routines used by state changes below
  virtual void StartState(int newstate);
  virtual void StopState();

  // Interaction mode entry points used internally.  
  virtual void StartAnimate();  
  virtual void StopAnimate();  
  virtual void StartRotate();
  virtual void EndRotate();
  virtual void StartZoom();
  virtual void EndZoom();
  virtual void StartPan();
  virtual void EndPan();
  virtual void StartSpin();
  virtual void EndSpin();
  virtual void StartDolly();
  virtual void EndDolly();
  virtual void StartUniformScale();
  virtual void EndUniformScale();
  virtual void StartTimer();
  virtual void EndTimer();

  // Data we need to maintain internally
  vtkRenderWindowInteractor *Interactor;
  //
  vtkCamera          *CurrentCamera;
  vtkLight           *CurrentLight;
  vtkRenderer        *CurrentRenderer;
  //
  float Center[2];
  float DeltaAzimuth;
  float DeltaElevation;
  int   CtrlKey;
  int   ShiftKey;
  int   LastPos[2];
  int   State;  
  int   AnimState;  
  float FocalDepth;  

  // for picking and highlighting props
  vtkOutlineSource   *Outline;
  vtkPolyDataMapper  *OutlineMapper;
  vtkActor           *OutlineActor;
  vtkRenderer        *PickedRenderer;
  vtkProp            *CurrentProp;
  int                PropPicked;          // boolean: prop picked?
  float              PickColor[3];        // support 2D picking
  vtkActor2D         *PickedActor2D;

  void (*LeftButtonPressMethod)(void *);
  void (*LeftButtonPressMethodArgDelete)(void *);
  void *LeftButtonPressMethodArg;
  void (*LeftButtonReleaseMethod)(void *);
  void (*LeftButtonReleaseMethodArgDelete)(void *);
  void *LeftButtonReleaseMethodArg;

  void (*MiddleButtonPressMethod)(void *);
  void (*MiddleButtonPressMethodArgDelete)(void *);
  void *MiddleButtonPressMethodArg;
  void (*MiddleButtonReleaseMethod)(void *);
  void (*MiddleButtonReleaseMethodArgDelete)(void *);
  void *MiddleButtonReleaseMethodArg;

  void (*RightButtonPressMethod)(void *);
  void (*RightButtonPressMethodArgDelete)(void *);
  void *RightButtonPressMethodArg;
  void (*RightButtonReleaseMethod)(void *);
  void (*RightButtonReleaseMethodArgDelete)(void *);
  void *RightButtonReleaseMethodArg;
};

#endif
