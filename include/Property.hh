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
  virtual void Render(vlRenderer *ren) = 0;
  void SetFlat (void);
  void SetGouraud (void);
  void SetPhong (void);
  void SetPoints (void);
  void SetWireframe (void);
  void SetSurface (void);
  void SetColor(float R,float G,float B);
  float GetTransparency();
  int   GetRepresentation();
  int   GetInterpolation();
};

#endif
