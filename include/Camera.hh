/*=========================================================================

  Program:   OSCAR 
  Module:    Camera.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlCamera_hh
#define __vlCamera_hh

class vlRenderer;

class vlCamera
{
 public:
  float FocalPoint[3];
  float Position[3];
  float ViewUp[3];
  float ViewAngle;
  float ClippingRange[2];
  
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
};

#endif
