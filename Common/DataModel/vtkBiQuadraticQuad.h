/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiQuadraticQuad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBiQuadraticQuad
 * @brief   cell represents a parabolic, 9-node
 * isoparametric quad
 *
 * vtkQuadraticQuad is a concrete implementation of vtkNonLinearCell to
 * represent a two-dimensional, 9-node isoparametric parabolic quadrilateral
 * element with a Centerpoint. The interpolation is the standard finite
 * element, quadratic isoparametric shape function. The cell includes a
 * mid-edge node for each of the four edges of the cell and a center node at
 * the surface. The ordering of the eight points defining the cell are point
 * ids (0-3,4-8) where ids 0-3 define the four corner vertices of the quad;
 * ids 4-7 define the midedge nodes (0,1), (1,2), (2,3), (3,0) and 8 define
 * the face center node.
 *
 * @sa
 * vtkQuadraticEdge vtkQuadraticTriangle vtkQuadraticTetra
 * vtkQuadraticHexahedron vtkQuadraticWedge vtkQuadraticPyramid
 * vtkQuadraticQuad
 *
 * @par Thanks:
 * Thanks to Soeren Gebbert who developed this class and
 * integrated it into VTK 5.0.
*/

#ifndef vtkBiQuadraticQuad_h
#define vtkBiQuadraticQuad_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkQuad;
class vtkTriangle;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkBiQuadraticQuad : public vtkNonLinearCell
{
public:
  static vtkBiQuadraticQuad *New ();
  vtkTypeMacro(vtkBiQuadraticQuad,vtkNonLinearCell);
  void PrintSelf (ostream & os, vtkIndent indent) override;

  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override { return VTK_BIQUADRATIC_QUAD; }
  int GetCellDimension() override { return 2; }
  int GetNumberOfEdges() override { return 4; }
  int GetNumberOfFaces() override { return 0; }
  vtkCell *GetEdge (int) override;
  vtkCell *GetFace (int) override { return nullptr; }

  int CellBoundary(int subId, const double pcoords[3], vtkIdList * pts) override;
  int EvaluatePosition(const double x[3], double *closestPoint,
                        int &subId, double pcoords[3],
                        double &dist2, double *weights) override;
  void EvaluateLocation(int &subId, const double pcoords[3], double x[3],
                         double *weights) override;
  int Triangulate (int index, vtkIdList * ptIds, vtkPoints * pts) override;
  void Derivatives(int subId, const double pcoords[3], const double *values,
                    int dim, double *derivs) override;
  double *GetParametricCoords() override;

  void Contour (double value, vtkDataArray * cellScalars,
                vtkIncrementalPointLocator * locator, vtkCellArray * verts,
                vtkCellArray * lines, vtkCellArray * polys,
                vtkPointData * inPd, vtkPointData * outPd, vtkCellData * inCd,
                vtkIdType cellId, vtkCellData * outCd) override;

  /**
   * Clip this biquadratic quad using scalar value provided. Like contouring,
   * except that it cuts the twi quads to produce linear triangles.
   */
  void Clip (double value, vtkDataArray * cellScalars,
             vtkIncrementalPointLocator * locator, vtkCellArray * polys,
             vtkPointData * inPd, vtkPointData * outPd,
             vtkCellData * inCd, vtkIdType cellId, vtkCellData * outCd,
             int insideOut) override;

  /**
   * Line-edge intersection. Intersection has to occur within [0,1] parametric
   * coordinates and with specified tolerance.
   */
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double &t,
                         double x[3], double pcoords[3],
                         int &subId) override;

  /**
   * Return the center of the pyramid in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  void InterpolateFunctions(const double pcoords[3], double weights[9]) override
  {
    vtkBiQuadraticQuad::InterpolationFunctionsPrivate(pcoords,weights);
  }
  void InterpolateDerivs (const double pcoords[3], double derivs[18]) override
  {
    vtkBiQuadraticQuad::InterpolationDerivsPrivate(pcoords,derivs);
  }
  //@}

protected:
  vtkBiQuadraticQuad();
  ~vtkBiQuadraticQuad() override;

  vtkQuadraticEdge *Edge;
  vtkQuad          *Quad;
  vtkTriangle      *Triangle;
  vtkDoubleArray   *Scalars;

private:
  vtkBiQuadraticQuad(const vtkBiQuadraticQuad&) = delete;
  void operator=(const vtkBiQuadraticQuad&) = delete;

  static void InterpolationFunctionsPrivate (const double pcoords[3], double weights[9]);
  static void InterpolationDerivsPrivate (const double pcoords[3], double derivs[18]);
};
//----------------------------------------------------------------------------
inline int vtkBiQuadraticQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.;
  return 0;
}

#endif
