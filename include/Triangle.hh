/*=========================================================================

  Program:   Visualization Library
  Module:    Triangle.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTriangle - a cell that represents a triangle
// .SECTION Description
// vlTriangle is a concrete implementation of vlCell to represent a 2D 
// triangle.

#ifndef __vlTriangle_h
#define __vlTriangle_h

#include "Cell.hh"

class vlTriangle : public vlCell
{
public:
  vlTriangle() {};
  char *GetClassName() {return "vlTriangle";};

  int GetCellType() {return vlTRIANGLE;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 3;};
  int GetNumberOfFaces() {return 0;};
  vlCell *GetEdge(int edgeId);
  vlCell *GetFace(int faceId) {return 0;};

  void Contour(float value, vlFloatScalars *cellScalars, 
               vlFloatPoints *points, vlCellArray *verts,
               vlCellArray *lines, vlCellArray *polys, 
               vlFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);

};

#endif


