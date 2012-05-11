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
// .NAME vtkTriangleStrip - a cell that represents a triangle strip
// .SECTION Description
// vtkTriangleStrip is a concrete implementation of vtkCell to represent a 2D
// triangle strip. A triangle strip is a compact representation of triangles
// connected edge to edge in strip fashion. The connectivity of a triangle
// strip is three points defining an initial triangle, then for each
// additional triangle, a single point that, combined with the previous two
// points, defines the next triangle.

#ifndef __vtkTriangleStrip_h
#define __vtkTriangleStrip_h

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_TRIANGLE_STRIP;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int vtkNotUsed(faceId)) {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  int IsPrimaryCell() {return 0;}

  // Description:
  // Return the center of the point cloud in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Given a triangle strip, decompose it into a list of (triangle)
  // polygons. The polygons are appended to the end of the list of triangles.
  static void DecomposeStrip(int npts, vtkIdType *pts, vtkCellArray *tris);

  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double *weights);
  virtual void InterpolateDerivs(double pcoords[3], double *derivs);

protected:
  vtkTriangleStrip();
  ~vtkTriangleStrip();

  vtkLine *Line;
  vtkTriangle *Triangle;

private:
  vtkTriangleStrip(const vtkTriangleStrip&);  // Not implemented.
  void operator=(const vtkTriangleStrip&);  // Not implemented.
};

#endif


