/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolyMap.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyMapper - map vtkPolyData to graphics primitives
// .SECTION Description
// vtkPolyMapper is a mapper to map polygonal data (i.e., vtkPolyData) to 
// graphics primitives. It is possible to control which geometric 
// primitives are displayed using the boolean variables provided.

#ifndef __vtkPolyMapper_h
#define __vtkPolyMapper_h

#include "Mapper.hh"
#include "PolyData.hh"
#include "Renderer.hh"

class vtkPolyMapper : public vtkMapper 
{
public:
  vtkPolyMapper();
  ~vtkPolyMapper();
  char *GetClassName() {return "vtkPolyMapper";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Render(vtkRenderer *ren);
  float *GetBounds();

  // Description:
  // Specify the input data to map.
  void SetInput(vtkPolyData *in);
  void SetInput(vtkPolyData& in) {this->SetInput(&in);};

  // Description:
  // Control the visibility of vertices.
  vtkSetMacro(VertsVisibility,int);
  vtkGetMacro(VertsVisibility,int);
  vtkBooleanMacro(VertsVisibility,int);

  // Description:
  // Control the visibility of lines.
  vtkSetMacro(LinesVisibility,int);
  vtkGetMacro(LinesVisibility,int);
  vtkBooleanMacro(LinesVisibility,int);

  // Description:
  // Control the visibility of polygons.
  vtkSetMacro(PolysVisibility,int);
  vtkGetMacro(PolysVisibility,int);
  vtkBooleanMacro(PolysVisibility,int);

  // Description:
  // Control the visibility of triangle strips.
  vtkSetMacro(StripsVisibility,int);
  vtkGetMacro(StripsVisibility,int);
  vtkBooleanMacro(StripsVisibility,int);

protected:
  vtkGeometryPrimitive *Verts;
  vtkGeometryPrimitive *Lines;
  vtkGeometryPrimitive *Polys;
  vtkGeometryPrimitive *Strips;

  vtkColorScalars *Colors;

  int VertsVisibility;
  int LinesVisibility;
  int PolysVisibility;
  int StripsVisibility;

};

#endif


