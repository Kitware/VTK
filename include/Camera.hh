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

class vlRenderer;

class vlCamera : public vlObject
{
 public:
  float FocalPoint[3];
  float Position[3];
  float ViewUp[3];
  float ViewAngle;
  float ClippingRange[2];
  float EyeAngle;
  int   LeftEye;
  int   Switch;

 public:
  vlCamera();

  void SetPosition(float , float, float);
  void SetPosition(float a[3]);
  float *GetPosition();

  void SetFocalPoint(float , float, float);
  void SetFocalPoint(float a[3]);
  float *GetFocalPoint();

  void SetViewUp(float , float, float);
  void SetViewUp(float a[3]);
  float *GetViewUp();

  virtual void Render(vlRenderer *ren) = 0;

  vlSetMacro(EyeAngle,float);
  vlGetMacro(EyeAngle,float);

  vlSetMacro(Switch,int);
  vlGetMacro(Switch,int);
  vlBooleanMacro(Switch,int);

  float GetTwist();
  float *GetViewPlaneNormal();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlCamera";};
};

#endif
