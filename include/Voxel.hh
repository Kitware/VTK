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
// .NAME vlBrick - a cell that represents a 3D orthogonal parallelpiped
// .SECTION Description
// vlBrick is a concrete implementation of vlCell to represent a 3D
// orthogonal parallelpiped. Unlike vlHexahedeon, vlBrick has corners
// that are at 90 degrees. This results in large increases in computational
// performance.

#ifndef __vlBrick_h
#define __vlBrick_h

#include "Cell.hh"

class vlBrick : public vlCell
{
public:
  vlBrick() {};
  vlBrick(const vlBrick& b);
  char *GetClassName() {return "vlBrick";};

  vlCell *MakeObject() {return new vlBrick(*this);};
  int GetCellType() {return vlBRICK;};
  int GetCellDimension() {return 3;};
  int GetNumberOfEdges() {return 12;};
  int GetNumberOfFaces() {return 6;};
  vlCell *GetEdge(int edgeId);
  vlCell *GetFace(int faceId);

  void Contour(float value, vlFloatScalars *cellScalars, 
               vlFloatPoints *points, vlCellArray *verts, 
               vlCellArray *lines, vlCellArray *polys, vlFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);

  void ShapeFunctions(float pcoords[3], float sf[8]);

};

#endif


