/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInteractorStyleFlight.h
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
// Stationary 'look' can be achieved by holding both mouse buttons down
// and steering with the mouse.
// Stationary 'look' can also be achieved by holding 'Z' (or 'A') and
// steering with mouse in forward (or reverse) motion mode.
// By default, one "step" of motion corresponds to 1/250th of the diagonal
// of bounding box of visible actors, '+' and '-' keys allow user to
// increase or decrease step size.

#ifndef __vtkInteractorStyleFlight_h
#define __vtkInteractorStyleFlight_h

#include "vtkInteractorStyle.h"

class VTK_RENDERING_EXPORT vtkInteractorStyleFlight : public vtkInteractorStyle
{
public:
  static vtkInteractorStyleFlight *New();
  vtkTypeRevisionMacro(vtkInteractorStyleFlight,vtkInteractorStyle);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Move the Eye/Camera to a specific location (no intermediate
  // steps are taken
  void JumpTo(double campos[3], double focpos[3]);

  // Description:
  // rotate the camera round z axis by 360 degrees for viewing scene
  // this routine starts a timer and disables key/mouse events preventing
  // user interaction until finished (not fully implemented yet)
  // the number of steps can be supplied.
  void PerformAzimuthalScan(int numsteps);

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
  // Fix the "up" vector: also use FixedUpVector
  vtkSetMacro(FixUpVector,int);
  vtkGetMacro(FixUpVector,int);
  vtkBooleanMacro(FixUpVector,int);

  // Specify fixed "up"
  vtkGetVectorMacro(FixedUpVector,double,3);
  vtkSetVectorMacro(FixedUpVector,double,3);

protected:
  vtkInteractorStyleFlight();
  ~vtkInteractorStyleFlight();

  // Description:
  // Concrete implementation of Mouse event bindings for flight
  virtual void OnRightButtonDown (int ctrl, int shift, int x, int y);
  virtual void OnRightButtonUp   (int ctrl, int shift, int x, int y);
  virtual void OnMiddleButtonDown(int ctrl, int shift, int x, int y);
  virtual void OnMiddleButtonUp  (int ctrl, int shift, int x, int y);
  virtual void OnLeftButtonDown  (int ctrl, int shift, int x, int y);
  virtual void OnLeftButtonUp    (int ctrl, int shift, int x, int y);
  virtual void OnMouseMove       (int ctrl, int shift, int x, int y);

  // Description:
  // Concrete implementation of Keyboard event bindings for flight
  virtual void OnChar   (int ctrl, int shift, char keycode, int repeatcount);
  virtual void OnKeyDown(int ctrl, int shift, char keycode, int repeatcount);
  virtual void OnKeyUp  (int ctrl, int shift, char keycode, int repeatcount);

  // Description:
  // Mouse and key events set correct motion states, OnTimer performs the motion
  virtual void OnTimer(void);

  // Description:
  // Routines used internally for computing motion and steering
  void DoTimerStart(void);
  void DoTimerStop(void);
  void UpdateMouseSteering(int x, int y);
  void FlyByMouse(void);
  void FlyByKey(void);
  void ComputeLRVector(double vector[3]);
  void MotionAlongVector(double vector[3], double amount);
  void SetupMotionVars(void);
  void AzimuthScan(void);
  //
  //
  unsigned char KeysDown;
  int           Flying;
  int           Reversing;
  int           TimerRunning;
  int           AzimuthScanning;
  int           DisableMotion;
  int           FixUpVector;
  int           X2;
  int           Y2;
  double        DiagonalLength;
  double        MotionStepSize;
  double        MotionUserScale;
  double        MotionAccelerationFactor;
  double        AngleStepSize;
  double        AngleAccelerationFactor;
  double        YawAngle;
  double        PitchAngle;
  double        FixedUpVector[3];
  double        AzimuthStepSize;

  int LastPos[2];

private:
  vtkInteractorStyleFlight(const vtkInteractorStyleFlight&);  // Not implemented.
  void operator=(const vtkInteractorStyleFlight&);  // Not implemented.
};

#endif
