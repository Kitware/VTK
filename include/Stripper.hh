/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Stripper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkStripper - create triangle strips
// .SECTION Description
// vtkStripper is a filter that generates triangle strips from input
// polygons and triangle strips. Input polygons are assumed to be 
// triangles. (Use vtkTriangleFilter to triangulate non-triangular
// polygons.) The filter will also pass through vertices and lines, if
// requested.

#ifndef __vtkStripper_h
#define __vtkStripper_h

#include "P2PF.hh"

class vtkStripper : public vtkPolyToPolyFilter
{
public:
  vtkStripper();
  ~vtkStripper() {};
  char *GetClassName() {return "vtkStripper";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the maximum number of triangles in a triangle strip.
  vtkSetClampMacro(MaximumStripLength,int,4,MAX_CELL_SIZE-2);
  vtkGetMacro(MaximumStripLength,int);

  // Description:
  // Turn on/off passing of vertices through to output.
  vtkBooleanMacro(PassVerts,int);
  vtkSetMacro(PassVerts,int);
  vtkGetMacro(PassVerts,int);

  // Description:
  // Turn on/off passing of lines through to output.
  vtkBooleanMacro(PassLines,int);
  vtkSetMacro(PassLines,int);
  vtkGetMacro(PassLines,int);

protected:
  // Usual data generation method
  void Execute();

  int MaximumStripLength;
  // control whether vertices and lines are passed through filter
  int PassVerts;
  int PassLines;
};

#endif


