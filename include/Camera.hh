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
#ifndef __vlCamera_hh
#define __vlCamera_hh

#include "Object.hh"
#include "Trans.hh"

class vlRenderer;

class vlCamera : public vlObject
{
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

 public:
  vlCamera();

  void SetPosition(float , float, float);
  void SetPosition(float a[3]);
  vlGetVectorMacro(Position,float);

  void SetFocalPoint(float , float, float);
  void SetFocalPoint(float a[3]);
  vlGetVectorMacro(FocalPoint,float);

  void SetViewUp(float , float, float);
  void SetViewUp(float a[3]);
  vlGetVectorMacro(ViewUp,float);

  void SetClippingRange(float , float);
  void SetClippingRange(float a[2]);
  vlGetVectorMacro(ClippingRange,float);

  virtual void Render(vlRenderer *ren) = 0;

  vlSetClampMacro(ViewAngle,float,1.0,179.0);
  vlGetMacro(ViewAngle,float);

  vlSetMacro(EyeAngle,float);
  vlGetMacro(EyeAngle,float);

  void SetThickness(float);
  vlGetMacro(Thickness,float);

  void SetDistance(float);
  vlGetMacro(Distance,float);

  vlSetMacro(Switch,int);
  vlGetMacro(Switch,int);
  vlBooleanMacro(Switch,int);

  float GetTwist();
  void CalcViewPlaneNormal();
  void CalcDistance();
  void CalcPerspectiveTransform();
  vlMatrix4x4 &GetPerspectiveTransform();
  vlGetVectorMacro(ViewPlaneNormal,float);

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

  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlCamera";};
};

#endif
