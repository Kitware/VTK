/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackball.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkInteractorStyleTrackball - provides trackball motion control

// .SECTION Description
// vtkInteractorStyleTrackball is an implementation of vtkInteractorStyle
// that defines the trackball style. The trackball style can be thought of
// as a "grab and move" approach. That is, on mouse down a point on the
// object is grabbed, and then moving the mouse cause motion in proportion
// to the amount of motion. Note that the events that are bound by this
// class are the same as vtkInteractorStyle (which implements joystick mode),
// just the behavior is modified consistent with the trackball style of 
// interaction.

// .SECTION See Also
// vtkInteractorStyle

#ifndef __vtkInteractorStyleTrackball_h
#define __vtkInteractorStyleTrackball_h

#include "vtkInteractorStyle.h"
#include "vtkAbstractPropPicker.h"

#define VTKIS_JOY   0
#define VTKIS_TRACK  1
#define VTKIS_CAMERA 0
#define VTKIS_ACTOR  1
#define VTKIS_CONTROL_OFF 0
#define VTKIS_CONTROL_ON 1

class VTK_RENDERING_EXPORT vtkInteractorStyleTrackball : public vtkInteractorStyle 
{
public:
  static vtkInteractorStyleTrackball *New();
  vtkTypeRevisionMacro(vtkInteractorStyleTrackball,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Concrete implementation of event bindings
  virtual   void OnRightButtonDown(int ctrl, int shift, int X, int Y);
  virtual   void OnRightButtonUp  (int ctrl, int shift, int X, int Y);
  virtual   void OnMiddleButtonDown(int ctrl, int shift, int X, int Y);
  virtual   void OnMiddleButtonUp  (int ctrl, int shift, int X, int Y);
  virtual   void OnLeftButtonDown(int ctrl, int shift, int X, int Y);
  virtual   void OnLeftButtonUp  (int ctrl, int shift, int X, int Y);
  virtual   void OnChar(int ctrl, int shift, char keycode, int repeatcount);
  
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
  // OnTimer calls RotateCamera, RotateActor etc which should be overridden by
  // style subclasses.
  virtual void OnTimer(void);

protected:
  vtkInteractorStyleTrackball();
  ~vtkInteractorStyleTrackball();

  // used to track picked objects in actor mode
  // reason for existence: user may use any kind of picker.  Interactor
  //    need the high precision of cell picker at all time.
  vtkAbstractPropPicker *InteractionPicker;
  int PropPicked;                      // boolean: prop picked?
  vtkProp3D *InteractionProp;
  
  // new interactor modes
  int ActorMode;
  int TrackballMode;
  int ControlMode;
  float MotionFactor;                // constant: for motion
  int Preprocess;                       // boolean: was preprocessing done?
  float RadianToDegree;                 // constant: for conv from deg to rad

  // data arrays for motion
  float NewPickPoint[4];
  float OldPickPoint[4];
  float MotionVector[3];                // vector used for interaction
  float OldX;
  float OldY;
  
  // this really belong in camera
  double ViewLook[3];
  double ViewPoint[3];
  double ViewFocus[3];
  double ViewUp[3];
  double ViewRight[3];

  // actor stuff
  float Origin[3];
  float Position[3];
  float ObjCenter[3];                   // center of bounding box
  float DispObjCenter[3];               // center of box in display coord
  float Radius;                         // radius of virtual sphere

  // methods for the different interactions in different modes
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

  void Prop3DTransform(vtkProp3D *prop3D, double *boxCenter,
                      int numRotation, double **rotate,
                      double *scale);
  void Prop3DTransform(vtkProp3D *prop3D,float *boxCenter,
                      int NumRotation,double **rotate,
                      double *scale);
  void FindPickedActor(int X, int Y);
private:
  vtkInteractorStyleTrackball(const vtkInteractorStyleTrackball&);  // Not implemented.
  void operator=(const vtkInteractorStyleTrackball&);  // Not implemented.
};

#endif
