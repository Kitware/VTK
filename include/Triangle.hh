/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Triangle.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkTriangle - a cell that represents a triangle
// .SECTION Description
// vtkTriangle is a concrete implementation of vtkCell to represent a triangle
// located in 3-space.

#ifndef __vtkTriangle_h
#define __vtkTriangle_h

#include "Cell.hh"
#include "vtkMath.hh"

class vtkTriangle : public vtkCell
{
public:
  vtkTriangle() {};
  vtkTriangle(const vtkTriangle& t);
  char *GetClassName() {return "vtkTriangle";};

  vtkCell *MakeObject() {return new vtkTriangle(*this);};
  int GetCellType() {return vtkTRIANGLE;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 3;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId) {return 0;};

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkFloatPoints *points, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);

  void TriangleCenter(float p1[3], float p2[3], float p3[3], float center[3]);
  float TriangleArea(float p1[3], float p2[3], float p3[3]);

private:
  int _EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);

};

// Description:
// Compute the center of the triangle.
inline void vtkTriangle::TriangleCenter(float p1[3], float p2[3], float p3[3],
                                       float center[3])
{
  center[0] = (p1[0]+p2[0]+p3[0]) / 3.0;
  center[1] = (p1[1]+p2[1]+p3[1]) / 3.0;
  center[2] = (p1[2]+p2[2]+p3[2]) / 3.0;
}

// Description:
// Compute the area of a triangle in 3D.
inline float vtkTriangle::TriangleArea(float p1[3], float p2[3], float p3[3])
{
  vtkMath math;
  float a,b,c;
  a = math.Distance2BetweenPoints(p1,p2);
  b = math.Distance2BetweenPoints(p2,p3);
  c = math.Distance2BetweenPoints(p3,p1);
  return (0.25* sqrt(fabs((double)4.0*a*c - (a-b+c)*(a-b+c))));
} 

#endif


