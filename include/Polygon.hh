/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Polygon.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolygon - a cell that represents a n-sided polygon
// .SECTION Description
// vtkPolygon is a concrete implementation of vtkCell to represent a 2D 
// n-sided polygon. The polygons cannot have any internal holes, and cannot
// self-intersect.

#ifndef __vtkPolygon_h
#define __vtkPolygon_h

#include "Cell.hh"
#include "Points.hh"

class vtkPolygon : public vtkCell
{
public:
  vtkPolygon() {};
  vtkPolygon(const vtkPolygon& p);
  char *GetClassName() {return "vtkPolygon";};

  void ComputeNormal(vtkPoints *p, int numPts, int *pts, float n[3]);
  void ComputeNormal(float v1[3], float v2[3], float v3[3], float n[3]);
  void ComputeNormal(vtkFloatPoints *p, float n[3]);

  vtkCell *MakeObject() {return new vtkPolygon(*this);};
  int GetCellType() {return vtkPOLYGON;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId) {return 0;};

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkFloatPoints *points,vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, vtkFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);

  void ComputeWeights(float x[3], float weights[MAX_CELL_SIZE]);

  int ParameterizePolygon(float p0[3], float p10[3], float &l10, 
                          float p20[3], float &l20, float n[3]);

  int PointInPolygon(float bounds[6], float x[3], float n[3]);  

  int Triangulate(vtkIdList &outTris);
  int FastTriangulate(int numVerts, int *verts, vtkIdList& Tris);
  int CanSplitLoop(int fedges[2], int numVerts, int *verts, int& n1, int *l1,
                   int& n2, int *l2, float& ar);
  void SplitLoop (int fedges[2], int numVerts, int *verts, int& n1, int *l1, 
                  int& n2, int* l2);

};

#endif

