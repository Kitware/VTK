/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTriangle - a cell that represents a triangle
// .SECTION Description
// vtkTriangle is a concrete implementation of vtkCell to represent a triangle
// located in 3-space.

#ifndef __vtkTriangle_h
#define __vtkTriangle_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkCell.h"

#include "vtkMath.h" // Needed for inline methods

class vtkLine;
class vtkQuadric;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkTriangle : public vtkCell
{
public:
  static vtkTriangle *New();
  vtkTypeMacro(vtkTriangle,vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the edge specified by edgeId (range 0 to 2) and return that edge's
  // coordinates.
  vtkCell *GetEdge(int edgeId);

  // Description:
  // See the vtkCell API for descriptions of these methods.
  int GetCellType() {return VTK_TRIANGLE;};
  int GetCellDimension() {return 2;};
  int GetNumberOfEdges() {return 3;};
  int GetNumberOfFaces() {return 0;};
  vtkCell *GetFace(int) {return 0;};
  int CellBoundary(int subId, double pcoords[3], vtkIdList *pts);
  void Contour(double value, vtkDataArray *cellScalars,
               vtkIncrementalPointLocator *locator, vtkCellArray *verts,
               vtkCellArray *lines, vtkCellArray *polys,
               vtkPointData *inPd, vtkPointData *outPd,
               vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd);
  int EvaluatePosition(double x[3], double* closestPoint,
                       int& subId, double pcoords[3],
                       double& dist2, double *weights);
  void EvaluateLocation(int& subId, double pcoords[3], double x[3],
                        double *weights);
  int Triangulate(int index, vtkIdList *ptIds, vtkPoints *pts);
  void Derivatives(int subId, double pcoords[3], double *values,
                   int dim, double *derivs);
  virtual double *GetParametricCoords();

  // Description:
  // A convenience function to compute the area of a vtkTriangle.
  double ComputeArea();

  // Description:
  // Clip this triangle using scalar value provided. Like contouring, except
  // that it cuts the triangle to produce other triangles.
  void Clip(double value, vtkDataArray *cellScalars,
            vtkIncrementalPointLocator *locator, vtkCellArray *polys,
            vtkPointData *inPd, vtkPointData *outPd,
            vtkCellData *inCd, vtkIdType cellId, vtkCellData *outCd,
            int insideOut);

  // Description:
  // @deprecated Replaced by vtkTriangle::InterpolateFunctions as of VTK 5.2
  static void InterpolationFunctions(double pcoords[3], double sf[3]);
  // Description:
  // @deprecated Replaced by vtkTriangle::InterpolateDerivs as of VTK 5.2
  static void InterpolationDerivs(double pcoords[3], double derivs[6]);
  // Description:
  // Compute the interpolation functions/derivatives
  // (aka shape functions/derivatives)
  virtual void InterpolateFunctions(double pcoords[3], double sf[3])
    {
    vtkTriangle::InterpolationFunctions(pcoords,sf);
    }
  virtual void InterpolateDerivs(double pcoords[3], double derivs[6])
    {
    vtkTriangle::InterpolationDerivs(pcoords,derivs);
    }
  // Description:
  // Return the ids of the vertices defining edge (`edgeId`).
  // Ids are related to the cell, not to the dataset.
  int *GetEdgeArray(int edgeId);

  // Description:
  // Plane intersection plus in/out test on triangle. The in/out test is
  // performed using tol as the tolerance.
  int IntersectWithLine(double p1[3], double p2[3], double tol, double& t,
                        double x[3], double pcoords[3], int& subId);

  // Description:
  // Return the center of the triangle in parametric coordinates.
  int GetParametricCenter(double pcoords[3]);

  // Description:
  // Return the distance of the parametric coordinate provided to the
  // cell. If inside the cell, a distance of zero is returned.
  double GetParametricDistance(double pcoords[3]);

  // Description:
  // Compute the center of the triangle.
  static void TriangleCenter(double p1[3], double p2[3], double p3[3],
                             double center[3]);

  // Description:
  // Compute the area of a triangle in 3D.
  // See also vtkTriangle::ComputeArea()
  static double TriangleArea(double p1[3], double p2[3], double p3[3]);

  // Description:
  // Compute the circumcenter (center[3]) and radius squared (method
  // return value) of a triangle defined by the three points x1, x2,
  // and x3. (Note that the coordinates are 2D. 3D points can be used
  // but the z-component will be ignored.)
  static double Circumcircle(double  p1[2], double p2[2], double p3[2],
                            double center[2]);

  // Description:
  // Given a 2D point x[2], determine the barycentric coordinates of the point.
  // Barycentric coordinates are a natural coordinate system for simplices that
  // express a position as a linear combination of the vertices. For a
  // triangle, there are three barycentric coordinates (because there are
  // three vertices), and the sum of the coordinates must equal 1. If a
  // point x is inside a simplex, then all three coordinates will be strictly
  // positive.  If two coordinates are zero (so the third =1), then the
  // point x is on a vertex. If one coordinates are zero, the point x is on an
  // edge. In this method, you must specify the vertex coordinates x1->x3.
  // Returns 0 if triangle is degenerate.
  static int BarycentricCoords(double x[2], double  x1[2], double x2[2],
                               double x3[2], double bcoords[3]);


  // Description:
  // Project triangle defined in 3D to 2D coordinates. Returns 0 if
  // degenerate triangle; non-zero value otherwise. Input points are x1->x3;
  // output 2D points are v1->v3.
  static int ProjectTo2D(double x1[3], double x2[3], double x3[3],
                         double v1[2], double v2[2], double v3[2]);

  // Description:
  // Compute the triangle normal from a points list, and a list of point ids
  // that index into the points list.
  static void ComputeNormal(vtkPoints *p, int numPts, vtkIdType *pts,
                            double n[3]);

  // Description:
  // Compute the triangle normal from three points.
  static void ComputeNormal(double v1[3], double v2[3], double v3[3], double n[3]);

  // Description:
  // Compute the (unnormalized) triangle normal direction from three points.
  static void ComputeNormalDirection(double v1[3], double v2[3], double v3[3],
                                     double n[3]);

  // Description:
  // Given a point x, determine whether it is inside (within the
  // tolerance squared, tol2) the triangle defined by the three
  // coordinate values p1, p2, p3. Method is via comparing dot products.
  // (Note: in current implementation the tolerance only works in the
  // neighborhood of the three vertices of the triangle.
  static int PointInTriangle(double x[3], double x1[3],
                             double x2[3], double x3[3],
                             double tol2);

  // Description:
  // Calculate the error quadric for this triangle.  Return the
  // quadric as a 4x4 matrix or a vtkQuadric.  (from Peter
  // Lindstrom's Siggraph 2000 paper, "Out-of-Core Simplification of
  // Large Polygonal Models")
  static void ComputeQuadric(double x1[3], double x2[3], double x3[3],
                             double quadric[4][4]);
  static void ComputeQuadric(double x1[3], double x2[3], double x3[3],
                             vtkQuadric *quadric);


protected:
  vtkTriangle();
  ~vtkTriangle();

  vtkLine *Line;

private:
  vtkTriangle(const vtkTriangle&);  // Not implemented.
  void operator=(const vtkTriangle&);  // Not implemented.
};

//----------------------------------------------------------------------------
inline int vtkTriangle::GetParametricCenter(double pcoords[3])
{
  pcoords[0] = pcoords[1] = 1./3; pcoords[2] = 0.0;
  return 0;
}

//----------------------------------------------------------------------------
inline void vtkTriangle::ComputeNormalDirection(double v1[3], double v2[3],
                                       double v3[3], double n[3])
{
  double ax, ay, az, bx, by, bz;

  // order is important!!! maintain consistency with triangle vertex order
  ax = v3[0] - v2[0]; ay = v3[1] - v2[1]; az = v3[2] - v2[2];
  bx = v1[0] - v2[0]; by = v1[1] - v2[1]; bz = v1[2] - v2[2];

  n[0] = (ay * bz - az * by);
  n[1] = (az * bx - ax * bz);
  n[2] = (ax * by - ay * bx);
}

//----------------------------------------------------------------------------
inline void vtkTriangle::ComputeNormal(double v1[3], double v2[3],
                                       double v3[3], double n[3])
{
  double length;

  vtkTriangle::ComputeNormalDirection(v1, v2, v3, n);

  if ( (length = sqrt((n[0]*n[0] + n[1]*n[1] + n[2]*n[2]))) != 0.0 )
    {
    n[0] /= length;
    n[1] /= length;
    n[2] /= length;
    }
}

//----------------------------------------------------------------------------
inline void vtkTriangle::TriangleCenter(double p1[3], double p2[3],
                                        double p3[3], double center[3])
{
  center[0] = (p1[0]+p2[0]+p3[0]) / 3.0;
  center[1] = (p1[1]+p2[1]+p3[1]) / 3.0;
  center[2] = (p1[2]+p2[2]+p3[2]) / 3.0;
}

//----------------------------------------------------------------------------
inline double vtkTriangle::TriangleArea(double p1[3], double p2[3], double p3[3])
{
  double a,b,c;
  a = vtkMath::Distance2BetweenPoints(p1,p2);
  b = vtkMath::Distance2BetweenPoints(p2,p3);
  c = vtkMath::Distance2BetweenPoints(p3,p1);
  return (0.25* sqrt(fabs(4.0*a*c - (a-b+c)*(a-b+c))));
}

#endif


