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
// Specific devices have different camera and actor bindings. The bindings
// are on both mouse events as well as keyboard presses. See
// vtkXRenderWindowInteractor and vtkWin32RenderWindowInteractor for
// specific information.

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
  virtual void Initialize() {this->Initialized=1;this->RenderWindow->Render();}

  // Description:
  // Start the event loop. This is provided so that you do not have to
  // implement your own event loop. You still can use your own 
  // event loop if you want. Initialize should be called before Start.
  virtual void Start() {};

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
  // Set the object used to perform pick operations. You can use this to 
  // control what type of data is picked.
  void SetPicker(vtkPicker *picker);

  // Description:
  // Get the object used to perform pick operations.
  vtkGetObjectMacro(Picker,vtkPicker);

  // Description:
  // Create default picker. Used to create one when none is specified.
  virtual vtkPicker *CreateDefaultPicker();

  // Description:
  // Set the user method. This method is invoked on a <u> keypress.
  void SetUserMethod(void (*f)(void *), void *arg);
  
  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetUserMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a <e> keypress.
  void SetExitMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetExitMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked during rotate/zoom/pan
  void SetTimerMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetTimerMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a <e> keypress.
  void SetLeftButtonPressMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetLeftButtonPressMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a <e> keyrelease.
  void SetLeftButtonReleaseMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetLeftButtonReleaseMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a <e> keypress.
  void SetMiddleButtonPressMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetMiddleButtonPressMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a <e> keyrelease.
  void SetMiddleButtonReleaseMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetMiddleButtonReleaseMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a <e> keypress.
  void SetRightButtonPressMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetRightButtonPressMethodArgDelete(void (*f)(void *));

  // Description:
  // Set the exit method. This method is invoked on a <e> keyrelease.
  void SetRightButtonReleaseMethod(void (*f)(void *), void *arg);

  // Description:
  // Called when a void* argument is being discarded.  Lets the user free it.
  void SetRightButtonReleaseMethodArgDelete(void (*f)(void *));

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
  // This Method detects loops of RenderWindow<->Interactor,
  // so objects are freed properly.
  void UnRegister(vtkObject *o);
  
  // Description:
  // For legacy compatibiltiy. Do not use.
  void SetPicker(vtkPicker& picker) {this->SetPicker(&picker);};

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
  void (*UserMethod)(void *);
  void (*UserMethodArgDelete)(void *);
  void *UserMethodArg;
  void (*ExitMethod)(void *);
  void (*ExitMethodArgDelete)(void *);
  void *ExitMethodArg;

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
