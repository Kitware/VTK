/*=========================================================================

  Program:   Visualization Library
  Module:    Camera.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlCamera - a virtual camera for 3D rendering
// .SECTION Description
// vlCamera is a virtual camera for 3D rendering. It provides methods
// to position and orient the view point and focal point. Convenience 
// methods for moving about the focal point are also provided. More 
// complex methods allow the manipulation of the computer graphics
// graphics model including view up vector, clipping planes, and 
// camera perspective.

#ifndef __vlCamera_hh
#define __vlCamera_hh

#include "Object.hh"
#include "Trans.hh"

class vlRenderer;

class vlCamera : public vlObject
{
 public:
  vlCamera();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlCamera";};

  void SetPosition(float x, float y, float z);
  void SetPosition(float a[3]);
  vlGetVectorMacro(Position,float,3);

  void SetFocalPoint(float x, float y, float z);
  void SetFocalPoint(float a[3]);
  vlGetVectorMacro(FocalPoint,float,3);

  void SetViewUp(float vx, float vy, float vz);
  void SetViewUp(float a[3]);
  vlGetVectorMacro(ViewUp,float,3);

  void SetClippingRange(float front, float back);
  void SetClippingRange(float a[2]);
  vlGetVectorMacro(ClippingRange,float,2);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vlCamera
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Render(vlRenderer *ren) = 0;

  // Description:
  // Set the camera view angle (i.e., the width of view in degrees). Larger
  // values yield greater perspective distortion.
  vlSetClampMacro(ViewAngle,float,1.0,179.0);
  // Description:
  // Get the camera view angle (i.e., the width of view in degrees).
  vlGetMacro(ViewAngle,float);

  // Description:
  // Set the seperation between eyes (in degrees). Used to generate stereo
  // images.
  vlSetMacro(EyeAngle,float);
  // Description:
  // Get the seperation between eyes (in degrees). Used to generate stereo
  // images.
  vlGetMacro(EyeAngle,float);

  void SetThickness(float);
  vlGetMacro(Thickness,float);

  void SetDistance(float);
  vlGetMacro(Distance,float);

  // Description:
  // Turn the camera on/off.
  vlSetMacro(Switch,int);
  // Description:
  // Get the value of the Switch instance variable. This indicates if the 
  // camera is on or off.
  vlGetMacro(Switch,int);
  // Description:
  // Turn the camera on/off.
  vlBooleanMacro(Switch,int);

  float GetTwist();
  void SetViewPlaneNormal(float a[3]);
  void SetViewPlaneNormal(float x, float y, float z);
  void CalcViewPlaneNormal();
  void CalcDistance();
  void CalcPerspectiveTransform();
  vlMatrix4x4 &GetPerspectiveTransform();
  vlGetVectorMacro(ViewPlaneNormal,float,3);

  void SetRoll(float);
  void Roll(float);
  float GetRoll();

  void Zoom(float);
  void Azimuth(float);
  void Yaw(float);
  void Elevation(float);
  void Pitch(float);
  void OrthogonalizeViewUp();

  float *GetOrientation();

 protected:
  float FocalPoint[3];
  float Position[3];
  float ViewUp[3];
  float ViewAngle;
  float ClippingRange[2];
  float EyeAngle;
  int   LeftEye;
  int   Switch;
  float Thickness;
  float Distance;
  float ViewPlaneNormal[3];
  vlTransform Transform;
  vlTransform PerspectiveTransform;
  float Orientation[3];

};

#endif
