/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkQuad
 * @brief   a cell that represents a 2D quadrilateral
 *
 * vtkQuad is a concrete implementation of vtkCell to represent a 2D
 * quadrilateral. vtkQuad is defined by the four points (0,1,2,3) in
 * counterclockwise order. vtkQuad uses the standard isoparametric
 * interpolation functions for a linear quadrilateral.
*/

#ifndef vtkQuad_h
#define vtkQuad_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class vtkLine;
class vtkTriangle;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkQuad : public vtkCell
{
public:
  static vtkQuad *New();
  vtkTypeMacro(vtkQuad,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override {return VTK_QUAD;};
  int GetCellDimension() override {return 2;};
  int GetNumberOfEdges() override {return 4;};
  int GetNumberOfFaces() override {return 0;};
  vtkCell *GetEdge(int edgeId) override;
  vtkCell *GetFace(int) override {return nullptr;};
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
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) override;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) override;
  void Derivatives(int subId, const double pcoords[3], const double *values,
                   int dim, double *derivs) override;
  double *GetParametricCoords() override;
  //@}

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) override;

  /**
   * Clip this quad using scalar value provided. Like contouring, except
   * that it cuts the quad to produce other quads and/or triangles.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) override;

  /**
   * @deprecated Replaced by vtkQuad::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(const double pcoords[3], double sf[4]);
  /**
   * @deprecated Replaced by vtkQuad::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(const double pcoords[3], double derivs[8]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(const double pcoords[3], double sf[4]) override
  {
    vtkQuad::InterpolationFunctions(pcoords,sf);
  }
  void InterpolateDerivs(const double pcoords[3], double derivs[8]) override
  {
    vtkQuad::InterpolationDerivs(pcoords,derivs);
  }
  //@}

  /**
   * Return the ids of the vertices defining edge (`edgeId`).
   * Ids are related to the cell, not to the dataset.
   */
  int *GetEdgeArray(int edgeId);

protected:
  vtkQuad();
  ~vtkQuad() override;

  vtkLine     *Line;
  vtkTriangle *Triangle;

private:
  vtkQuad(const vtkQuad&) = delete;
  void operator=(const vtkQuad&) = delete;
};
//----------------------------------------------------------------------------
inline int vtkQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.0;
  return 0;
}


#endif


