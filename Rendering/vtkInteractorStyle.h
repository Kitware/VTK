/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle.h
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
// .NAME vtkInteractorStyle - provide event-driven interface to the rendering window (defines trackball mode)
// .SECTION Description 
// vtkInteractorStyle is a base class implementing the majority of motion
// control routines and defines an event driven interface to support
// vtkRenderWindowInteractor. vtkRenderWindowInteractor implements 
// platform dependent key/mouse routing and timer control, which forwards
// events in a neutral form to vtkInteractorStyle.
//
// vtkInteractorStyle implements the "joystick" style of interaction. That
// is, holding down the mouse keys generates a stream of events that cause
// continuous actions (e.g., rotate, translate, pan, zoom). (The class
// vtkInteractorStyleTrackball implements a grab and move style.) The event
// bindings for this class include the following:
// - Keypress j / Keypress t: toggle between joystick (position sensitive) and
// trackball (motion sensitive) styles. In joystick style, motion occurs
// continuously as long as a mouse button is pressed. In trackball style,
// motion occurs when the mouse button is pressed and the mouse pointer
// moves.
// - Keypress c / Keypress o: toggle between camera and object (actor)
// modes. In camera mode, mouse events affect the camera position and focal
// point. In object mode, mouse events affect the actor that is under the
// mouse pointer.
// - Button 1: rotate the camera around its focal point (if camera mode) or
// rotate the actor around its origin (if actor mode). The rotation is in the
// direction defined from the center of the renderer's viewport towards
// the mouse position. In joystick mode, the magnitude of the rotation is
// determined by the distance the mouse is from the center of the render
// window.
// - Button 2: pan the camera (if camera mode) or translate the actor (if
// object mode). In joystick mode, the direction of pan or translation is
// from the center of the viewport towards the mouse position. In trackball
// mode, the direction of motion is the direction the mouse moves. (Note:
// with 2-button mice, pan is defined as <Shift>-Button 1.)
// - Button 3: zoom the camera (if camera mode) or scale the actor (if
// object mode). Zoom in/increase scale if the mouse position is in the top
// half of the viewport; zoom out/decrease scale if the mouse position is in
// the bottom half. In joystick mode, the amount of zoom is controlled by the
// distance of the mouse pointer from the horizontal centerline of the
// window.
// - Keypress 3: toggle the render window into and out of stereo mode. By
// default, red-blue stereo pairs are created. Some systems support Crystal
// Eyes LCD stereo glasses; you have to invoke SetStereoTypeToCrystalEyes()
// on the rendering window.
// - Keypress e: exit the application.
// - Keypress f: fly to the picked point
// - Keypress p: perform a pick operation. The render window interactor has
// an internal instance of vtkCellPicker that it uses to pick. 
// - Keypress r: reset the camera view along the current view
// direction. Centers the actors and moves the camera so that all actors are
// visible.
// - Keypress s: modify the representation of all actors so that they are
// surfaces.  
// - Keypress u: invoke the user-defined function. Typically,
// this keypress will bring up an interactor that you can type commands in.
// - Keypress w: modify the representation of all actors so that they are
// wireframe.
//
// vtkInteractorStyle can be subclassed to provide new interaction styles and
// a facility to override any of the default mouse/key operations which
// currently handle trackball or joystick styles is provided.
//
// .SECTION See Also
// vtkInteractorStyleTrackball

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
  virtual void SetInteractor(vtkRenderWindowInteractor *interactor);
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
  // OnChar implements keyboard functions, but subclasses can override this 
  // behavior
  virtual void OnChar   (int ctrl, int shift, char keycode, int repeatcount);
  virtual void OnKeyDown(int ctrl, int shift, char keycode, int repeatcount);
  virtual void OnKeyUp  (int ctrl, int shift, char keycode, int repeatcount);
  virtual void OnKeyPress(int ctrl, int shift, char keycode, char *keysym, 
			  int repeatcount);
  virtual void OnKeyRelease(int ctrl, int shift, char keycode, char *keysym,
			    int repeatcount);

  // Description:
  // These are more esoteric events, but are useful in some cases.
  virtual void OnConfigure(int width, int height);
  virtual void OnEnter(int ctrl, int shift, int x, int y);
  virtual void OnLeave(int ctrl, int shift, int x, int y);

  // Description:
  // OnTimer calls RotateCamera, RotateActor etc which should be overridden by
  // style subclasses.
  virtual void OnTimer();

  // Description:
  // Callbacks so that the application can override the default behaviour.
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
  vtkInteractorStyle(const vtkInteractorStyle&);
  void operator=(const vtkInteractorStyle&);

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

  unsigned long LeftButtonPressTag;
  unsigned long LeftButtonReleaseTag;
  unsigned long MiddleButtonPressTag;
  unsigned long MiddleButtonReleaseTag;
  unsigned long RightButtonPressTag;
  unsigned long RightButtonReleaseTag;
};

#endif
