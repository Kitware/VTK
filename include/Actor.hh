/*=========================================================================

  Program:   Visualization Library
  Module:    Actor.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

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
  void PrintSelf(ostream& os, vlIndent indent);

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

