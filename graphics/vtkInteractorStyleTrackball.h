/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleTrackball.h
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

// .NAME vtkInteractorStyleTrackball - provides Trackball/Joystick motion routines
//
// .SECTION Description

#ifndef __vtkInteractorStyleTrackball_h
#define __vtkInteractorStyleTrackball_h

#include "vtkInteractorStyle.h"
#define VTKIS_JOY   0
#define VTKIS_TRACK  1
#define VTKIS_CAMERA 0
#define VTKIS_ACTOR  1
#define VTKIS_CONTROL_OFF 0
#define VTKIS_CONTROL_ON 1

class VTK_EXPORT vtkInteractorStyleTrackball : public vtkInteractorStyle 
{
public:
  vtkInteractorStyleTrackball();
  ~vtkInteractorStyleTrackball();
  static vtkInteractorStyleTrackball *New() {return new vtkInteractorStyleTrackball;}
  const char *GetClassName() {return "vtkInteractorStyleTrackball";};
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

  void ActorTransform(vtkActor *actor, double *boxCenter,
                      int numRotation, double **rotate,
                      double *scale);
  void ActorTransform(vtkActor *actor,float *boxCenter,
                      int NumRotation,double **rotate,
                      double *scale);
  void FindPickedActor(int X, int Y);
};

#endif
