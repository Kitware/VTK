/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Light.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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
  ~vtkLight();
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
