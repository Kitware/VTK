/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticTriangle.h
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
// .NAME vtkQuadraticTriangle - cell represents a parabolic, isoparametric triangle
// .SECTION Description
// vtkQuadraticTriangle is a concrete implementation of vtkNonLinearCell to
// represent a rwo-dimensional, isoparametric parabolic triangle. The
// interpolation is the standard finite element, quadratic isoparametric
// shape function. The cell includes three mid-edge nodes besides the three
// triangle vertices. The ordering of the three points defining the cell is 
// point ids (1-3,4-6) where id #4 is the midedge node between points
// (1,2); id #5 is the midedge node between points (2,3); and id #6 is the 
// midedge node between points (3,1).

#ifndef __vtkQuadraticTriangle_h
#define __vtkQuadraticTriangle_h

#include "vtkNonLinearCell.h"

class vtkPolyData;
class vtkQuadraticEdge;
class vtkTriangle;

class VTK_COMMON_EXPORT vtkQuadraticTriangle : public vtkNonLinearCell
{
public:
  static vtkQuadraticTriangle *New();
  vtkTypeRevisionMacro(vtkQuadraticTriangle,vtkNonLinearCell);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions 
  // of these methods.
  vtkCell *MakeObject();
  int GetCellType() {return VTK_QUADRATIC_TRIANGLE;};
  int GetCellDimension() {return 2;}
  int GetNumberOfEdges() {return 3;}
  int GetNumberOfFaces() {return 0;}
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;}

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
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, float pcoords[3], float *values, 
                   int dim, float *derivs);

  // Description:
  // Clip this edge using scalar value provided. Like contouring, except
  // that it cuts the edge to produce linear line segments.
  void Clip(float value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(float p1[3], float p2[3], float tol, float& t,
                        float x[3], float pcoords[3], int& subId);

  
  // Description:
  // Return the center of the quadratic triangle in parametric coordinates.
  int GetParametricCenter(float pcoords[3]);

  // Description:
  // Quadratic edge specific methods. 
  static void InterpolationFunctions(float pcoords[3], float weights[3]);
  static void InterpolationDerivs(float pcoords[3], float derivs[3]);

protected:
  vtkQuadraticTriangle();
  ~vtkQuadraticTriangle();

  vtkQuadraticEdge *Edge;
  vtkTriangle      *Face;

private:
  vtkQuadraticTriangle(const vtkQuadraticTriangle&);  // Not implemented.
  void operator=(const vtkQuadraticTriangle&);  // Not implemented.
};

#endif


