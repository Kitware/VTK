/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleFlight.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkInteractorStyleFlight - provides flight motion routines
//
// .SECTION Description
// Left  mouse button press produces forward motion.
// Right mouse button press produces reverse motion.
// Moving mouse during motion steers user in desired direction.
// Keyboard controls are:
// Left/Right/Up/Down Arrows for steering direction
// 'A' forward, 'Z' reverse motion
// Ctrl Key causes sidestep instead of steering in mouse and key modes
// Shift key is accelerator in mouse and key modes
// Ctrl and Shift together causes Roll in mouse and key modes
//
// By default, one "step" of motion corresponds to 1/250th of the diagonal
// of bounding box of visible actors, '+' and '-' keys allow user to
// increase or decrease step size.

#ifndef __vtkInteractorStyleFlight_h
#define __vtkInteractorStyleFlight_h

#include "vtkInteractionStyleModule.h" // For export macro
#include "vtkInteractorStyle.h"
class vtkCamera;
class vtkPerspectiveTransform;
//BTX
class CPIDControl;
//ETX

class VTKINTERACTIONSTYLE_EXPORT vtkInteractorStyleFlight : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleFlight *New();
  vtkTypeMacro(vtkInteractorStyleFlight,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move the Eye/Camera to a specific location (no intermediate
  // steps are taken
  void JumpTo(double campos[3], double focpos[3]);

  // Description:
  // Set the basic unit step size : by default 1/250 of bounding diagonal
  vtkSetMacro(MotionStepSize,double);
  vtkGetMacro(MotionStepSize,double);

  // Description:
  // Set acceleration factor when shift key is applied : default 10
  vtkSetMacro(MotionAccelerationFactor,double);
  vtkGetMacro(MotionAccelerationFactor,double);

  // Description:
  // Set the basic angular unit for turning : default 1 degree
  vtkSetMacro(AngleStepSize,double);
  vtkGetMacro(AngleStepSize,double);

  // Description:
  // Set angular acceleration when shift key is applied : default 5
  vtkSetMacro(AngleAccelerationFactor,double);
  vtkGetMacro(AngleAccelerationFactor,double);

  // Description:
  // Disable motion (temporarily - for viewing etc)
  vtkSetMacro(DisableMotion,int);
  vtkGetMacro(DisableMotion,int);
  vtkBooleanMacro(DisableMotion,int);

  // Description:
  // When flying, apply a restorative force to the "Up" vector.
  // This is activated when the current 'up' is close to the actual 'up'
  // (as defined in DefaultUpVector). This prevents excessive twisting forces
  // when viewing from arbitrary angles, but keep the horizon level when
  // the user is flying over terrain.
  vtkSetMacro(RestoreUpVector,int);
  vtkGetMacro(RestoreUpVector,int);
  vtkBooleanMacro(RestoreUpVector,int);

  // Specify "up" (by default {0,0,1} but can be changed)
  vtkGetVectorMacro(DefaultUpVector,double,3);
  vtkSetVectorMacro(DefaultUpVector,double,3);

  // Description:
  // Concrete implementation of Mouse event bindings for flight
  virtual void OnMouseMove();
  virtual void OnLeftButtonDown();
  virtual void OnLeftButtonUp();
  virtual void OnMiddleButtonDown();
  virtual void OnMiddleButtonUp();
  virtual void OnRightButtonDown();
  virtual void OnRightButtonUp();

  // Description:
  // Concrete implementation of Keyboard event bindings for flight
  virtual void OnChar();
  virtual void OnKeyDown();
  virtual void OnKeyUp();
  virtual void OnTimer();
  //
  virtual void ForwardFly();
  virtual void ReverseFly();
  //
  virtual void StartForwardFly();
  virtual void EndForwardFly();
  virtual void StartReverseFly();
  virtual void EndReverseFly();

protected:
   vtkInteractorStyleFlight();
  ~vtkInteractorStyleFlight();

  // Description:
  // Routines used internally for computing motion and steering
  void UpdateSteering(vtkCamera *cam);
  void UpdateMouseSteering(vtkCamera *cam);
  void FlyByMouse(vtkCamera* cam);
  void FlyByKey(vtkCamera* cam);
  void GetLRVector(double vector[3], vtkCamera* cam);
  void MotionAlongVector(double vector[3], double amount, vtkCamera* cam);
  void SetupMotionVars(vtkCamera *cam);
  void FinishCamera(vtkCamera* cam);
  //
  //
  unsigned char KeysDown;
  int           DisableMotion;
  int           RestoreUpVector;
  double        DiagonalLength;
  double        MotionStepSize;
  double        MotionUserScale;
  double        MotionAccelerationFactor;
  double        AngleStepSize;
  double        AngleAccelerationFactor;
  double        DefaultUpVector[3];
  double        AzimuthStepSize;
  double        IdealFocalPoint[3];
  vtkPerspectiveTransform *Transform;
  double        DeltaYaw;
  double        lYaw;
  double        DeltaPitch;
  double        lPitch;
//BTX
  CPIDControl  *PID_Yaw;
  CPIDControl  *PID_Pitch;
//ETX
private:
  vtkInteractorStyleFlight(const vtkInteractorStyleFlight&);  // Not implemented.
  void operator=(const vtkInteractorStyleFlight&);  // Not implemented.
};

#endif
