/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticPyramid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkQuadraticPyramid - cell represents a parabolic, isoparametric triangle
// .SECTION Description
// vtkQuadraticPyramid is a concrete implementation of vtkNonLinearCell to
// represent a two-dimensional, 6-node, isoparametric parabolic triangle. The
// interpolation is the standard finite element, quadratic isoparametric
// shape function. The cell includes three mid-edge nodes besides the three
// triangle vertices. The ordering of the three points defining the cell is 
// point ids (0-2,3-5) where id #3 is the midedge node between points
// (0,1); id #4 is the midedge node between points (1,2); and id #5 is the 
// midedge node between points (2,0).

// .SECTION See Also
// vtkQuadraticEdge vtkQuadraticTetra
// vtkQuadraticQuad vtkQuadraticHexahedron

#ifndef __vtkQuadraticPyramid_h
#define __vtkQuadraticPyramid_h

#include "vtkNonLinearCell.h"

class vtkPolyData;
class vtkQuadraticEdge;
class vtkQuadraticQuad;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkPyramid;
class vtkDoubleArray;

class VTK_COMMON_EXPORT vtkQuadraticPyramid : public vtkNonLinearCell
{
public:
  static vtkQuadraticPyramid *New();
  vtkTypeRevisionMacro(vtkQuadraticPyramid,vtkNonLinearCell);

  // Description:
  // Implement the vtkCell API. See the vtkCell API for descriptions 
  // of these methods.
  int GetCellType() {return VTK_QUADRATIC_PYRAMID;};
  int GetCellDimension() {return 3;}
  int GetNumberOfEdges() {return 8;}
  int GetNumberOfFaces() {return 5;}
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int faceId);

  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars, 
               vtkPointLocator *locator, vtkCellArray *verts, 
               vtkCellArray *lines, vtkCellArray *polys, 
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3], 
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values, 
                   int dim, double *derivs);
  virtual double *GetParametricCoords();

  // Description:
  // Clip this quadratic triangle using scalar value provided. Like 
  // contouring, except that it cuts the triangle to produce linear 
  // triangles.
  void Clip(double value, vtkDataArray *cellScalars, 
            vtkPointLocator *locator, vtkCellArray *tets,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // Line-edge intersection. Intersection has to occur within [0,1] parametric
  // coordinates and with specified tolerance.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);

  
  // Description:
  // Return the center of the quadratic pyramid in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Quadratic pyramid specific methods. 
  static void InterpolationFunctions(double pcoords[3], double weights[13]);
  static void InterpolationDerivs(double pcoords[3], double derivs[39]);

  // Description:
  // Given parametric coordinates compute inverse Jacobian transformation
  // matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
  // function derivatives.
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[39]);

protected:
  vtkQuadraticPyramid();
  ~vtkQuadraticPyramid();

  vtkQuadraticEdge *Edge;
  vtkQuadraticTriangle *TriangleFace;
  vtkQuadraticQuad *Face;
  vtkTetra         *Tetra;
  vtkPyramid       *Pyramid;
  vtkPointData     *PointData;
  vtkCellData      *CellData;
  vtkDoubleArray   *Scalars; //used to avoid New/Delete in contouring/clipping

  void Subdivide(vtkPointData *inPd, vtkCellData *inCd, vtkIdType cellId);

private:
  vtkQuadraticPyramid(const vtkQuadraticPyramid&);  // Not implemented.
  void operator=(const vtkQuadraticPyramid&);  // Not implemented.
};

#endif
