/*=========================================================================

  Program:   Visualization Library
  Module:    Light.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

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
  void PrintSelf(ostream& os, vlIndent indent);
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
