/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Light.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkLight - a virtual light for 3D rendering
// .SECTION Description
// vtkLight is a virtual light for 3D rendering. It provides methods to locate
// and point the light, turn it on and off, and set its brightness and color.

#ifndef __vtkLight_hh
#define __vtkLight_hh

#include "Object.hh"

/* need for virtual function */
class vtkRenderer;
class vtkLightDevice;

class vtkLight : public vtkObject
{
public:
  vtkLight();
  char *GetClassName() {return "vtkLight";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vtkLight
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Render(vtkRenderer *ren,int light_index);

  // Description:
  // Set the color of the light.
  vtkSetVector3Macro(Color,float);
  vtkGetVectorMacro(Color,float,3);

  // Description:
  // Set the position of the light.
  vtkSetVector3Macro(Position,float);
  vtkGetVectorMacro(Position,float,3);

  // Description:
  // Set the point at which the light is shining.
  vtkSetVector3Macro(FocalPoint,float);
  vtkGetVectorMacro(FocalPoint,float,3);

  // Description:
  // Set the brightness of the light.
  vtkSetMacro(Intensity,float);
  vtkGetMacro(Intensity,float);

  // Description:
  // Turn the light on/off.
  vtkSetMacro(Switch,int);
  vtkGetMacro(Switch,int);
  vtkBooleanMacro(Switch,int);

  // Description:
  // Turn positional lighting on/off.
  vtkSetMacro(Positional,int);
  vtkGetMacro(Positional,int);
  vtkBooleanMacro(Positional,int);

  // Description:
  // Set the exponent of the cosine used in positional lighting.
  vtkSetMacro(Exponent,float);
  vtkGetMacro(Exponent,float);

  // Description:
  // Set the lighting cone angle in degrees of a positional light.
  vtkSetMacro(ConeAngle,float);
  vtkGetMacro(ConeAngle,float);

  // Description:
  // Set the quadratic attenuation constants, const linear quad in order.
  vtkSetVector3Macro(AttenuationValues,float);
  vtkGetVectorMacro(AttenuationValues,float,3);

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
  vtkLightDevice *Device;
};

#endif
