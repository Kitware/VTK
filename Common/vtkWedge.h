/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWedge.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkWedge - a 3D cell that represents a linear wedge
// .SECTION Description
// vtkWedge is a concrete implementation of vtkCell to represent a linear 3D
// wedge. A wedge consists of two triangular and three quadrilateral faces
// and is defined by the six points (0-5). vtkWedge uses the standard
// isoparametric shape functions for a linear pyramid. The pyramid is defined
// by the six points (0-5) where (0,1,2) is the base of the wedge which,
// using the right hand rule, forms a triangle whose normal points in
// the direction of the opposite triangular face (3,4,5).

#ifndef __vtkWedge_h
#define __vtkWedge_h

#include "vtkCell3D.h"
#include "vtkLine.h"
#include "vtkTriangle.h"
#include "vtkQuad.h"

class vtkUnstructuredGrid;

class VTK_COMMON_EXPORT vtkWedge : public vtkCell3D
{
public:
  static vtkWedge *New();
  vtkTypeRevisionMacro(vtkWedge,vtkCell);

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int edgeId, int* &pts);
  virtual void GetFacePoints(int faceId, int* &pts);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_WEDGE;}
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 9;}
  int GetNumberOfFaces() {return 5;}
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);
  int CellBoundary(int subId, float pcoords[3], vtkIdList *pts);
  void Contour(float value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(float x[3], float* closestPoint,
                       int& subId, float pcoords[3],
                       float& dist2, float *weights);
  void EvaluateLocation(int& subId, float pcoords[3], float x[3],
                        float *weights);
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Return the center of the wedge in parametric coordinates.
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // Wedge specific methods for computing interpolation functions and
  // derivatives.
  static void InterpolationFunctions(float pcoords[3], float weights[6]);
  static void InterpolationDerivs(float pcoords[3], float derivs[18]);
  int JacobianInverse(float pcoords[3], double **inverse, float derivs[18]);
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);

protected:
  vtkWedge();
  ~vtkWedge();

  vtkLine *Line;
  vtkTriangle *Triangle;
  vtkQuad *Quad;

private:
  vtkWedge(const vtkWedge&);  // Not implemented.
  void operator=(const vtkWedge&);  // Not implemented.
};

inline int vtkWedge::GetParametricCenter(float pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.333333;
  pcoords[2] = 0.5;
  return 0;
}

#endif



