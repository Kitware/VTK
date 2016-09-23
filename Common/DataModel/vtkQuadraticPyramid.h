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
/**
 * @class   vtkQuadraticPyramid
 * @brief   cell represents a parabolic, 13-node isoparametric pyramid
 *
 * vtkQuadraticPyramid is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 13-node isoparametric parabolic
 * pyramid. The interpolation is the standard finite element, quadratic
 * isoparametric shape function. The cell includes a mid-edge node. The
 * ordering of the thirteen points defining the cell is point ids (0-4,5-12)
 * where point ids 0-4 are the five corner vertices of the pyramid; followed
 * by eight midedge nodes (5-12). Note that these midedge nodes correspond lie
 * on the edges defined by (0,1), (1,2), (2,3), (3,0), (0,4), (1,4), (2,4),
 * (3,4).
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticHexahedron vtkQuadraticQuad vtkQuadraticWedge
 *
 * @par Thanks:
 * The shape functions and derivatives could be implemented thanks to
 * the report Pyramid Solid Elements Linear and Quadratic Iso-P Models
 * From Center For Aerospace Structures
*/

#ifndef vtkQuadraticPyramid_h
#define vtkQuadraticPyramid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkQuadraticQuad;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkPyramid;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticPyramid : public vtkNonLinearCell
{
public:
  static vtkQuadraticPyramid *New();
  vtkTypeMacro(vtkQuadraticPyramid,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_QUADRATIC_PYRAMID;};
  int GetCellDimension() VTK_OVERRIDE {return 3;}
  int GetNumberOfEdges() VTK_OVERRIDE {return 8;}
  int GetNumberOfFaces() VTK_OVERRIDE {return 5;}
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int faceId) VTK_OVERRIDE;
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
            vtkIncrementalPointLocator *locator, vtkCellArray *tets,
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
   * Return the center of the quadratic pyramid in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkQuadraticPyramid::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[13]);
  /**
   * @deprecated Replaced by vtkQuadraticPyramid::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[39]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[13]) VTK_OVERRIDE
  {
    vtkQuadraticPyramid::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[39]) VTK_OVERRIDE
  {
    vtkQuadraticPyramid::InterpolationDerivs(pcoords,derivs);
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
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[39]);

protected:
  vtkQuadraticPyramid();
  ~vtkQuadraticPyramid() VTK_OVERRIDE;

  vtkQuadraticEdge *Edge;
  vtkQuadraticTriangle *TriangleFace;
  vtkQuadraticQuad *Face;
  vtkTetra         *Tetra;
  vtkPyramid       *Pyramid;
  vtkPointData     *PointData;
  vtkCellData      *CellData;
  vtkDoubleArray   *CellScalars;
  vtkDoubleArray   *Scalars; //used to avoid New/Delete in contouring/clipping

  void Subdivide(vtkPointData *inPd, vtkCellData *inCd, vtkIdType cellId,
    vtkDataArray *cellScalars);

private:
  vtkQuadraticPyramid(const vtkQuadraticPyramid&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQuadraticPyramid&) VTK_DELETE_FUNCTION;
};
//----------------------------------------------------------------------------
// Return the center of the quadratic pyramid in parametric coordinates.
//
inline int vtkQuadraticPyramid::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 6.0/13.0;
  pcoords[2] = 3.0/13.0;
  return 0;
}


#endif
