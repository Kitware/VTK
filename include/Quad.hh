/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Quad.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkQuad - a cell that represents a four sided quadrilateral
// .SECTION Description
// vtkQuad is a concrete implementation of vtkCell to represent a 2D 
// quadrilateral.

#ifndef __vtkQuad_h
#define __vtkQuad_h

#include "Cell.hh"

class vtkQuad : public vtkCell
{
public:
  vtkQuad() {};
  vtkQuad(const vtkQuad& q);
  char *GetClassName() {return "vtkQuad";};

  vtkCell *MakeObject() {return new vtkQuad(*this);};
  int GetCellType() {return vtkQUAD;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 4;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId) {return 0;};

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkFloatPoints *points, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, vtkFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  void InterpolationFunctions(float pcoords[3], float sf[4]);
  void InterpolationDerivs(float pcoords[3], float derivs[12]);

};

#endif


