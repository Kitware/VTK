/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkRenderWindowInteractor - provide event driven interface to rendering window

// .SECTION Description
// vtkRenderWindowInteractor is a convenience object that provides event
// bindings to common graphics functions. For example, camera or actor
// zoom-in/zoom-out, pan, rotate, spin, dolly, scale, resetting in either
// trackball or joystick mode; picking of actors, points,
// or cells; switching in/out of stereo mode; property changes such as
// wireframe and surface; and a toggle to force the light to be placed at
// camera viewpoint (pointing in view direction).
//
//<PRE>
// Mouse bindings:
//    camera: Button 1 - rotate
//            Button 2 - pan
//            Button 3 - zoom
//            ctrl-Button 1 - spin
//    actor:  Button 1 - rotate
//            Button 2 - pan
//            Button 3 - uniform scale
//            ctrl-Button 1 - spin
//            ctrl-Button 2 - dolly.
//
// Keyboard bindings (upper or lower case):
//    j - joystick like mouse interactions
//    t - trackball like mouse interactions
//    o - object/ actor interaction
//    c - camera interaction
//    r - reset camera view
//    w - turn all actors wireframe
//    s - turn all actors surface
//    u - execute user defined function
//    p - pick actor under mouse pointer (if pickable)
//    3 - toggle in/out of 3D mode (if supported by renderer)
//    e - exit
//    q - exit
//</PRE>
//
// Camera mode and joystick mode are the default modes for compatibility.
// 
// When "j" is pressed, the interaction models after a joystick. The distance
// from the center of the renderer viewport determines how quickly to rotate,
// pan, zoom, spin, and dolly.  This is the default mode for compatiblity
// reasons.  This is also known as position sensitive motion.
//
// When "t" is pressed, the interaction models after a trackball. Each mouse
// movement is used to move the actor or camera. When the mouse stops, the
// camera or actor motion is also stopped. This is also known as motion
// sensitive motion.
//
// Rotate, pan, and zoom work the same way as before.  Spin has two different
// interfaces depending on whether the interactor is in trackball or joystick
// mode.  In trackball mode, by moving the mouse around the camera or actor
// center in a circular motion, the camera or actor is spun.  In joystick mode
// by moving the mouse in the y direction, the actor or camera is spun. Scale
// dolly, and zoom all work in the same manner, that motion of mouse in y
// direction generates the transformation.
//
// The event bindings for Camera mode and Actor mode are very similar, with
// the exception of zoom (Camera only), and scale and dolly (Actor only). The
// same user events elicit the same responses from the interactor.
//
// When the "p" key is pressed, an actor is selected using the user supplied
// picker if one exist, or the default picker if one does not.  The picked
// actor is NOT used for actor mode interactions.  To interact with an actor,
// click on the actor with the pointer in Actor mode, and an internal picker
// will select the appropriate actor.  Since the selections of the actors are
// for different purposes, and handled by two different pickers, the
// previously selected actor will be unselected when the interaction mode
// has been switched between Actor mode and Camera mode.
//
// Interactors for a particular platform may have additional, specific event
// bindings.  Please see the documentation for the subclasses.

// .SECTION See Also
// vtkXRenderWindowInteractor vtkWin32RenderWindowInteractor vtkPicker

#ifndef __vtkRenderWindowInteractor_h
#define __vtkRenderWindowInteractor_h

#include "vtkObject.h"
#include "vtkRenderWindow.h"
#include "vtkCamera.h"
#include "vtkLight.h"
//#include "vtkPicker.h"
#include "vtkPolyDataMapper.h"
#include "vtkOutlineSource.h"
#include "vtkMath.h"

// for interaction picker
#include "vtkCellPicker.h"

// interaction modes
#define VTKXI_JOY   0
#define VTKXI_TRACK  1
#define VTKXI_CAMERA 0
#define VTKXI_ACTOR  1
#define VTKXI_CONTROL_OFF 0
#define VTKXI_CONTROL_ON 1

// interactions
#define VTKXI_START  0
#define VTKXI_ROTATE 1
#define VTKXI_ZOOM   2
#define VTKXI_PAN    3
#define VTKXI_SPIN   4
#define VTKXI_DOLLY  5
#define VTKXI_USCALE 6


class VTK_EXPORT vtkRenderWindowInteractor : public vtkObject
{
public:
  vtkRenderWindowInteractor();
  ~vtkRenderWindowInteractor();
  static vtkRenderWindowInteractor *New();
  const char *GetClassName() {return "vtkRenderWindowInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Prepare for handling events. This must be called before the
  // interactor will work.
  virtual void Initialize() {this->Initialized=1; this->Enable();
                             this->RenderWindow->Render();}

  // Description:
  // Start the event loop. This is provided so that you do not have to
  // implement your own event loop. You still can use your own 
  // event loop if you want. Initialize should be called before Start.
  virtual void Start() {};

  // Description:
  // Enable/Disable interactions.  By default interactors are enabled when
  // initialized.  Initialize() must be called prior to enabling/disabling
  // interaction. These methods are used when a window/widget is being
  // shared by multiple renderers and interactors.  This allows a "modal"
  // display where one interactor is active when its data is to be displayed
  // and all other interactors associated with the widget are disabled
  // when their data is not displayed.
  virtual void Enable() { this->Enabled = 1; this->Modified();};
  virtual void Disable() { this->Enabled = 0; this->Modified();};
  vtkGetMacro(Enabled, int);
  
  // Description:
  // Set/Get the rendering window being controlled by this object.
  void SetRenderWindow(vtkRenderWindow *aren);
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);

  // Description:
  // Turn on/off the automatic repositioning of lights as the camera moves.
  vtkSetMacro(LightFollowCamera,int);
  vtkGetMacro(LightFollowCamera,int);
  vtkBooleanMacro(LightFollowCamera,int);

  // Description:
  // Set/Get the desired update rate. This is used by vtkLODActor's to tell 
  // them how quickly they need to render.  This update is in effect only
  // when the camera is being rotated, or zoomed.  When the interactor is
  // still, the StillUpdateRate is used instead. 
  vtkSetClampMacro(DesiredUpdateRate,float,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(DesiredUpdateRate,float);

  // Description:
  // Set/Get the desired update rate when movement has stopped.
  // See the SetDesiredUpdateRate method. 
  vtkSetClampMacro(StillUpdateRate,float,0.0001,VTK_LARGE_FLOAT);
  vtkGetMacro(StillUpdateRate,float);

  // Description:
  // See whether interactor has been initialized yet.
  vtkGetMacro(Initialized,int);

  // Description:
  // When an event occurs, we must determine which Renderer the event
  // occurred within, since one RenderWindow may contain multiple 
  // renderers. We also need to know what camera to operate on.
  // This is just the ActiveCamera of the poked renderer. 
  void FindPokedCamera(int,int);
  void FindPokedRenderer(int,int);

  // Description:
  // When pick action successfully selects actor, this method highlights the 
  // actor appropriately. Currently this is done by placing a bounding box
  // around the actor.
  virtual void HighlightActor(vtkActor *actor);

  // Description:
  // Specify a method to be executed prior to the pick operation.
  void SetStartPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetStartPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Specify a method to be executed after the pick operation.
  void SetEndPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetEndPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Specify a method to be executed prior to the pick operation.
  void SetStartInteractionPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetStartInteractionPickMethodArgDelete(void (*f)(void *));

  // Description:
  // Specify a method to be executed after the pick operation.
  void SetEndInteractionPickMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetEndInteractionPickMethodArgDelete(void (*f)(void *));

  
  // Description:
  // Set the object used to perform pick operations. You can use this to 
  // control what type of data is picked.
  void SetPicker(vtkPicker *picker);

  // Description:
  // Get the object used to perform pick operations.
  vtkGetObjectMacro(Picker,vtkPicker);

  //Description:
  // Get the object used to perform mouse interaction pick operation
  vtkGetObjectMacro(InteractionPicker, vtkCellPicker);

  // Description:
  // Create default picker. Used to create one when none is specified.
  virtual vtkPicker *CreateDefaultPicker();

  // Description:
  // Set the user method. This method is invoked on a "u" keypress.
  void SetUserMethod(void (*f)(void *), void *arg);
  
  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetUserMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a "e" or "q" keypress.
  void SetExitMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetExitMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the timer method. This method is invoked during rotate/zoom/pan
  void SetTimerMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetTimerMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the mouse event method, invoked on left mouse button press.
  void SetLeftButtonPressMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetLeftButtonPressMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the mouse event method, invoked on left mouse button release.
  void SetLeftButtonReleaseMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetLeftButtonReleaseMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the mouse event method, invoked on middle mouse button press.
  void SetMiddleButtonPressMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetMiddleButtonPressMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the mouse event method, invoked on middle mouse button release.
  void SetMiddleButtonReleaseMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetMiddleButtonReleaseMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the mouse event method, invoked on right mouse button press.
  void SetRightButtonPressMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetRightButtonPressMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the mouse event method, invoked on right mouse button release.
  void SetRightButtonReleaseMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetRightButtonReleaseMethodArgDelete(void (*f)(void *));

  // Description:
  // This method is invoked on a "c" keypress
  void SetCameraModeMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetCameraModeMethodArgDelete(void (*f)(void *));

  // Description:
  // This method is invoked on a "a" keypress
  void SetActorModeMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetActorModeMethodArgDelete(void (*f)(void *));

  // Description:
  // This method is invoked on a "t" keypress
  void SetTrackballModeMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetTrackballModeMethodArgDelete(void (*f)(void *));

  // Description:
  // This method is invoked on a "j" keypress
  void SetJoystickModeMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetJoystickModeMethodArgDelete(void (*f)(void *));

  // Description:
  // This method can be used by user callbacks to get the 
  // x, y, coordinates of the current event.
  vtkSetVector2Macro(EventPosition,int);
  vtkGetVectorMacro(EventPosition,int,2);

  // Description:
  // Primarily internal methods used to start and stop interactions
  // overridden in subclass to provide platform and hardware support
  virtual void StartRotate() {};
  virtual void EndRotate() {};
  virtual void StartZoom() {};
  virtual void EndZoom() {};
  virtual void StartPan() {};
  virtual void EndPan() {};
  virtual void StartSpin() {};
  virtual void EndSpin() {};
  virtual void StartDolly() {};
  virtual void EndDolly() {};
  virtual void StartUniformScale() {};
  virtual void EndUniformScale() {};

  // Description:
  // This Method detects loops of RenderWindow-Interactor,
  // so objects are freed properly.
  void UnRegister(vtkObject *o);
  
  // Description:
  // For legacy compatibiltiy. Do not use.
  void SetPicker(vtkPicker& picker) {this->SetPicker(&picker);};

  // Description:
  // External switching between actor and camera mode.
  virtual void SetActorModeToCamera();
  virtual void SetActorModeToActor();
  vtkGetMacro(ActorMode, int);
  
  // Description:
  // External switching between joystick and trackball mode.
  virtual void SetTrackballModeToTrackball();
  virtual void SetTrackballModeToJoystick();
  vtkGetMacro(TrackballMode, int);
  
  // Description:
  // This methods sets the Size ivar of the interactor without
  // actually changing the size of the window. Normally
  // application programmers would use UpdateSize if anything.
  // This is useful for letting someone else change the size of
  // the rendering window and just letting the interactor
  // know about the change.
  vtkSetVector2Macro(Size,int);
  vtkGetVector2Macro(Size,int);
  
protected:
  vtkRenderWindow *RenderWindow;
  vtkCamera   *CurrentCamera;
  vtkLight    *CurrentLight;
  vtkRenderer *CurrentRenderer;
  int   LightFollowCamera;
  float Center[2];
  float DeltaAzimuth;
  float DeltaElevation;
  int   Size[2];
  int   State;
  int   AnimationState;
  float FocalDepth;
  int   Initialized;
  int   Enabled;
  float DesiredUpdateRate;
  float StillUpdateRate;
  int   EventPosition[2];

  // for picking actors
  vtkPicker *Picker;
  vtkOutlineSource *Outline;
  vtkPolyDataMapper *OutlineMapper;
  vtkActor *OutlineActor;
  vtkRenderer *PickedRenderer;
  vtkActor *CurrentActor;
  
  // used to track picked objects in actor mode
  // reason for existence: user may use any kind of picker.  Interactor
  //    need the high precision of cell picker at all time.
  vtkCellPicker *InteractionPicker;
  int ActorPicked;                      // boolean: actor picked?
  vtkActor *InteractionActor;
  
  // new interactor modes
  int ActorMode;
  int TrackballMode;
  int ControlMode;

  // some constants
  int Preprocess;                       // boolean: was preprocessing done?
  float RadianToDegree;                 // constant: for conv from deg to rad
  float TrackballFactor;                // constant: for motion

  // data arrays for motion
  float NewPickPoint[4];
  float OldPickPoint[4];
  float MotionVector[3];                // vector used for interaction
  float OldX;
  float OldY;
  
  // this really belong in camera
  float ViewLook[3];
  float ViewPoint[3];
  float ViewFocus[3];
  float ViewUp[3];
  float ViewRight[3];

  // actor stuff
  float Origin[3];
  float Position[3];
  float ObjCenter[3];                   // center of bounding box
  float DispObjCenter[3];               // center of box in display coord
  float Radius;                         // radius of virtual sphere
  
  // user methods that can be used to override default behaviour
  void (*StartPickMethod)(void *);
  void (*StartPickMethodArgDelete)(void *);
  void *StartPickMethodArg;
  void (*EndPickMethod)(void *);
  void (*EndPickMethodArgDelete)(void *);
  void *EndPickMethodArg;
  void (*StartInteractionPickMethod)(void *);
  void (*StartInteractionPickMethodArgDelete)(void *);
  void *StartInteractionPickMethodArg;
  void (*EndInteractionPickMethod)(void *);
  void (*EndInteractionPickMethodArgDelete)(void *);
  void *EndInteractionPickMethodArg;
  void (*UserMethod)(void *);
  void (*UserMethodArgDelete)(void *);
  void *UserMethodArg;
  void (*ExitMethod)(void *);
  void (*ExitMethodArgDelete)(void *);
  void *ExitMethodArg;

  void (*CameraModeMethod)(void *);
  void (*CameraModeMethodArgDelete)(void *);
  void *CameraModeMethodArg;
  void (*ActorModeMethod)(void *);
  void (*ActorModeMethodArgDelete)(void *);
  void *ActorModeMethodArg;
  void (*JoystickModeMethod)(void *);
  void (*JoystickModeMethodArgDelete)(void *);
  void *JoystickModeMethodArg;
  void (*TrackballModeMethod)(void *);
  void (*TrackballModeMethodArgDelete)(void *);
  void *TrackballModeMethodArg;

  void (*TimerMethod)(void *);
  void (*TimerMethodArgDelete)(void *);
  void *TimerMethodArg;

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

  // convenience methods for converting between coordinate systems
  virtual void ComputeDisplayToWorld(float x, float y, float z,
                                     float *worldPt);
  virtual void ComputeWorldToDisplay(float x, float y, float z,
                                     float *displayPt);

  // perform actor mode scale and rotate transformations
  virtual void ActorTransform(vtkActor *actor,
                              float *boxCenter,
                              int NumRotation,
                              float **rotate,
                              float *scale);
  
  // methods for the different interactions in different modes
  virtual void JoystickRotateCamera(int x, int y);
  virtual void JoystickSpinCamera(int x, int y);
  virtual void JoystickPanCamera(int x, int y);
  virtual void JoystickDollyCamera(int x, int y);
  
  virtual void TrackballRotateCamera(int x, int y);
  virtual void TrackballSpinCamera(int x, int y);
  virtual void TrackballPanCamera(int x, int y);
  virtual void TrackballDollyCamera(int x, int y);
  
  virtual void JoystickRotateActor(int x, int y);
  virtual void JoystickSpinActor(int x, int y);
  virtual void JoystickPanActor(int x, int y);
  virtual void JoystickDollyActor(int x, int y);
  virtual void JoystickScaleActor(int x, int y);
  
  virtual void TrackballRotateActor(int x, int y);
  virtual void TrackballSpinActor(int x, int y);
  virtual void TrackballPanActor(int x, int y);
  virtual void TrackballDollyActor(int x, int y);
  virtual void TrackballScaleActor(int x, int y);
};

#endif
