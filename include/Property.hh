/*=========================================================================

  Program:   Visualization Library
  Module:    Property.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlProperty - represent surface properties of a geometric object
// .SECTION Description
// vlProperty is an object that represents lighting and other surface
// properties of a geometric object. The primary properties that can be 
// set are colors (object, ambient, diffuse, specular, and edge color),
// specular power, transparency of the object, the representation of the
// object (points, wireframe, or surface), and the shading method to be 
// used (flat, Gouraud, and Phong).
// .SECTION See Also
// See vlRenderer for definition of #define's.

#ifndef __vlProperty_hh
#define __vlProperty_hh

#include "Render.hh"
#include "Object.hh"

class vlRenderer;

class vlProperty : public vlObject
{
public:
  vlProperty();
  char *GetClassName() {return "vlProperty";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vlProperty
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Render(vlRenderer *ren) = 0;

  void SetFlat (void);
  void SetGouraud (void);
  void SetPhong (void);
  void SetPoints (void);
  void SetWireframe (void);
  void SetSurface (void);

  // Description:
  // Get the method of representation for the object.
  vlGetMacro(Representation,int);

  // Description:
  // Get the shading method for the object.
  vlGetMacro(Interpolation,int);

  void SetColor(float r,float g,float b);
  void SetColor(float a[3]) { this->SetColor(a[0], a[1], a[2]); };
  vlGetVectorMacro(Color,float,3);

  // Description:
  // Set ambient coefficient.
  vlSetClampMacro(Ambient,float,0.0,1.0);
  vlGetMacro(Ambient,float);

  // Description:
  // Set diffuse coefficient.
  vlSetClampMacro(Diffuse,float,0.0,1.0);
  vlGetMacro(Diffuse,float);

  // Description:
  // Set specular coefficient.
  vlSetClampMacro(Specular,float,0.0,1.0);
  vlGetMacro(Specular,float);

  // Description:
  // Set the specular power.
  vlSetClampMacro(SpecularPower,float,0.0,100.0);
  vlGetMacro(SpecularPower,float);

  // Description:
  // Set the object transparency.
  vlSetClampMacro(Transparency,float,0.0,1.0);
  vlGetMacro(Transparency,float);

  // Description:
  // Turn on/off the visibility of edges. On some renderers it is
  // possible to render the edges of geometric primitives separately
  // from the interior.
  vlGetMacro(EdgeVisibility,int);
  vlSetMacro(EdgeVisibility,int);
  vlBooleanMacro(EdgeVisibility,int);

  // Description:
  // Turn on/off screen subdivision. Screen subdivision is used to perform
  // aliasing on the image.
  vlGetMacro(Subdivide,int);
  vlSetMacro(Subdivide,int);
  vlBooleanMacro(Subdivide,int);

  // Description:
  // Set the ambient light color.
  vlSetVector3Macro(AmbientColor,float);
  vlGetVectorMacro(AmbientColor,float,3);

  // Description:
  // Set the diffuse light color.
  vlSetVector3Macro(DiffuseColor,float);
  vlGetVectorMacro(DiffuseColor,float,3);

  // Description:
  // Set the specular color.
  vlSetVector3Macro(SpecularColor,float);
  vlGetVectorMacro(SpecularColor,float,3);

  // Description:
  // Set the color of edges (if edge visibility enabled).
  vlSetVector3Macro(EdgeColor,float);
  vlGetVectorMacro(EdgeColor,float,3);

protected:
  float Color[3];
  float AmbientColor[3];
  float DiffuseColor[3];
  float SpecularColor[3];
  float EdgeColor[3];
  float Ambient;
  float Diffuse;
  float Specular;
  float SpecularPower;
  float Transparency;
  int   Interpolation; 
  int   Representation;
  int   EdgeVisibility;
  int   Backface;
  int   Subdivide;

};

#endif
