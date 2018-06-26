/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticEdge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuadraticEdge
 * @brief   cell represents a parabolic, isoparametric edge
 *
 * vtkQuadraticEdge is a concrete implementation of vtkNonLinearCell to
 * represent a one-dimensional, 3-nodes, isoparametric parabolic line. The
 * interpolation is the standard finite element, quadratic isoparametric
 * shape function. The cell includes a mid-edge node. The ordering of the
 * three points defining the cell is point ids (0,1,2) where id #2 is the
 * midedge node.
 *
 * @sa
 * vtkQuadraticTriangle vtkQuadraticTetra vtkQuadraticWedge
 * vtkQuadraticQuad vtkQuadraticHexahedron vtkQuadraticPyramid
*/

#ifndef vtkQuadraticEdge_h
#define vtkQuadraticEdge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkLine;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticEdge : public vtkNonLinearCell
{
public:
  static vtkQuadraticEdge *New();
  vtkTypeMacro(vtkQuadraticEdge,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override {return VTK_QUADRATIC_EDGE;};
  int GetCellDimension() override {return 1;}
  int GetNumberOfEdges() override {return 0;}
  int GetNumberOfFaces() override {return 0;}
  vtkCell *GetEdge(int) override {return nullptr;}
  vtkCell *GetFace(int) override {return nullptr;}

  int CellBoundary(int subId, const double pcoords[3], vtkIdList *pts) override;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) override;
  int EvaluatePosition(const double x[3], double closestPoint[3],
                       int& subId, double pcoords[3],
                       double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3],
                        double *weights) override;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) override;
  void Derivatives(int subId, const double pcoords[3], const double *values,
                   int dim, double *derivs) override;
  double *GetParametricCoords() override;

  /**
   * Clip this edge using scalar value provided. Like contouring, except
   * that it cuts the edge to produce linear line segments.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *lines,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) override;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) override;

  /**
   * Return the center of the quadratic tetra in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * @deprecated Replaced by vtkQuadraticEdge::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[3]);
  /**
   * @deprecated Replaced by vtkQuadraticEdge::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[3]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[3]) override
  {
    vtkQuadraticEdge::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[3]) override
  {
    vtkQuadraticEdge::InterpolationDerivs(pcoords,derivs);
  }
  //@}

protected:
  vtkQuadraticEdge();
  ~vtkQuadraticEdge() override;

  vtkLine *Line;
  vtkDoubleArray *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticEdge(const vtkQuadraticEdge&) = delete;
  void operator=(const vtkQuadraticEdge&) = delete;
};
//----------------------------------------------------------------------------
inline int vtkQuadraticEdge::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = 0.5;
  pcoords[1] = pcoords[2] = 0.;
  return 0;
}

#endif


