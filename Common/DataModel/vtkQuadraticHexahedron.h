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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override {return VTK_QUADRATIC_HEXAHEDRON;}
  int GetCellDimension() override {return 3;}
  int GetNumberOfEdges() override {return 12;}
  int GetNumberOfFaces() override {return 6;}
  vtkCell *GetEdge(int)  override;
  vtkCell *GetFace(int)  override;
  //@}

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
   * Clip this quadratic hexahedron using scalar value provided. Like
   * contouring, except that it cuts the hex to produce linear
   * tetrahedron.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *tetras,
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
   * @deprecated Replaced by vtkQuadraticHexahedron::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[20]);
  /**
   * @deprecated Replaced by vtkQuadraticHexahedron::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[60]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[20]) override
  {
    vtkQuadraticHexahedron::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[60]) override
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
  void JacobianInverse(const double pcoords[3], double **inverse, double derivs[60]);

protected:
  vtkQuadraticHexahedron();
  ~vtkQuadraticHexahedron() override;

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
  vtkQuadraticHexahedron(const vtkQuadraticHexahedron&) = delete;
  void operator=(const vtkQuadraticHexahedron&) = delete;
};

#endif


