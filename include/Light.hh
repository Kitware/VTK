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
// .NAME vlLight - a virtual light for 3D rendering
// .SECTION Description
// vlLight is a virtual light for 3D rendering. It provides methods to locate
// and point the light, turn it on and off, and set its brightness and color.

#ifndef __vlLight_hh
#define __vlLight_hh

#include "Object.hh"

/* need for virtual function */
class vlRenderer;

class vlLight : public vlObject
{
public:
  vlLight();
  char *GetClassName() {return "vlLight";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vlLight
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Render(vlRenderer *ren,int light_index) = 0;

  // Description:
  // Set the color of the light.
  vlSetVector3Macro(Color,float);
  vlGetVectorMacro(Color,float,3);

  // Description:
  // Set the position of the light.
  vlSetVector3Macro(Position,float);
  vlGetVectorMacro(Position,float,3);

  // Description:
  // Set the point at which the light is shining.
  vlSetVector3Macro(FocalPoint,float);
  vlGetVectorMacro(FocalPoint,float,3);

  // Description:
  // Set the brightness of the light.
  vlSetMacro(Intensity,float);
  vlGetMacro(Intensity,float);

  // Description:
  // Turn the light on/off.
  vlSetMacro(Switch,int);
  vlGetMacro(Switch,int);
  vlBooleanMacro(Switch,int);

  // Description:
  // Turn positional lighting on/off.
  vlSetMacro(Positional,int);
  vlGetMacro(Positional,int);
  vlBooleanMacro(Positional,int);

  // Description:
  // Set the exponent of the cosine used in positional lighting.
  vlSetMacro(Exponent,float);
  vlGetMacro(Exponent,float);

  // Description:
  // Set the lighting cone angle in degrees of a positional light.
  vlSetMacro(ConeAngle,float);
  vlGetMacro(ConeAngle,float);

  // Description:
  // Set the quadratic attenuation constants, const linear quad in order.
  vlSetVector3Macro(AttenuationValues,float);
  vlGetVectorMacro(AttenuationValues,float,3);

protected:
  float FocalPoint[3];
  float Position[3];
  float Intensity;
  float Color[3];
  int   Switch;
  int   Positional;
  float Exponent;
  float ConeAngle;
  float AttenuationValues[3];
};

#endif
