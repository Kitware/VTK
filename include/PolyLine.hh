/*========================================================================

  Program:   Visualization Toolkit
  Module:    PolyLine.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkPolyLine - cell represents a set of 1D lines
// .SECTION Description
// vtkPolyLine is a concrete implementation of vtkCell to represent a set
// of 1D lines

#ifndef __vtkPolyLine_h
#define __vtkPolyLine_h

#include "Cell.hh"
#include "Points.hh"
#include "CellArr.hh"
#include "FNormals.hh"

class vtkPolyLine : public vtkCell
{
public:
  vtkPolyLine() {};
  vtkPolyLine(const vtkPolyLine& pl);
  char *GetClassName() {return "vtkPolyLine";};

  int GenerateNormals(vtkPoints *, vtkCellArray *, vtkFloatNormals *);
  int GenerateSlidingNormals(vtkPoints *, vtkCellArray *, vtkFloatNormals *);

  vtkCell *MakeObject() {return new vtkPolyLine(*this);};
  int GetCellType() {return vtkPOLY_LINE;};
  int GetCellDimension() {return 1;};
  int GetNumberOfEdges() {return 0;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId) {return 0;};
  vtkCell *GetFace(int faceId) {return 0;};

  int CellBoundary(int subId, float pcoords[3], vtkIdList& pts);
  void Contour(float value, vtkFloatScalars *cellScalars, 
               vtkFloatPoints *points, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkFloatScalars *s);
  int EvaluatePosition(float x[3], float closestPoint[3],
                       int& subId, float pcoords[3],
                       float& dist2, float weights[MAX_CELL_SIZE]);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float weights[MAX_CELL_SIZE]);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
};

#endif


