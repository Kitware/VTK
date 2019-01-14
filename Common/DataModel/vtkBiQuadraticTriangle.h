/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBiQuadraticTriangle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkBiQuadraticTriangle
 * @brief   cell represents a parabolic, isoparametric triangle
 *
 * vtkBiQuadraticTriangle is a concrete implementation of vtkNonLinearCell to
 * represent a two-dimensional, 7-node, isoparametric parabolic triangle. The
 * interpolation is the standard finite element, bi-quadratic isoparametric
 * shape function. The cell includes three mid-edge nodes besides the three
 * triangle vertices and a center node. The ordering of the three points defining the cell is
 * point ids (0-2,3-6) where id #3 is the midedge node between points
 * (0,1); id #4 is the midedge node between points (1,2); and id #5 is the
 * midedge node between points (2,0). id #6 is the center node of the cell.
 *
 * @sa
 * vtkTriangle vtkQuadraticTriangle
 * vtkBiQuadraticQuad vtkBiQuadraticQuadraticWedge vtkBiQuadraticQuadraticHexahedron
 * @par Thanks:
 * <verbatim>
 * This file has been developed by Oxalya - www.oxalya.com
 * Copyright (c) EDF - www.edf.fr
 * </verbatim>
*/

#ifndef vtkBiQuadraticTriangle_h
#define vtkBiQuadraticTriangle_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkNonLinearCell.h"

class vtkQuadraticEdge;
class vtkTriangle;
class vtkDoubleArray;

class VTKCOMMONDATAMODEL_EXPORT vtkBiQuadraticTriangle : public vtkNonLinearCell
{
public:
  static vtkBiQuadraticTriangle *New();
  vtkTypeMacro(vtkBiQuadraticTriangle,vtkNonLinearCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Implement the vtkCell API. See the vtkCell API for descriptions
   * of these methods.
   */
  int GetCellType() override {return VTK_BIQUADRATIC_TRIANGLE;};
  int GetCellDimension() override {return 2;}
  int GetNumberOfEdges() override {return 3;}
  int GetNumberOfFaces() override {return 0;}
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int) override {return nullptr;}
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
   * Clip this quadratic triangle using scalar value provided. Like
   * contouring, except that it cuts the triangle to produce linear
   * triangles.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
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
   * Return the center of the quadratic triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * Return the distance of the parametric coordinate provided to the
   * cell. If inside the cell, a distance of zero is returned.
   */
  double GetParametricDistance(const double pcoords[3]) override;

  /**
   * @deprecated Replaced by vtkBiQuadraticTriangle::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double weights[7]);
  /**
   * @deprecated Replaced by vtkBiQuadraticTriangle::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[14]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double weights[7]) override
  {
    vtkBiQuadraticTriangle::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[14]) override
  {
    vtkBiQuadraticTriangle::InterpolationDerivs(pcoords,derivs);
  }
  //@}

protected:
  vtkBiQuadraticTriangle();
  ~vtkBiQuadraticTriangle() override;

  vtkQuadraticEdge *Edge;
  vtkTriangle      *Face;
  vtkDoubleArray   *Scalars; //used to avoid New/Delete in contouring/clipping

private:
  vtkBiQuadraticTriangle(const vtkBiQuadraticTriangle&) = delete;
  void operator=(const vtkBiQuadraticTriangle&) = delete;
};
//----------------------------------------------------------------------------
inline int vtkBiQuadraticTriangle::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1./3;
  pcoords[2] = 0.0;
  return 0;
}


#endif


