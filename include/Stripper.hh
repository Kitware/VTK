/*=========================================================================

  Program:   Visualization Library
  Module:    Stripper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStripper - create triangle strips
// .SECTION Description
// vlStripper is a filter that generates triangle strips from input
// polygons and triangle strips. Input polygons are assumed to be 
// triangles. (Use vlTriangleFilter to triangulate non-triangular
// polygons.) The filter will also pass through vertices and lines, if
// requested.

#ifndef __vlStripper_h
#define __vlStripper_h

#include "P2PF.hh"

class vlStripper : public vlPolyToPolyFilter
{
public:
  vlStripper();
  ~vlStripper() {};
  char *GetClassName() {return "vlStripper";};
  void PrintSelf(ostream& os, vlIndent indent);

  // Description:
  // Specify the maximum number of triangles in a triangle strip.
  vlSetClampMacro(MaximumStripLength,int,4,MAX_CELL_SIZE-2);
  vlGetMacro(MaximumStripLength,int);

  // Description:
  // Turn on/off passing of vertices through to output.
  vlBooleanMacro(PassVerts,int);
  vlSetMacro(PassVerts,int);
  vlGetMacro(PassVerts,int);

  // Description:
  // Turn on/off passing of lines through to output.
  vlBooleanMacro(PassLines,int);
  vlSetMacro(PassLines,int);
  vlGetMacro(PassLines,int);

protected:
  // Usual data generation method
  void Execute();

  int MaximumStripLength;
  // control whether vertices and lines are passed through filter
  int PassVerts;
  int PassLines;
};

#endif


