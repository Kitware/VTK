/*=========================================================================

  Program:   Visualization Library
  Module:    TriStrip.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTriangleStrip - a cell that represents a triangle strip
// .SECTION Description
// vlTriangleStrip is a concrete implementation of vlCell to represent a 2D 
// triangle strip. A triangle strip is a compact representation of triangles
// connected edge to edge in strip fashion. The connectivity of a triangle 
// strip is three points defining an initial triangle, then for each 
// additional triangle, a single point that, combined with the previous two
// points, defines the next triangle.

#ifndef __vlTriangleStrip_h
#define __vlTriangleStrip_h

#include "Cell.hh"

class vlTriangleStrip : public vlCell
{
public:
  vlTriangleStrip() {};
  vlTriangleStrip(const vlTriangleStrip& ts);
  char *GetClassName() {return "vlTriangleStrip";};

  vlCell *MakeObject() {return new vlTriangleStrip(*this);};
  int GetCellType() {return vlTRIANGLE_STRIP;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() {return 0;};
  vlCell *GetEdge(int edgeId);
  vlCell *GetFace(int faceId) {return 0;};

  int CellBoundary(int subId, float pcoords[3], vlIdList& pts);
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
};

#endif


