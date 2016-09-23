/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvexPointSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkConvexPointSet
 * @brief   a 3D cell defined by a set of convex points
 *
 * vtkConvexPointSet is a concrete implementation that represents a 3D cell
 * defined by a convex set of points. An example of such a cell is an octant
 * (from an octree). vtkConvexPointSet uses the ordered triangulations
 * approach (vtkOrderedTriangulator) to create triangulations guaranteed to
 * be compatible across shared faces. This allows a general approach to
 * processing complex, convex cell types.
 *
 * @sa
 * vtkHexahedron vtkPyramid vtkTetra vtkVoxel vtkWedge
*/

#ifndef vtkConvexPointSet_h
#define vtkConvexPointSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell3D.h"

class vtkUnstructuredGrid;
class vtkCellArray;
class vtkTriangle;
class vtkTetra;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkConvexPointSet : public vtkCell3D
{
public:
  static vtkConvexPointSet *New();
  vtkTypeMacro(vtkConvexPointSet,vtkCell3D);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * See vtkCell3D API for description of this method.
   */
  virtual int HasFixedTopology() {return 0;}

  /**
   * See vtkCell3D API for description of these methods.
   */
  void GetEdgePoints(int vtkNotUsed(edgeId), int* &vtkNotUsed(pts)) VTK_OVERRIDE {}
  void GetFacePoints(int vtkNotUsed(faceId), int* &vtkNotUsed(pts)) VTK_OVERRIDE {}
  double *GetParametricCoords() VTK_OVERRIDE;

  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_CONVEX_POINT_SET;}

  /**
   * This cell requires that it be initialized prior to access.
   */
  int RequiresInitialization() VTK_OVERRIDE {return 1;}
  void Initialize() VTK_OVERRIDE;

  //@{
  /**
   * A convex point set has no explicit cell edge or faces; however
   * implicitly (after triangulation) it does. Currently the method
   * GetNumberOfEdges() always returns 0 while the GetNumberOfFaces() returns
   * the number of boundary triangles of the triangulation of the convex
   * point set. The method GetNumberOfFaces() triggers a triangulation of the
   * convex point set; repeated calls to GetFace() then return the boundary
   * faces. (Note: GetNumberOfEdges() currently returns 0 because it is a
   * rarely used method and hard to implement. It can be changed in the future.
   */
  int GetNumberOfEdges() VTK_OVERRIDE {return 0;}
  vtkCell *GetEdge(int) VTK_OVERRIDE {return NULL;}
  int GetNumberOfFaces() VTK_OVERRIDE;
  vtkCell *GetFace(int faceId) VTK_OVERRIDE;
  //@}

  /**
   * Satisfy the vtkCell API. This method contours by triangulating the
   * cell and then contouring the resulting tetrahedra.
   */
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;

  /**
   * Satisfy the vtkCell API. This method contours by triangulating the
   * cell and then adding clip-edge intersection points into the
   * triangulation; extracting the clipped region.
   */
  void Clip(double value, vtkDataArray *cellScalars,
                    vtkIncrementalPointLocator *locator, vtkCellArray *connectivity,
                    vtkPointData *inPd, vtkPointData *outPd,
                    vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
                    int insideOut) VTK_OVERRIDE;

  /**
   * Satisfy the vtkCell API. This method determines the subId, pcoords,
   * and weights by triangulating the convex point set, and then
   * determining which tetrahedron the point lies in.
   */
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;

  /**
   * The inverse of EvaluatePosition.
   */
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;

  /**
   * Triangulates the cells and then intersects them to determine the
   * intersection point.
   */
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;

  /**
   * Triangulate using methods of vtkOrderedTriangulator.
   */
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;

  /**
   * Computes derivatives by triangulating and from subId and pcoords,
   * evaluating derivatives on the resulting tetrahedron.
   */
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;

  /**
   * Returns the set of points forming a face of the triangulation of these
   * points that are on the boundary of the cell that are closest
   * parametrically to the point specified.
   */
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;

  /**
   * Return the center of the cell in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * A convex point set is triangulated prior to any operations on it so
   * it is not a primary cell, it is a composite cell.
   */
  int IsPrimaryCell() VTK_OVERRIDE {return 0;}

  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double *sf) VTK_OVERRIDE;
  void InterpolateDerivs(double pcoords[3], double *derivs) VTK_OVERRIDE;
  //@}

protected:
  vtkConvexPointSet();
  ~vtkConvexPointSet() VTK_OVERRIDE;

  vtkTetra       *Tetra;
  vtkIdList      *TetraIds;
  vtkPoints      *TetraPoints;
  vtkDoubleArray *TetraScalars;

  vtkCellArray   *BoundaryTris;
  vtkTriangle    *Triangle;
  vtkDoubleArray *ParametricCoords;

private:
  vtkConvexPointSet(const vtkConvexPointSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkConvexPointSet&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
inline int vtkConvexPointSet::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.5;
  return 0;
}

#endif



