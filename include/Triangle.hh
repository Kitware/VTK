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
// vlTriangle is a concrete implementation of vlCell to represent a triangle
// located in 3-space.

#ifndef __vlTriangle_h
#define __vlTriangle_h

#include "Cell.hh"
#include "vlMath.hh"

class vlTriangle : public vlCell
{
public:
  vlTriangle() {};
  vlTriangle(const vlTriangle& t);
  char *GetClassName() {return "vlTriangle";};

  vlCell *MakeObject() {return new vlTriangle(*this);};
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
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);

  void TriangleCenter(float p1[3], float p2[3], float p3[3], float center[3]);
  float TriangleArea(float p1[3], float p2[3], float p3[3]);

};

// Description:
// Compute the center of the triangle.
inline void vlTriangle::TriangleCenter(float p1[3], float p2[3], float p3[3],
                                       float center[3])
{
  center[0] = (p1[0]+p2[0]+p3[0]) / 3.0;
  center[1] = (p1[1]+p2[1]+p3[1]) / 3.0;
  center[2] = (p1[2]+p2[2]+p3[2]) / 3.0;
}

// Description:
// Compute the area of a triangle in 3D.
inline float vlTriangle::TriangleArea(float p1[3], float p2[3], float p3[3])
{
  vlMath math;
  float a,b,c;
  a = math.Distance2BetweenPoints(p1,p2);
  b = math.Distance2BetweenPoints(p2,p3);
  c = math.Distance2BetweenPoints(p3,p1);
  return (0.25* sqrt(fabs((double)4.0*a*c - (a-b+c)*(a-b+c))));
} 

#endif


