/*=========================================================================

  Program:   Visualization Library
  Module:    Vertex.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlVertex - a cell that represents a 3D point
// .SECTION Description
// vlVertex is a concrete implementation of vlCell to represent a 3D point.

#ifndef __vlVertex_h
#define __vlVertex_h

#include "Cell.hh"

class vlVertex : public vlCell
{
public:
  vlVertex() {};
  vlVertex(const vlVertex& p);
  char *GetClassName() {return "vlVertex";};

  vlCell *MakeObject() {return new vlVertex(*this);};
  int GetCellType() {return vlVERTEX;};
  int GetCellDimension() {return 0;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vlCell *GetEdge(int edgeId) {return 0;};
  vlCell *GetFace(int faceId) {return 0;};
  int CellBoundary(int subId, float pcoords[3], vlIdList& pts);

  void Contour(float value, vlFloatScalars *cellScalars, 
               vlFloatPoints *points, vlCellArray *verts, 
               vlCellArray *lines, vlCellArray *polys, vlFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3], 
                       int& subId, float pcoords[3], 
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);


};

#endif


