/*=========================================================================

  Program:   Visualization Library
  Module:    Tetra.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlTetra - a cell that represents a tetrahedron
// .SECTION Description
// vlTetra is a concrete implementation of vlCell to represent a 3D
// tetrahedron.

#ifndef __vlTetra_h
#define __vlTetra_h

#include "Cell.hh"

class vlTetra : public vlCell
{
public:
  vlTetra() {};
  vlTetra(const vlTetra& t);
  char *GetClassName() {return "vlTetra";};

  vlCell *MakeObject() {return new vlTetra(*this);};
  int GetCellType() {return vlTETRA;};
  int GetCellDimension() {return 3;};
  int GetNumberOfEdges() {return 6;};
  int GetNumberOfFaces() {return 4;};
  vlCell *GetEdge(int edgeId);
  vlCell *GetFace(int faceId);

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



