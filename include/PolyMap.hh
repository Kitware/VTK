/*=========================================================================

  Program:   Visualization Library
  Module:    PolyMap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// PolyMapper takes PolyData as input
//
#ifndef __vlPolyMapper_h
#define __vlPolyMapper_h

#include "Mapper.hh"
#include "PolyData.hh"
#include "Renderer.hh"

class vlPolyMapper : public vlMapper 
{
public:
  vlPolyMapper();
  ~vlPolyMapper();
  char *GetClassName() {return "vlPolyMapper";};
  void PrintSelf(ostream& os, vlIndent indent);

  void Render(vlRenderer *ren);
  virtual void SetInput(vlPolyData *in);
  virtual vlPolyData* GetInput();

  vlSetMacro(VertsVisibility,int);
  vlGetMacro(VertsVisibility,int);
  vlBooleanMacro(VertsVisibility,int);

  vlSetMacro(LinesVisibility,int);
  vlGetMacro(LinesVisibility,int);
  vlBooleanMacro(LinesVisibility,int);

  vlSetMacro(PolysVisibility,int);
  vlGetMacro(PolysVisibility,int);
  vlBooleanMacro(PolysVisibility,int);

  vlSetMacro(StripsVisibility,int);
  vlGetMacro(StripsVisibility,int);
  vlBooleanMacro(StripsVisibility,int);

protected:
  vlPolyData *Input;
  vlGeometryPrimitive *Verts;
  vlGeometryPrimitive *Lines;
  vlGeometryPrimitive *Polys;
  vlGeometryPrimitive *Strips;
  int VertsVisibility;
  int LinesVisibility;
  int PolysVisibility;
  int StripsVisibility;
};

#endif


