/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticTetra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuadraticTetra
 * @brief   cell represents a parabolic, 10-node isoparametric tetrahedron
 *
 * vtkQuadraticTetra is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 10-node, isoparametric parabolic
 * tetrahedron. The interpolation is the standard finite element, quadratic
 * isoparametric shape function. The cell includes a mid-edge node on each of
 * the size edges of the tetrahedron. The ordering of the ten points defining
 * the cell is point ids (0-3,4-9) where ids 0-3 are the four tetra
 * vertices; and point ids 4-9 are the midedge nodes between (0,1), (1,2),
 * (2,0), (0,3), (1,3), and (2,3).
 *
 * Note that this class uses an internal linear tesselation for some internal operations
 * (e.g., clipping and contouring). This means that some artifacts may appear trying to
 * represent a non-linear interpolation function with linear tets.
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticWedge
 * vtkQuadraticQuad vtkQuadraticHexahedron vtkQuadraticPyramid
*/

#ifndef vtkQuadraticTetra_h
#define vtkQuadraticTetra_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkQuadraticTriangle;
class vtkTetra;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticTetra : public vtkNonLinearCell
{
public:
  static vtkQuadraticTetra *New();
  vtkTypeMacro(vtkQuadraticTetra,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_QUADRATIC_TETRA;}
  int GetCellDimension() VTK_OVERRIDE {return 3;}
  int GetNumberOfEdges() VTK_OVERRIDE {return 6;}
  int GetNumberOfFaces() VTK_OVERRIDE {return 4;}
  vtkCell *GetEdge(int) VTK_OVERRIDE;
  vtkCell *GetFace(int) VTK_OVERRIDE;
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
   * Clip this edge using scalar value provided. Like contouring, except
   * that it cuts the tetra to produce new tetras.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *tetras,
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
   * Return the center of the quadratic tetra in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned.
   */
  double GetParametricDistance(double pcoords[3]) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkQuadraticTetra::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[10]);
  /**
   * @deprecated Replaced by vtkQuadraticTetra::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[30]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[10]) VTK_OVERRIDE
  {
    vtkQuadraticTetra::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[30]) VTK_OVERRIDE
  {
    vtkQuadraticTetra::InterpolationDerivs(pcoords,derivs);
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
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[30]);

protected:
  vtkQuadraticTetra();
  ~vtkQuadraticTetra() VTK_OVERRIDE;

  vtkQuadraticEdge *Edge;
  vtkQuadraticTriangle *Face;
  vtkTetra *Tetra;
  vtkDoubleArray *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkQuadraticTetra(const vtkQuadraticTetra&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQuadraticTetra&) VTK_DELETE_FUNCTION;
};

#endif


