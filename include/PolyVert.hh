/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PolyVert.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyVertex - cell represents a set of 0D vertices
// .SECTION Description
// vtkPolyVertex is a concrete implementation of vtkCell to represent a 
// set of 3D vertices.

#ifndef __vtkPolyVertex_h
#define __vtkPolyVertex_h

#include "Cell.hh"

class vtkPolyVertex : public vtkCell
{
public:
  vtkPolyVertex() {};
  vtkPolyVertex(const vtkPolyVertex& pp);
  char *GetClassName() {return "vtkPolyVertex";};

  vtkCell *MakeObject() {return new vtkPolyVertex(*this);};
  int GetCellType() {return vtkPOLY_VERTEX;};
  int GetCellDimension() {return 0;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId) {return 0;};
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


};

#endif


