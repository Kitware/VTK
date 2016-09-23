/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticLinearQuad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuadraticLinearQuad
 * @brief   cell represents a quadratic-linear, 6-node isoparametric quad
 *
 * vtkQuadraticQuad is a concrete implementation of vtkNonLinearCell to
 * represent a two-dimensional, 6-node isoparametric quadratic-linear quadrilateral
 * element. The interpolation is the standard finite element, quadratic-linear
 * isoparametric shape function. The cell includes a mid-edge node for two
 * of the four edges. The ordering of the six points defining
 * the cell are point ids (0-3,4-5) where ids 0-3 define the four corner
 * vertices of the quad; ids 4-7 define the midedge nodes (0,1) and (2,3) .
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra vtkQuadraticQuad
 * vtkQuadraticHexahedron vtkQuadraticWedge vtkQuadraticPyramid
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert  who developed this class and
 * integrated it into VTK 5.0.
*/

#ifndef vtkQuadraticLinearQuad_h
#define vtkQuadraticLinearQuad_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkLine;
class vtkQuad;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticLinearQuad : public vtkNonLinearCell
{
public:
  static vtkQuadraticLinearQuad *New();
  vtkTypeMacro(vtkQuadraticLinearQuad, vtkNonLinearCell);
  void PrintSelf(ostream & os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() VTK_OVERRIDE { return VTK_QUADRATIC_LINEAR_QUAD; };
  int GetCellDimension() VTK_OVERRIDE { return 2; }
  int GetNumberOfEdges() VTK_OVERRIDE { return 4; }
  int GetNumberOfFaces() VTK_OVERRIDE { return 0; }
  vtkCell *GetEdge (int) VTK_OVERRIDE;
  vtkCell *GetFace (int) VTK_OVERRIDE { return 0; }
  //@}

  int CellBoundary (int subId, double pcoords[3], vtkIdList * pts) VTK_OVERRIDE;
  void Contour (double value, vtkDataArray * cellScalars,
    vtkIncrementalPointLocator * locator, vtkCellArray * verts,
    vtkCellArray * lines, vtkCellArray * polys,
    vtkPointData * inPd, vtkPointData * outPd, vtkCellData * inCd,
    vtkIdType cellId, vtkCellData * outCd) VTK_OVERRIDE;
  int EvaluatePosition (double x[3], double *closestPoint,
    int &subId, double pcoords[3], double &dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation (int &subId, double pcoords[3], double x[3],
    double *weights) VTK_OVERRIDE;
  int Triangulate (int index, vtkIdList * ptIds, vtkPoints * pts) VTK_OVERRIDE;
  void Derivatives (int subId, double pcoords[3], double *values, int dim,
    double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords () VTK_OVERRIDE;

  /**
   * Clip this quadratic linear quad using scalar value provided. Like
   * contouring, except that it cuts the quad to produce linear triangles.
   */
  void Clip (double value, vtkDataArray * cellScalars,
       vtkIncrementalPointLocator * locator, vtkCellArray * polys,
       vtkPointData * inPd, vtkPointData * outPd,
       vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd,
       int insideOut) VTK_OVERRIDE;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine (double p1[3], double p2[3], double tol, double &t,
    double x[3], double pcoords[3], int &subId) VTK_OVERRIDE;

  /**
   * Return the center of the pyramid in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkQuadraticLinearQuad::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions (double pcoords[3], double weights[6]);
  /**
   * @deprecated Replaced by vtkQuadraticLinearQuad::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs (double pcoords[3], double derivs[12]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions (double pcoords[3], double weights[6]) VTK_OVERRIDE
  {
    vtkQuadraticLinearQuad::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs (double pcoords[3], double derivs[12]) VTK_OVERRIDE
  {
    vtkQuadraticLinearQuad::InterpolationDerivs(pcoords,derivs);
  }
  //@}
  /**
   * Return the ids of the vertices defining edge (`edgeId`).
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId);

protected:
  vtkQuadraticLinearQuad ();
  ~vtkQuadraticLinearQuad () VTK_OVERRIDE;

  vtkQuadraticEdge *Edge;
  vtkLine *LinEdge;
  vtkQuad *Quad;
  vtkDoubleArray *Scalars;

private:
  vtkQuadraticLinearQuad (const vtkQuadraticLinearQuad &) VTK_DELETE_FUNCTION;
  void operator = (const vtkQuadraticLinearQuad &) VTK_DELETE_FUNCTION;
};
//----------------------------------------------------------------------------
inline int vtkQuadraticLinearQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.;
  return 0;
}

#endif
