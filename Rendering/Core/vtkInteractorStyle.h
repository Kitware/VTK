/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
// - Keypress c / Keypress a: toggle between camera and actor modes. In
// camera mode, mouse events affect the camera position and focal point. In
// actor mode, mouse events affect the actor that is under the mouse pointer.
// - Button 1: rotate the camera around its focal point (if camera mode) or
// rotate the actor around its origin (if actor mode). The rotation is in the
// direction defined from the center of the renderer's viewport towards
// the mouse position. In joystick mode, the magnitude of the rotation is
// determined by the distance the mouse is from the center of the render
// window.
// - Button 2: pan the camera (if camera mode) or translate the actor (if
// actor mode). In joystick mode, the direction of pan or translation is
// from the center of the viewport towards the mouse position. In trackball
// mode, the direction of motion is the direction the mouse moves. (Note:
// with 2-button mice, pan is defined as \<Shift\>-Button 1.)
// - Button 3: zoom the camera (if camera mode) or scale the actor (if
// actor mode). Zoom in/increase scale if the mouse position is in the top
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
// Typing u calls UserCallBack() on the vtkRenderWindowInteractor, which
// invokes a vtkCommand::UserEvent. In other words, to define a user-defined
// callback, just add an observer to the vtkCommand::UserEvent on the
// vtkRenderWindowInteractor object. 
// - Keypress w: modify the representation of all actors so that they are
// wireframe.
//
// vtkInteractorStyle can be subclassed to provide new interaction styles and
// a facility to override any of the default mouse/key operations which
// currently handle trackball or joystick styles is provided. Note that this
// class will fire a variety of events that can be watched using an observer,
// such as LeftButtonPressEvent, LeftButtonReleaseEvent,
// MiddleButtonPressEvent, MiddleButtonReleaseEvent, RightButtonPressEvent,
// RightButtonReleaseEvent, EnterEvent, LeaveEvent, KeyPressEvent,
// KeyReleaseEvent, CharEvent, ExposeEvent, ConfigureEvent, TimerEvent,
// MouseMoveEvent,


//
// .SECTION See Also
// vtkInteractorStyleTrackball

#ifndef __vtkInteractorStyle_h
#define __vtkInteractorStyle_h

#include "vtkInteractorObserver.h"

// Motion flags

#define VTKIS_START        0
#define VTKIS_NONE         0

#define VTKIS_ROTATE       1
#define VTKIS_PAN          2
#define VTKIS_SPIN         3
#define VTKIS_DOLLY        4
#define VTKIS_ZOOM         5
#define VTKIS_USCALE       6
#define VTKIS_TIMER        7
#define VTKIS_FORWARDFLY   8
#define VTKIS_REVERSEFLY   9

#define VTKIS_ANIM_OFF 0
#define VTKIS_ANIM_ON  1

class vtkActor2D;
class vtkActor;
class vtkCallbackCommand;
class vtkEventForwarderCommand;
class vtkOutlineSource;
class vtkPolyDataMapper;
class vtkProp3D;
class vtkProp;
class vtkTDxInteractorStyle;

class VTK_RENDERING_EXPORT vtkInteractorStyle : public vtkInteractorObserver
{
public:
  // Description:
  // This class must be supplied with a vtkRenderWindowInteractor wrapper or
  // parent. This class should not normally be instantiated by application
  // programmers.
  static vtkInteractorStyle *New();

  vtkTypeMacro(vtkInteractorStyle,vtkInteractorObserver);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the Interactor wrapper being controlled by this object.
  // (Satisfy superclass API.)
  virtual void SetInteractor(vtkRenderWindowInteractor *interactor);

  // Description:
  // Turn on/off this interactor. Interactor styles operate a little
  // bit differently than other types of interactor observers. When
  // the SetInteractor() method is invoked, the automatically enable
  // themselves. This is a legacy requirement, and convenient for the
  // user.
  virtual void SetEnabled(int);

  // Description:
  // If AutoAdjustCameraClippingRange is on, then before each render the
  // camera clipping range will be adjusted to "fit" the whole scene. Clipping
  // will still occur if objects in the scene are behind the camera or
  // come very close. If AutoAdjustCameraClippingRange is off, no adjustment
  // will be made per render, but the camera clipping range will still
  // be reset when the camera is reset.
  vtkSetClampMacro(AutoAdjustCameraClippingRange, int, 0, 1 );
  vtkGetMacro(AutoAdjustCameraClippingRange, int );
  vtkBooleanMacro(AutoAdjustCameraClippingRange, int );
  
  // Description:
  // When an event occurs, we must determine which Renderer the event
  // occurred within, since one RenderWindow may contain multiple
  // renderers. 
  void FindPokedRenderer(int,int);

  // Description:
  // Some useful information for interaction
  vtkGetMacro(State,int);

  // Description:
  // Set/Get timer hint
  vtkGetMacro(UseTimers,int);
  vtkSetMacro(UseTimers,int);
  vtkBooleanMacro(UseTimers,int);

  // Description:
  // If using timers, specify the default timer interval (in
  // milliseconds). Care must be taken when adjusting the timer interval from
  // the default value of 10 milliseconds--it may adversely affect the
  // interactors.
  vtkSetClampMacro(TimerDuration,unsigned long,1,100000);
  vtkGetMacro(TimerDuration,unsigned long);

  // Description:
  // Does ProcessEvents handle observers on this class or not
  vtkSetMacro(HandleObservers,int);
  vtkGetMacro(HandleObservers,int);
  vtkBooleanMacro(HandleObservers,int);

  // Description:                                                  
  // Generic event bindings can be overridden in subclasses
  virtual void OnMouseMove() {};
  virtual void OnLeftButtonDown() {};
  virtual void OnLeftButtonUp() {};
  virtual void OnMiddleButtonDown() {};
  virtual void OnMiddleButtonUp() {};
  virtual void OnRightButtonDown() {};
  virtual void OnRightButtonUp() {};
  virtual void OnMouseWheelForward() {};
  virtual void OnMouseWheelBackward() {};

  // Description:
  // OnChar is triggered when an ASCII key is pressed. Some basic key presses
  // are handled here ('q' for Quit, 'p' for Pick, etc)
  virtual void OnChar();

  // OnKeyDown is triggered by pressing any key (identical to OnKeyPress()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyDown() {};

  // OnKeyUp is triggered by releaseing any key (identical to OnKeyRelease()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyUp() {};

  // OnKeyPress is triggered by pressing any key (identical to OnKeyDown()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyPress() {};

  // OnKeyRelease is triggered by pressing any key (identical to OnKeyUp()).
  // An empty implementation is provided. The behavior of this function should
  // be specified in the subclass.
  virtual void OnKeyRelease() {};

  // Description:
  // These are more esoteric events, but are useful in some cases.
  virtual void OnExpose() {};
  virtual void OnConfigure() {};
  virtual void OnEnter() {};
  virtual void OnLeave() {};

  // Description:
  // OnTimer calls Rotate, Rotate etc which should be overridden by
  // style subclasses.
  virtual void OnTimer();

  // Description:
  // These methods for the different interactions in different modes
  // are overridden in subclasses to perform the correct motion. Since
  // they might be called from OnTimer, they do not have mouse coord parameters
  // (use interactor's GetEventPosition and GetLastEventPosition)
  virtual void Rotate() {};
  virtual void Spin() {};
  virtual void Pan() {};
  virtual void Dolly() {};
  virtual void Zoom() {};
  virtual void UniformScale() {};

  // Description:
  // utility routines used by state changes
  virtual void StartState(int newstate);
  virtual void StopState();

  // Description:
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
  vtkSetVector3Macro(PickColor,double);
  vtkGetVectorMacro(PickColor, double, 3);

  // Description:
  // Set/Get the mouse wheel motion factor. Default to 1.0. Set it to a 
  // different value to emphasize or de-emphasize the action triggered by
  // mouse wheel motion.
  vtkSetMacro(MouseWheelMotionFactor, double);
  vtkGetMacro(MouseWheelMotionFactor, double);

  // Description:
  // 3Dconnexion device interactor style. Initial value is a pointer to an
  // object of class vtkTdxInteractorStyleCamera.
  vtkGetObjectMacro(TDxStyle,vtkTDxInteractorStyle);
  virtual void SetTDxStyle(vtkTDxInteractorStyle *tdxStyle);
  
  // Description:
  // Called by the callback to process 3DConnexion device events.
  void DelegateTDxEvent(unsigned long event,
                        void *calldata);
  
protected:
  vtkInteractorStyle();
  ~vtkInteractorStyle();
  
  // Description:
  // Main process event method
  static void ProcessEvents(vtkObject* object, 
                            unsigned long event,
                            void* clientdata, 
                            void* calldata);
  
  // Keep track of current state
  int State;  
  int AnimState;  

  // Should observers be handled here, should we fire timers
  int HandleObservers; 
  int UseTimers;       
  int TimerId; //keep track of the timers that are created/destroyed

  int AutoAdjustCameraClippingRange;

  // For picking and highlighting props
  vtkOutlineSource   *Outline;
  vtkPolyDataMapper  *OutlineMapper;
  vtkActor           *OutlineActor;
  vtkRenderer        *PickedRenderer;
  vtkProp            *CurrentProp;
  vtkActor2D         *PickedActor2D;
  int                PropPicked;      // bool: prop picked?
  double             PickColor[3];    // support 2D picking
  double             MouseWheelMotionFactor;

  // Control the timer duration
  unsigned long  TimerDuration; //in milliseconds
  
  // Forward events to the RenderWindowInteractor
  vtkEventForwarderCommand * EventForwarder;

  vtkTDxInteractorStyle *TDxStyle;
  
private:
  vtkInteractorStyle(const vtkInteractorStyle&);  // Not implemented.
  void operator=(const vtkInteractorStyle&);  // Not implemented.
};

#endif
