/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Property.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkProperty - represent surface properties of a geometric object
// .SECTION Description
// vtkProperty is an object that represents lighting and other surface
// properties of a geometric object. The primary properties that can be 
// set are colors (object, ambient, diffuse, specular, and edge color),
// specular power, transparency of the object, the representation of the
// object (points, wireframe, or surface), and the shading method to be 
// used (flat, Gouraud, and Phong).
// .SECTION See Also
// See vtkRenderer for definition of #define's.

#ifndef __vtkProperty_hh
#define __vtkProperty_hh

#include "Render.hh"
#include "Object.hh"
#include "StrPts.hh"

class vtkRenderer;
class vtkPropertyDevice;

class vtkProperty : public vtkObject
{
public:
  vtkProperty();
  char *GetClassName() {return "vtkProperty";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Abstract interface to renderer. Each concrete subclass of vtkProperty
  // will load its data into graphics system in response to this method
  // invocation.
  virtual void Render(vtkRenderer *ren);

  void SetFlat (void);
  void SetGouraud (void);
  void SetPhong (void);
  void SetPoints (void);
  void SetWireframe (void);
  void SetSurface (void);

  // Description:
  // Get the method of representation for the object.
  vtkGetMacro(Representation,int);

  // Description:
  // Get the shading method for the object.
  vtkGetMacro(Interpolation,int);

  void SetColor(float r,float g,float b);
  void SetColor(float a[3]) { this->SetColor(a[0], a[1], a[2]); };
  vtkGetVectorMacro(Color,float,3);

  // Description:
  // Set ambient coefficient.
  vtkSetClampMacro(Ambient,float,0.0,1.0);
  vtkGetMacro(Ambient,float);

  // Description:
  // Set diffuse coefficient.
  vtkSetClampMacro(Diffuse,float,0.0,1.0);
  vtkGetMacro(Diffuse,float);

  // Description:
  // Set specular coefficient.
  vtkSetClampMacro(Specular,float,0.0,1.0);
  vtkGetMacro(Specular,float);

  // Description:
  // Set the specular power.
  vtkSetClampMacro(SpecularPower,float,0.0,100.0);
  vtkGetMacro(SpecularPower,float);

  // Description:
  // Set the object transparency.
  vtkSetClampMacro(Transparency,float,0.0,1.0);
  vtkGetMacro(Transparency,float);

  // Description:
  // Turn on/off the visibility of edges. On some renderers it is
  // possible to render the edges of geometric primitives separately
  // from the interior.
  vtkGetMacro(EdgeVisibility,int);
  vtkSetMacro(EdgeVisibility,int);
  vtkBooleanMacro(EdgeVisibility,int);

  // Description:
  // Set the ambient light color.
  vtkSetVector3Macro(AmbientColor,float);
  vtkGetVectorMacro(AmbientColor,float,3);

  // Description:
  // Set the diffuse light color.
  vtkSetVector3Macro(DiffuseColor,float);
  vtkGetVectorMacro(DiffuseColor,float,3);

  // Description:
  // Set the specular color.
  vtkSetVector3Macro(SpecularColor,float);
  vtkGetVectorMacro(SpecularColor,float,3);

  // Description:
  // Set the color of edges (if edge visibility enabled).
  vtkSetVector3Macro(EdgeColor,float);
  vtkGetVectorMacro(EdgeColor,float,3);

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
  vtkPropertyDevice *Device;
};

#endif
