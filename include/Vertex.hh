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
// .NAME vlPoint - a cell that represents a point
// .SECTION Description
// vlPoint is a concrete implementation of vlCell to represent a 0D vertex.

#ifndef __vlPoint_h
#define __vlPoint_h

#include "Cell.hh"

class vlPoint : public vlCell
{
public:
  vlPoint() {};
  vlPoint(const vlPoint& p);
  char *GetClassName() {return "vlPoint";};

  vlCell *MakeObject() {return new vlPoint(*this);};
  int GetCellType() {return vlPOINT;};
  int GetCellDimension() {return 0;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vlCell *GetEdge(int edgeId) {return 0;};
  vlCell *GetFace(int faceId) {return 0;};

  void Contour(float value, vlFloatScalars *cellScalars, 
               vlFloatPoints *points, vlCellArray *verts, 
               vlCellArray *lines, vlCellArray *polys, vlFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3], 
                       int& subId, float pcoords[3], 
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);

};

#endif


