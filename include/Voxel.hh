/*=========================================================================

  Program:   Visualization Library
  Module:    Voxel.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlVoxel - a cell that represents a 3D orthogonal parallelepiped
// .SECTION Description
// vlVoxel is a concrete implementation of vlCell to represent a 3D
// orthogonal parallelpiped. Unlike vlHexahedeon, vlVoxel has interior angles
// of 90 degrees, and sides are parallel to coordinate axes. This results 
// in large increases in computational performance.

#ifndef __vlVoxel_h
#define __vlVoxel_h

#include "Cell.hh"

class vlVoxel : public vlCell
{
public:
  vlVoxel() {};
  vlVoxel(const vlVoxel& b);
  char *GetClassName() {return "vlVoxel";};

  vlCell *MakeObject() {return new vlVoxel(*this);};
  int GetCellType() {return vlVOXEL;};
  int GetCellDimension() {return 3;};
  int GetNumberOfEdges() {return 12;};
  int GetNumberOfFaces() {return 6;};
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

  void InterpolationFunctions(float pcoords[3], float weights[8]);

};

#endif


