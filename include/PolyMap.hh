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
// .NAME vlPolyMapper - map vlPolyData to graphics primitives
// .SECTION Description
// vlPolyMapper is a mapper to map polygonal data (i.e., vlPolyData) to 
// graphics primitives. It is possible to control which geometric 
// primitives are displayed using the boolean variables provided.

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
  float *GetBounds();

  // Description:
  // Specify the input data to map.
  virtual void SetInput(vlPolyData *in);
  virtual vlPolyData* GetInput();

  // Description:
  // Control the visibility of vertices.
  vlSetMacro(VertsVisibility,int);
  vlGetMacro(VertsVisibility,int);
  vlBooleanMacro(VertsVisibility,int);

  // Description:
  // Control the visibility of lines.
  vlSetMacro(LinesVisibility,int);
  vlGetMacro(LinesVisibility,int);
  vlBooleanMacro(LinesVisibility,int);

  // Description:
  // Control the visibility of polygons.
  vlSetMacro(PolysVisibility,int);
  vlGetMacro(PolysVisibility,int);
  vlBooleanMacro(PolysVisibility,int);

  // Description:
  // Control the visibility of triangle strips.
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


