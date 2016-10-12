/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticTriangle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuadraticTriangle
 * @brief   cell represents a parabolic, isoparametric triangle
 *
 * vtkQuadraticTriangle is a concrete implementation of vtkNonLinearCell to
 * represent a two-dimensional, 6-node, isoparametric parabolic triangle. The
 * interpolation is the standard finite element, quadratic isoparametric
 * shape function. The cell includes three mid-edge nodes besides the three
 * triangle vertices. The ordering of the three points defining the cell is
 * point ids (0-2,3-5) where id #3 is the midedge node between points
 * (0,1); id #4 is the midedge node between points (1,2); and id #5 is the
 * midedge node between points (2,0).
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTetra vtkQuadraticPyramid
 * vtkQuadraticQuad vtkQuadraticHexahedron vtkQuadraticWedge
*/

#ifndef vtkQuadraticTriangle_h
#define vtkQuadraticTriangle_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkTriangle;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticTriangle : public vtkNonLinearCell
{
public:
  static vtkQuadraticTriangle *New();
  vtkTypeMacro(vtkQuadraticTriangle,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_QUADRATIC_TRIANGLE;};
  int GetCellDimension() VTK_OVERRIDE {return 2;}
  int GetNumberOfEdges() VTK_OVERRIDE {return 3;}
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;}
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int) VTK_OVERRIDE {return 0;}
  //@}

  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;

  /**
   * Clip this quadratic triangle using scalar value provided. Like
   * contouring, except that it cuts the triangle to produce linear
   * triangles.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;


  /**
   * Return the center of the quadratic triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned.
   */
  double GetParametricDistance(double pcoords[3]) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkQuadraticTriangle::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[6]);
  /**
   * @deprecated Replaced by vtkQuadraticTriangle::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[12]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[6]) VTK_OVERRIDE
  {
    vtkQuadraticTriangle::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[12]) VTK_OVERRIDE
  {
    vtkQuadraticTriangle::InterpolationDerivs(pcoords,derivs);
  }
  //@}

protected:
  vtkQuadraticTriangle();
  ~vtkQuadraticTriangle() VTK_OVERRIDE;

  vtkQuadraticEdge *Edge;
  vtkTriangle      *Face;
  vtkDoubleArray    *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticTriangle(const vtkQuadraticTriangle&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQuadraticTriangle&) VTK_DELETE_FUNCTION;
};
//----------------------------------------------------------------------------
inline int vtkQuadraticTriangle::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1./3;
  pcoords[2] = 0.0;
  return 0;
}


#endif


