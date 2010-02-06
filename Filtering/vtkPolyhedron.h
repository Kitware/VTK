/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkPolyhedron.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyhedron - a 3D cell defined by a set of polygonal faces
// .SECTION Description
// vtkPolyhedron is a concrete implementation that represents a 3D cell
// defined by a set of polygonal faces. The polyhedron should be watertight
// non-self-intersecting and manifold (each edge is used twice).

// .SECTION See Also
// vtkCell3D vtkConvecPointSet vtkHexahedron 

#ifndef __vtkPolyhedron_h
#define __vtkPolyhedron_h

#include "vtkCell3D.h"

class vtkUnstructuredGrid;
class vtkCellArray;
class vtkTriangle;
class vtkTetra;
class vtkDoubleArray;

class VTK_FILTERING_EXPORT vtkPolyhedron : public vtkCell3D
{
public:
  // Description:
  // Standard new methods.
  static vtkPolyhedron *New();
  vtkTypeRevisionMacro(vtkPolyhedron,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See vtkCell3D API for description of this method.
  virtual int HasFixedTopology() {return 0;}

  // Description:
  // See vtkCell3D API for description of these methods.
  virtual void GetEdgePoints(int vtkNotUsed(edgeId), int* &vtkNotUsed(pts)) {}
  virtual void GetFacePoints(int vtkNotUsed(faceId), int* &vtkNotUsed(pts)) {}
  virtual double *GetParametricCoords();

  // Description:
  // See the vtkCell API for descriptions of these methods.
  virtual int GetCellType() {return VTK_CONVEX_POINT_SET;}

  // Description:
  // This cell requires that it be initialized prior to access.
  virtual int RequiresInitialization() {return 1;}
  virtual void Initialize();

  // Description:
  // A convex point set has no explicit cell edge or faces; however
  // implicitly (after triangulation) it does. Currently the method
  // GetNumberOfEdges() always returns 0 while the GetNumberOfFaces() returns
  // the number of boundary triangles of the triangulation of the convex
  // point set. The method GetNumberOfFaces() triggers a triangulation of the
  // convex point set; repeated calls to GetFace() then return the boundary
  // faces. (Note: GetNumberOfEdges() currently returns 0 because it is a
  // rarely used method and hard to implement. It can be changed in the future.
  virtual int GetNumberOfEdges() {return 0;}
  virtual vtkCell *GetEdge(int) {return NULL;}
  virtual int GetNumberOfFaces();
  virtual vtkCell *GetFace(int faceId);

  // Description:
  // Satisfy the vtkCell API. This method contours by triangulating the
  // cell and then contouring the resulting tetrahedra.
  virtual void Contour(double value, vtkDataArray *cellScalars,
                       vtkIncrementalPointLocator *locator, vtkCellArray *verts,
                       vtkCellArray *lines, vtkCellArray *polys,
                       vtkPointData *inPd, vtkPointData *outPd,
                       vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);

  // Description:
  // Satisfy the vtkCell API. This method contours by triangulating the
  // cell and then adding clip-edge intersection points into the
  // triangulation; extracting the clipped region.
  virtual void Clip(double value, vtkDataArray *cellScalars,
                    vtkIncrementalPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                    int insideOut);

  // Description:
  // Satisfy the vtkCell API. This method determines the subId, pcoords,
  // and weights by triangulating the convex point set, and then
  // determining which tetrahedron the point lies in.
  virtual int EvaluatePosition(double x[3], double* closestPoint,
                               int& subId, double pcoords[3],
                               double& dist2, double *weights);

  // Description:
  // The inverse of EvaluatePosition.
  virtual void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                                double *weights);

  // Description:
  // Triangulates the cells and then intersects them to determine the
  // intersection point.
  virtual int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                                double x[3], double pcoords[3], int& subId);

  // Description:
  // Triangulate using methods of vtkOrderedTriangulator.
  virtual int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);

  // Description:
  // Computes derivatives by triangulating and from subId and pcoords,
  // evaluating derivatives on the resulting tetrahedron.
  virtual void Derivatives(int subId, double pcoords[3], double *values,
                           int dim, double *derivs);

  // Description:
  // Returns the set of points forming a face of the triangulation of these
  // points that are on the boundary of the cell that are closest
  // parametrically to the point specified.
  virtual int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);

  // Description:
  // Return the center of the cell in parametric coordinates.
  virtual int GetParametricCenter(double pcoords[3]);

  // Description:
  // A convex point set is triangulated prior to any operations on it so
  // it is not a primary cell, it is a composite cell.
  int IsPrimaryCell() {return 0;}

  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double *sf);
  virtual void InterpolateDerivs(double pcoords[3], double *derivs);

protected:
  vtkPolyhedron();
  ~vtkPolyhedron();

  vtkTetra       *Tetra;
  vtkIdList      *TetraIds;
  vtkPoints      *TetraPoints;
  vtkDoubleArray *TetraScalars;

  vtkCellArray   *BoundaryTris;
  vtkTriangle    *Triangle;
  vtkDoubleArray *ParametricCoords;

private:
  vtkPolyhedron(const vtkPolyhedron&);  // Not implemented.
  void operator=(const vtkPolyhedron&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline int vtkPolyhedron::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return 0;
}

#endif



