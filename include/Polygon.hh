/*=========================================================================

  Program:   Visualization Library
  Module:    Polygon.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolygon - a cell that represents a n-sided polygon
// .SECTION Description
// vlPolygon is a concrete implementation of vlCell to represent a 2D 
// n-sided polygon. The polygons cannot have any internal holes, and cannot
// self-intersect.

#ifndef __vlPolygon_h
#define __vlPolygon_h

#include "Cell.hh"
#include "Points.hh"

class vlPolygon : public vlCell
{
public:
  vlPolygon() {};
  char *GetClassName() {return "vlPolygon";};

  void ComputeNormal(vlPoints *p, int numPts, int *pts, float n[3]);
  void ComputeNormal(float v1[3], float v2[3], float v3[3], float n[3]);
  void ComputeNormal(vlFloatPoints *p, float n[3]);

  int GetCellType() {return vlPOLYGON;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() {return 0;};
  vlCell *GetEdge(int edgeId);
  vlCell *GetFace(int faceId) {return 0;};

  void Contour(float value, vlFloatScalars *cellScalars, 
               vlFloatPoints *points,vlCellArray *verts, 
               vlCellArray *lines, vlCellArray *polys, vlFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);
  void ComputeWeights(float x[3], float weights[MAX_CELL_SIZE]);

  int ParameterizePolygon(float p0[3], float p10[3], float &l10, 
                          float p20[3], float &l20, float n[3]);

  int PointInPolygon(float bounds[6], float x[3], float n[3]);  

  int Triangulate(vlIdList &outTris);
  int FastTriangulate(int numVerts, int *verts, vlIdList& Tris);
  int CanSplitLoop(int fedges[2], int numVerts, int *verts, int& n1, int *l1,
                   int& n2, int *l2, float& ar);
  void SplitLoop (int fedges[2], int numVerts, int *verts, int& n1, int *l1, 
                  int& n2, int* l2);


};

#endif


