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
protected:
  float FocalPoint[3];
  float Position[3];
  float Intensity;
  float Color[3];
  int   Switch;

public:
  vlLight();
  char *GetClassName() {return "vlLight";};
  virtual void Render(vlRenderer *ren,int light_index) = 0;

  vlSetVector3Macro(Color,float);
  vlGetVectorMacro(Color,float);

  vlSetVector3Macro(Position,float);
  vlGetVectorMacro(Position,float);

  vlSetVector3Macro(FocalPoint,float);
  vlGetVectorMacro(FocalPoint,float);

  vlSetMacro(Intensity,float);
  vlGetMacro(Intensity,float);

  vlSetMacro(Switch,int);
  vlGetMacro(Switch,int);
  vlBooleanMacro(Switch,int);

};

#endif
