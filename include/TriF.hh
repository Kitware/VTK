/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TriF.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTriangleFilter - create triangle polygons from input polygons and triangle strips
// .SECTION Description
// vtkTriangleFilter generates triangles from input polygons and triangle 
// strips. The filter will also pass through vertices and lines, if
// requested.

#ifndef __vtkTriangleFilter_h
#define __vtkTriangleFilter_h

#include "P2PF.hh"

class vtkTriangleFilter : public vtkPolyToPolyFilter
{
public:
  vtkTriangleFilter() : PassVerts(1), PassLines(1) {};
  ~vtkTriangleFilter() {};
  char *GetClassName() {return "vtkTriangleFilter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off passing vertices through filter.
  vtkBooleanMacro(PassVerts,int);
  vtkSetMacro(PassVerts,int);
  vtkGetMacro(PassVerts,int);

  // Description:
  // Turn on/off passing lines through filter.
  vtkBooleanMacro(PassLines,int);
  vtkSetMacro(PassLines,int);
  vtkGetMacro(PassLines,int);

protected:
  // Usual data generation method
  void Execute();

  int PassVerts;
  int PassLines;
};

#endif


