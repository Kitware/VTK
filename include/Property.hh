/*=========================================================================

  Program:   OSCAR 
  Module:    Property.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlProperty_hh
#define __vlProperty_hh

#include "Render.hh"
#include "Object.hh"

class vlRenderer;

class vlProperty : public vlObject
{
 public:
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
  int   Interpolation;   /* gouraud */
  int   Representation;  /* solid */
  int   EdgeVisibility;
  int   Backface;
  int   Subdivide;

 public:
  vlProperty();
  char *GetClassName() {return "vlProperty";};
  virtual void Render(vlRenderer *ren) = 0;

  void SetFlat (void);
  void SetGouraud (void);
  void SetPhong (void);
  void SetPoints (void);
  void SetWireframe (void);
  void SetSurface (void);

  vlGetMacro(Representation,int);
  vlGetMacro(Interpolation,int);

  void SetColor(float r,float g,float b);
  void SetColor(float a[3]) { this->SetColor(a[0], a[1], a[2]); };
  vlGetVectorMacro(Color,float);

  vlGetMacro(Ambient,float);
  vlSetClampMacro(Ambient,float,0.0,1.0);

  vlGetMacro(Diffuse,float);
  vlSetClampMacro(Diffuse,float,0.0,1.0);

  vlGetMacro(Specular,float);
  vlSetClampMacro(Specular,float,0.0,1.0);

  vlGetMacro(SpecularPower,float);
  vlSetClampMacro(SpecularPower,float,0.0,100.0);

  vlGetMacro(Transparency,float);
  vlSetClampMacro(Transparency,float,0.0,1.0);

  vlGetMacro(EdgeVisibility,int);
  vlSetMacro(EdgeVisibility,int);
  vlBooleanMacro(EdgeVisibility,int);

  vlGetMacro(Subdivide,int);
  vlSetMacro(Subdivide,int);
  vlBooleanMacro(Subdivide,int);

  vlSetVector3Macro(AmbientColor,float);
  vlGetVectorMacro(AmbientColor,float);

  vlSetVector3Macro(DiffuseColor,float);
  vlGetVectorMacro(DiffuseColor,float);

  vlSetVector3Macro(SpecularColor,float);
  vlGetVectorMacro(SpecularColor,float);

  vlSetVector3Macro(EdgeColor,float);
  vlGetVectorMacro(EdgeColor,float);
};

#endif
