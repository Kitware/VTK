/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleStrip.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTriangleStrip
 * @brief   a cell that represents a triangle strip
 *
 * vtkTriangleStrip is a concrete implementation of vtkCell to represent a 2D
 * triangle strip. A triangle strip is a compact representation of triangles
 * connected edge to edge in strip fashion. The connectivity of a triangle
 * strip is three points defining an initial triangle, then for each
 * additional triangle, a single point that, combined with the previous two
 * points, defines the next triangle.
*/

#ifndef vtkTriangleStrip_h
#define vtkTriangleStrip_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

class vtkLine;
class vtkTriangle;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkTriangleStrip : public vtkCell
{
public:
  static vtkTriangleStrip *New();
  vtkTypeMacro(vtkTriangleStrip,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() VTK_OVERRIDE {return VTK_TRIANGLE_STRIP;};
  int GetCellDimension() VTK_OVERRIDE {return 2;};
  int GetNumberOfEdges() VTK_OVERRIDE {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() VTK_OVERRIDE {return 0;};
  vtkCell *GetEdge(int edgeId) VTK_OVERRIDE;
  vtkCell *GetFace(int vtkNotUsed(faceId)) VTK_OVERRIDE {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts) VTK_OVERRIDE;
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd) VTK_OVERRIDE;
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut) VTK_OVERRIDE;
  //@}

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
  int IsPrimaryCell() VTK_OVERRIDE {return 0;}

  /**
   * Return the center of the point cloud in parametric coordinates.
   */
  int GetParametricCenter(double pcoords[3]) VTK_OVERRIDE;

  /**
   * Given a triangle strip, decompose it into a list of (triangle)
   * polygons. The polygons are appended to the end of the list of triangles.
   */
  static void DecomposeStrip(int npts, vtkIdType *pts, vtkCellArray *tris);

protected:
  vtkTriangleStrip();
  ~vtkTriangleStrip() VTK_OVERRIDE;

  vtkLine *Line;
  vtkTriangle *Triangle;

private:
  vtkTriangleStrip(const vtkTriangleStrip&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTriangleStrip&) VTK_DELETE_FUNCTION;
};

#endif


