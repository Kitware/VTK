/*========================================================================

  Program:   Visualization Library
  Module:    PolyLine.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyLine - cell represents a set of 1D lines
// .SECTION Description
// vlPolyLine is a concrete implementation of vlCell to represent a set
// of 1D lines

#ifndef __vlPolyLine_h
#define __vlPolyLine_h

#include "Cell.hh"
#include "Points.hh"
#include "CellArr.hh"
#include "FNormals.hh"

class vlPolyLine : public vlCell
{
public:
  vlPolyLine() {};
  vlPolyLine(const vlPolyLine& pl);
  char *GetClassName() {return "vlPolyLine";};

  int GenerateNormals(vlPoints *, vlCellArray *, vlFloatNormals *);
  int GenerateSlidingNormals(vlPoints *, vlCellArray *, vlFloatNormals *);

  vlCell *MakeObject() {return new vlPolyLine(*this);};
  int GetCellType() {return vlPOLY_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vlCell *GetEdge(int edgeId) {return 0;};
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


