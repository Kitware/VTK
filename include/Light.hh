/*=========================================================================

  Program:   OSCAR 
  Module:    Light.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlLight_hh
#define __vlLight_hh

#include "Object.hh"

/* need for virtual function */
class vlRenderer;

class vlLight : public vlObject
{
 public:
  float FocalPoint[3];
  float Position[3];
  float Intensity;
  float Color[3];
  int   OnStatus;

 public:
  vlLight();
  virtual void Render(vlRenderer *ren,int light_index) = 0;

  void SetPosition(float, float, float);
  void SetPosition(float a[3]);
  float *GetPosition();

  void SetFocalPoint(float , float, float);
  void SetFocalPoint(float a[3]);
  float *GetFocalPoint();

  int GetStatus();
};

#endif

