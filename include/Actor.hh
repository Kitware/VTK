/*=========================================================================

  Program:   OSCAR 
  Module:    Actor.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlActor_hh
#define __vlActor_hh

#include "Object.hh"
#include "Property.hh"
#include "Mapper.hh"

class vlRenderer;
class vlMapper;

class vlActor : public vlObject
{
 public:
  vlActor();
  ~vlActor();
  void Render(vlRenderer *ren);

  vlGetMacro(Visibility,int);
  vlSetMacro(Visibility,int);
  vlBooleanMacro(Visibility,int);

  vlGetMacro(Pickable,int);
  vlSetMacro(Pickable,int);
  vlBooleanMacro(Pickable,int);

  vlGetMacro(Dragable,int);
  vlSetMacro(Dragable,int);
  vlBooleanMacro(Dragable,int);

  void GetCompositeMatrix(float mat[4][4]);
  void SetMapper(vlMapper *m);
  vlMapper *GetMapper();
  vlProperty *Property; 
  char *GetClassName() {return "vlActor";};

protected:
  vlMapper *Mapper;
  float Origin[3];
  float Position[3];
  float Orientation[3];
  float Scale[3];
  int   Visibility;
  int   Pickable;
  int   Dragable;
};

#endif

