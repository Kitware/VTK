/*=========================================================================

  Program:   Visualization Library
  Module:    PolyVert.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlPolyVertex - cell represents a set of 0D vertices
// .SECTION Description
// vlPolyVertex is a concrete implementation of vlCell to represent a 
// set of 3D vertices.

#ifndef __vlPolyVertex_h
#define __vlPolyVertex_h

#include "Cell.hh"

class vlPolyVertex : public vlCell
{
public:
  vlPolyVertex() {};
  vlPolyVertex(const vlPolyVertex& pp);
  char *GetClassName() {return "vlPolyVertex";};

  vlCell *MakeObject() {return new vlPolyVertex(*this);};
  int GetCellType() {return vlPOLY_VERTEX;};
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


