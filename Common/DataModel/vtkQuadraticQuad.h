/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuadraticQuad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuadraticQuad
 * @brief   cell represents a parabolic, 8-node isoparametric quad
 *
 * vtkQuadraticQuad is a concrete implementation of vtkNonLinearCell to
 * represent a two-dimensional, 8-node isoparametric parabolic quadrilateral
 * element. The interpolation is the standard finite element, quadratic
 * isoparametric shape function. The cell includes a mid-edge node for each
 * of the four edges of the cell. The ordering of the eight points defining
 * the cell are point ids (0-3,4-7) where ids 0-3 define the four corner
 * vertices of the quad; ids 4-7 define the midedge nodes (0,1), (1,2),
 * (2,3), (3,0).
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticHexahedron vtkQuadraticWedge vtkQuadraticPyramid
*/

#ifndef vtkQuadraticQuad_h
#define vtkQuadraticQuad_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkQuad;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkQuadraticQuad : public vtkNonLinearCell
{
public:
  static vtkQuadraticQuad *New();
  vtkTypeMacro(vtkQuadraticQuad,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_QUADRATIC_QUAD;};
  int GetCellDimension() VTK_OVERRIDE {return 2;}
  int GetNumberOfEdges() VTK_OVERRIDE {return 4;}
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;}
  vtkCell *GetEdge(int) VTK_OVERRIDE;
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
   * Clip this quadratic quad using scalar value provided. Like contouring,
   * except that it cuts the quad to produce linear triangles.
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
   * Return the center of the pyramid in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkQuadraticQuad::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[8]);
  /**
   * @deprecated Replaced by vtkQuadraticQuad::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[16]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[8]) VTK_OVERRIDE
  {
    vtkQuadraticQuad::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[16]) VTK_OVERRIDE
  {
    vtkQuadraticQuad::InterpolationDerivs(pcoords,derivs);
  }
  //@}

protected:
  vtkQuadraticQuad();
  ~vtkQuadraticQuad() VTK_OVERRIDE;

  vtkQuadraticEdge *Edge;
  vtkQuad          *Quad;
  vtkPointData     *PointData;
  vtkDoubleArray   *Scalars;

  // In order to achieve some functionality we introduce a fake center point
  // which require to have some extra functionalities compare to other non-linar
  // cells
  vtkCellData      *CellData;
  vtkDoubleArray   *CellScalars;
  void Subdivide(double *weights);
  void InterpolateAttributes(vtkPointData *inPd, vtkCellData *inCd, vtkIdType cellId,
    vtkDataArray *cellScalars);

private:
  vtkQuadraticQuad(const vtkQuadraticQuad&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQuadraticQuad&) VTK_DELETE_FUNCTION;
};
//----------------------------------------------------------------------------
inline int vtkQuadraticQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.;
  return 0;
}

#endif


