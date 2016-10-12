/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertex.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkVertex
 * @brief   a cell that represents a 3D point
 *
 * vtkVertex is a concrete implementation of vtkCell to represent a 3D point.
*/

#ifndef vtkVertex_h
#define vtkVertex_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkVertex : public vtkCell
{
public:
  static vtkVertex *New();
  vtkTypeMacro(vtkVertex,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Make a new vtkVertex object with the same information as this object.
   */

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_VERTEX;};
  int GetCellDimension() VTK_OVERRIDE {return 0;};
  int GetNumberOfEdges() VTK_OVERRIDE {return 0;};
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;};
  vtkCell *GetEdge(int)  VTK_OVERRIDE {return 0;};
  vtkCell *GetFace(int)  VTK_OVERRIDE {return 0;};
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *pts,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights) VTK_OVERRIDE;
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights) VTK_OVERRIDE;
  double *GetParametricCoords() VTK_OVERRIDE;
  //@}

  /**
   * Given parametric coordinates of a point, return the closest cell
   * boundary, and whether the point is inside or outside of the cell. The
   * cell boundary is defined by a list of points (pts) that specify a vertex
   * (1D cell).  If the return value of the method is != 0, then the point is
   * inside the cell.
   */
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;

  /**
   * Generate contouring primitives. The scalar list cellScalars are
   * scalar values at each cell point. The point locator is essentially a
   * points list that merges points as they are inserted (i.e., prevents
   * duplicates).
   */
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts1,
               vtkCellArray *lines, vtkCellArray *verts2,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;

  /**
   * Return the center of the triangle in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Intersect with a ray. Return parametric coordinates (both line and cell)
   * and global intersection coordinates, given ray definition and tolerance.
   * The method returns non-zero value if intersection occurs.
   */
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId) VTK_OVERRIDE;

  /**
   * Triangulate the vertex. This method fills pts and ptIds with information
   * from the only point in the vertex.
   */
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts) VTK_OVERRIDE;

  /**
   * Get the derivative of the vertex. Returns (0.0, 0.0, 0.0) for all
   * dimensions.
   */
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs) VTK_OVERRIDE;

  /**
   * @deprecated Replaced by vtkVertex::InterpolateFunctions as of VTK 5.2
   */
  static void InterpolationFunctions(double pcoords[3], double weights[1]);
  /**
   * @deprecated Replaced by vtkVertex::InterpolateDerivs as of VTK 5.2
   */
  static void InterpolationDerivs(double pcoords[3], double derivs[3]);
  //@{
  /**
   * Compute the interpolation functions/derivatives
   * (aka shape functions/derivatives)
   */
  void InterpolateFunctions(double pcoords[3], double weights[1]) VTK_OVERRIDE
  {
    vtkVertex::InterpolationFunctions(pcoords,weights);
  }
  void InterpolateDerivs(double pcoords[3], double derivs[3]) VTK_OVERRIDE
  {
    vtkVertex::InterpolationDerivs(pcoords,derivs);
  }
  //@}

protected:
  vtkVertex();
  ~vtkVertex() VTK_OVERRIDE {}

private:
  vtkVertex(const vtkVertex&) VTK_DELETE_FUNCTION;
  void operator=(const vtkVertex&) VTK_DELETE_FUNCTION;
};

//----------------------------------------------------------------------------
inline int vtkVertex::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = pcoords[2] = 0.0;
  return 0;
}

#endif


