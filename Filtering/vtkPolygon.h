/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolygon.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolygon - a cell that represents an n-sided polygon
// .SECTION Description
// vtkPolygon is a concrete implementation of vtkCell to represent a 2D
// n-sided polygon. The polygons cannot have any internal holes, and cannot
// self-intersect. Define the polygon with n-points ordered in the counter-
// clockwise direction; do not repeat the last point.

#ifndef __vtkPolygon_h
#define __vtkPolygon_h

#include "vtkCell.h"

class vtkDoubleArray;
class vtkLine;
class vtkPoints;
class vtkQuad;
class vtkTriangle;

class VTK_FILTERING_EXPORT vtkPolygon : public vtkCell
{
public:
  static vtkPolygon *New();
  vtkTypeRevisionMacro(vtkPolygon,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_POLYGON;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return this->GetNumberOfPoints();};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetEdge(int edgeId);
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkPointLocator *locator,vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  void Clip(double value, vtkDataArray *cellScalars,
            vtkPointLocator *locator, vtkCellArray *tris,
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
  // Polygon specific methods.
  static void ComputeNormal(vtkPoints *p, int numPts, vtkIdType *pts,
                            double n[3]);
  static void ComputeNormal(vtkPoints *p, double n[3]);

  // Description:
  // Compute the polygon normal from an array of points. This version assumes
  // that the polygon is convex, and looks for the first valid normal.
  static void ComputeNormal(int numPts, double *pts, double n[3]);

  // Description:
  // Compute interpolation weights using 1/r**2 normalized sum.
  void ComputeWeights(double x[3], double *weights);


  // Description:
  // Create a local s-t coordinate system for a polygon. The point p0 is
  // the origin of the local system, p10 is s-axis vector, and p20 is the
  // t-axis vector. (These are expressed in the modeling coordinate system and
  // are vectors of dimension [3].) The values l20 and l20 are the lengths of
  // the vectors p10 and p20, and n is the polygon normal.
  int ParameterizePolygon(double p0[3], double p10[3], double &l10,
                          double p20[3], double &l20, double n[3]);

  // Description:
  // Determine whether point is inside polygon. Function uses ray-casting
  // to determine if point is inside polygon. Works for arbitrary polygon shape
  // (e.g., non-convex). Returns 0 if point is not in polygon; 1 if it is.
  // Can also return -1 to indicate degenerate polygon.
  static int PointInPolygon(double x[3], int numPts, double *pts,
                            double bounds[6], double n[3]);

  // Description:
  // Triangulate this polygon. The user must provide the vtkIdList outTris.
  // On output, the outTris list contains the ids of the points defining
  // the triangulation. The ids are ordered into groups of three: each
  // three-group defines one triangle.
  int Triangulate(vtkIdList *outTris);

  // Description:
  // Method intersects two polygons. You must supply the number of points and
  // point coordinates (npts, *pts) and the bounding box (bounds) of the two
  // polygons. Also supply a tolerance squared for controlling
  // error. The method returns 1 if there is an intersection, and 0 if
  // not. A single point of intersection x[3] is also returned if there
  // is an intersection.
  static int IntersectPolygonWithPolygon(int npts, double *pts, double bounds[6],
                                         int npts2, double *pts2,
                                         double bounds2[3], double tol,
                                         double x[3]);

protected:
  vtkPolygon();
  ~vtkPolygon();

  // variables used by instances of this class
  double   Tolerance; // Intersection tolerance
  int      SuccessfulTriangulation; // Stops recursive tri. if necessary
  double   Normal[3]; //polygon normal
  vtkIdList *Tris;
  vtkTriangle *Triangle;
  vtkQuad *Quad;
  vtkDoubleArray *TriScalars;
  vtkLine *Line;

  // Helper methods for triangulation------------------------------
  // Description:
  // A fast triangulation method. Uses recursive divide and
  // conquer based on plane splitting  to reduce loop into triangles.
  // The cell (e.g., triangle) is presumed properly initialized (i.e.,
  // Points and PointIds).
  int EarCutTriangulation();

private:
  vtkPolygon(const vtkPolygon&);  // Not implemented.
  void operator=(const vtkPolygon&);  // Not implemented.
};

#endif

