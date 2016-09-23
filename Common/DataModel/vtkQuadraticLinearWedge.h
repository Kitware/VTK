/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticLinearWedge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuadraticLinearWedge
 * @brief   cell represents a, 12-node isoparametric wedge
 *
 * vtkQuadraticLinearWedge is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 12-node isoparametric linear quadratic
 * wedge. The interpolation is the standard finite element, quadratic
 * isoparametric shape function in xy - layer and the linear functions in z - direction.
 * The cell includes mid-edge node in the triangle edges. The
 * ordering of the 12 points defining the cell is point ids (0-5,6-12)
 * where point ids 0-5 are the six corner vertices of the wedge; followed by
 * six midedge nodes (6-12). Note that these midedge nodes correspond lie
 * on the edges defined by (0,1), (1,2), (2,0), (3,4), (4,5), (5,3).
 * The Edges (0,3), (1,4), (2,5) dont have midedge nodes.
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticHexahedron vtkQuadraticQuad vtkQuadraticPyramid
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert  who developed this class and
 * integrated it into VTK 5.0.
*/

#ifndef vtkQuadraticLinearWedge_h
#define vtkQuadraticLinearWedge_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkLine;
class vtkQuadraticLinearQuad;
class vtkQuadraticTriangle;
class vtkWedge;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticLinearWedge : public vtkNonLinearCell
{
public:
  static vtkQuadraticLinearWedge *New ();
  vtkTypeMacro(vtkQuadraticLinearWedge,vtkNonLinearCell);
  void PrintSelf (ostream & os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() VTK_OVERRIDE { return VTK_QUADRATIC_LINEAR_WEDGE; }
  int GetCellDimension() VTK_OVERRIDE { return 3; }
  int GetNumberOfEdges() VTK_OVERRIDE { return 9; }
  int GetNumberOfFaces() VTK_OVERRIDE { return 5; }
  vtkCell *GetEdge (int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace (int faceId) VTK_OVERRIDE;
  //@}

  int CellBoundary (int subId, double pcoords[3], vtkIdList * pts) VTK_OVERRIDE;

  //@{
  /**
   * The quadratic linear wege is splitted into 4 linear wedges,
   * each of them is contoured by a provided scalar value
   */
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
  void Derivatives (int subId, double pcoords[3], double *values,
                    int dim, double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords () VTK_OVERRIDE;
  //@}

  /**
   * Clip this quadratic linear wedge using scalar value provided. Like
   * contouring, except that it cuts the hex to produce linear
   * tetrahedron.
   */
  void Clip (double value, vtkDataArray * cellScalars,
       vtkIncrementalPointLocator * locator, vtkCellArray * tetras,
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
   * Return the center of the quadratic linear wedge in parametric coordinates.
   */
  int GetParametricCenter (double pcoords[3]) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkQuadraticLinearWedge::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions (double pcoords[3], double weights[15]);
  /**
   * @deprecated Replaced by vtkQuadraticLinearWedge::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs (double pcoords[3], double derivs[45]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions (double pcoords[3], double weights[15]) VTK_OVERRIDE
  {
    vtkQuadraticLinearWedge::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs (double pcoords[3], double derivs[45]) VTK_OVERRIDE
  {
    vtkQuadraticLinearWedge::InterpolationDerivs(pcoords,derivs);
  }
  //@}
  //@{
  /**
   * Return the ids of the vertices defining edge/face (`edgeId`/`faceId').
   * Ids are related to the cell, not to the dataset.
   */
  static int *GetEdgeArray(int edgeId);
  static int *GetFaceArray(int faceId);
  //@}

  /**
   * Given parametric coordinates compute inverse Jacobian transformation
   * matrix. Returns 9 elements of 3x3 inverse Jacobian plus interpolation
   * function derivatives.
   */
  void JacobianInverse (double pcoords[3], double **inverse, double derivs[45]);

protected:
  vtkQuadraticLinearWedge ();
  ~vtkQuadraticLinearWedge () VTK_OVERRIDE;

  vtkQuadraticEdge *QuadEdge;
  vtkLine *Edge;
  vtkQuadraticTriangle *TriangleFace;
  vtkQuadraticLinearQuad *Face;
  vtkWedge *Wedge;
  vtkDoubleArray *Scalars;  //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticLinearWedge (const vtkQuadraticLinearWedge &) VTK_DELETE_FUNCTION;
  void operator = (const vtkQuadraticLinearWedge &) VTK_DELETE_FUNCTION;
};
//----------------------------------------------------------------------------
// Return the center of the quadratic wedge in parametric coordinates.
inline int vtkQuadraticLinearWedge::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1./3;
  pcoords[2] = 0.5;
  return 0;
}


#endif
