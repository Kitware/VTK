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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_QUAD;};
  int GetCellDimension() VTK_OVERRIDE {return 2;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 4;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;};
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int) VTK_OVERRIDE {return 0;};
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
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;
  //@}

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Clip this quad using scalar value provided. Like contouring, except
   * that it cuts the quad to produce other quads and/or triangles.
   */
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkQuad::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double sf[4]);
  /**
   * @deprecated Replaced by vtkQuad::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[8]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double sf[4]) VTK_OVERRIDE
  {
    vtkQuad::InterpolationFunctions(pcoords,sf);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[8]) VTK_OVERRIDE
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
  ~vtkQuad() VTK_OVERRIDE;

  vtkLine     *Line;
  vtkTriangle *Triangle;

private:
  vtkQuad(const vtkQuad&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQuad&) VTK_DELETE_FUNCTION;
};
//----------------------------------------------------------------------------
inline int vtkQuad::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 0.5;
  pcoords[2] = 0.0;
  return 0;
}


#endif


