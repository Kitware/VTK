/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticHexahedron.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuadraticHexahedron
 * @brief   cell represents a parabolic, 20-node isoparametric hexahedron
 *
 * vtkQuadraticHexahedron is a concrete implementation of vtkNonLinearCell to
 * represent a three-dimensional, 20-node isoparametric parabolic
 * hexahedron. The interpolation is the standard finite element, quadratic
 * isoparametric shape function. The cell includes a mid-edge node. The
 * ordering of the twenty points defining the cell is point ids (0-7,8-19)
 * where point ids 0-7 are the eight corner vertices of the cube; followed by
 * twelve midedge nodes (8-19). Note that these midedge nodes correspond lie
 * on the edges defined by (0,1), (1,2), (2,3), (3,0), (4,5), (5,6), (6,7),
 * (7,4), (0,4), (1,5), (2,6), (3,7).
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticQuad vtkQuadraticPyramid vtkQuadraticWedge
*/

#ifndef vtkQuadraticHexahedron_h
#define vtkQuadraticHexahedron_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkQuadraticQuad;
class vtkHexahedron;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticHexahedron : public vtkNonLinearCell
{
public:
  static vtkQuadraticHexahedron *New();
  vtkTypeMacro(vtkQuadraticHexahedron,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_QUADRATIC_HEXAHEDRON;}
  int GetCellDimension() VTK_OVERRIDE {return 3;}
  int GetNumberOfEdges() VTK_OVERRIDE {return 12;}
  int GetNumberOfFaces() VTK_OVERRIDE {return 6;}
  vtkCell *GetEdge(int)  VTK_OVERRIDE;
  vtkCell *GetFace(int)  VTK_OVERRIDE;
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
   * Clip this quadratic hexahedron using scalar value provided. Like
   * contouring, except that it cuts the hex to produce linear
   * tetrahedron.
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
   * @deprecated Replaced by vtkQuadraticHexahedron::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[20]);
  /**
   * @deprecated Replaced by vtkQuadraticHexahedron::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[60]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[20]) VTK_OVERRIDE
  {
    vtkQuadraticHexahedron::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[60]) VTK_OVERRIDE
  {
    vtkQuadraticHexahedron::InterpolationDerivs(pcoords,derivs);
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
  void JacobianInverse(double pcoords[3], double **inverse, double derivs[60]);

protected:
  vtkQuadraticHexahedron();
  ~vtkQuadraticHexahedron() VTK_OVERRIDE;

  vtkQuadraticEdge *Edge;
  vtkQuadraticQuad *Face;
  vtkHexahedron    *Hex;
  vtkPointData     *PointData;
  vtkCellData      *CellData;
  vtkDoubleArray   *CellScalars;
  vtkDoubleArray   *Scalars;

  void Subdivide(vtkPointData *inPd, vtkCellData *inCd, vtkIdType cellId,
    vtkDataArray *cellScalars);

private:
  vtkQuadraticHexahedron(const vtkQuadraticHexahedron&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQuadraticHexahedron&) VTK_DELETE_FUNCTION;
};

#endif


