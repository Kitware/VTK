// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolygon
 * @brief   a cell that represents an n-sided polygon
 *
 * vtkPolygon is a concrete implementation of vtkCell to represent a 2D
 * n-sided polygon. The polygons cannot have any internal holes, and cannot
 * self-intersect. Define the polygon with n-points ordered in the counter-
 * clockwise direction; do not repeat the last point.
 */

#ifndef vtkPolygon_h
#define vtkPolygon_h

#include "vtkCell.h"
#include "vtkCommonDataModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkLine;
class vtkPoints;
class vtkQuad;
class vtkTriangle;
class vtkIncrementalPointLocator;

class VTKCOMMONDATAMODEL_EXPORT vtkPolygon : public vtkCell
{
public:
  static vtkPolygon* New();
  vtkTypeMacro(vtkPolygon, vtkCell);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * See the vtkCell API for descriptions of these methods.
   */
  int GetCellType() override { return VTK_POLYGON; }
  int GetCellDimension() override { return 2; }
  int GetNumberOfEdges() override { return this->GetNumberOfPoints(); }
  int GetNumberOfFaces() override { return 0; }
  vtkCell* GetEdge(int edgeId) override;
  vtkCell* GetFace(int) override { return nullptr; }
  int CellBoundary(int subId, const double pcoords[3], vtkIdList* pts) override;
  void Contour(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* verts, vtkCellArray* lines, vtkCellArray* polys, vtkPointData* inPd,
    vtkPointData* outPd, vtkCellData* inCd, vtkIdType cellId, vtkCellData* outCd) override;
  void Clip(double value, vtkDataArray* cellScalars, vtkIncrementalPointLocator* locator,
    vtkCellArray* tris, vtkPointData* inPd, vtkPointData* outPd, vtkCellData* inCd,
    vtkIdType cellId, vtkCellData* outCd, int insideOut) override;
  int EvaluatePosition(const double x[3], double closestPoint[3], int& subId, double pcoords[3],
    double& dist2, double weights[]) override;
  void EvaluateLocation(int& subId, const double pcoords[3], double x[3], double* weights) override;
  int IntersectWithLine(const double p1[3], const double p2[3], double tol, double& t, double x[3],
    double pcoords[3], int& subId) override;
  int TriangulateLocalIds(int index, vtkIdList* ptIds) override;
  void Derivatives(
    int subId, const double pcoords[3], const double* values, int dim, double* derivs) override;
  int IsPrimaryCell() VTK_FUTURE_CONST override { return 0; }
  ///@}

  /**
   * Compute the area of a polygon. This is a convenience function
   * which simply calls static double ComputeArea(vtkPoints *p,
   * vtkIdType numPts, vtkIdType *pts, double normal[3]);
   * with the appropriate parameters from the instantiated vtkPolygon.
   */
  double ComputeArea();

  /**
   * Compute the interpolation functions/derivatives.
   * (aka shape functions/derivatives)
   * Two interpolation algorithms are available: 1/r^2 and Mean Value
   * Coordinate. The former is used by default. To use the second algorithm,
   * set UseMVCInterpolation to be true.
   * The function assumes the input point lies on the polygon plane without
   * checking that.
   */
  void InterpolateFunctions(const double x[3], double* sf) override;

  ///@{
  /**
   * Computes the unit normal to the polygon. If pts=nullptr, point indexing is
   * assumed to be {0, 1, ..., numPts-1}.
   */
  static void ComputeNormal(vtkPoints* p, int numPts, const vtkIdType* pts, double n[3]);
  static void ComputeNormal(vtkPoints* p, double n[3]);
  static void ComputeNormal(vtkIdTypeArray* ids, vtkPoints* pts, double n[3]);
  ///@}

  /**
   * Compute the polygon normal from an array of points. This version assumes
   * that the polygon is convex, and looks for the first valid normal.
   */
  static void ComputeNormal(int numPts, double* pts, double n[3]);

  /**
   * Determine whether or not a polygon is convex. This is a convenience
   * function that simply calls static bool IsConvex(int numPts,
   * vtkIdType *pts, vtkPoints *p) with the appropriate parameters from the
   * instantiated vtkPolygon.
   */
  bool IsConvex();

  ///@{
  /**
   * Determine whether or not a polygon is convex. If pts=nullptr, point indexing
   * is assumed to be {0, 1, ..., numPts-1}.
   */
  static bool IsConvex(vtkPoints* p, int numPts, const vtkIdType* pts);
  static bool IsConvex(vtkIdTypeArray* ids, vtkPoints* p);
  static bool IsConvex(vtkPoints* p);
  ///@}

  ///@{
  /**
   * Compute the centroid of a set of points. Returns false if the computation
   * is invalid (this occurs when numPts=0 or when ids is empty).
   *
   * The strategy used is to compute the average coordinate x_c (the center, but not
   * the centroid of the polygon) and then apply the "geometric decomposition"
   * method for centroids to an area-weighted sum centroids of triangles formed from
   * the center x_c to each edge of the polygon.
   *
   * This method is robust to significant non-planarity of the polygon, but not
   * so much that the normal computation is invalid. If the normal cannot be
   * determined or the total area of the polygon is near zero, then false will be returned.
   *
   * If a \a tolerance is provided, the ratio of the out-of-plane extent of the
   * polygon (dZ) relative to the longest in-plane extent of the polygon (dS) is
   * compared to it.
   * If dZ / dS > \a tolerance , then false will be returned and the \a centroid
   * will be unmodified.
   *
   * The default is \a tolerance of 0.1.
   * To ignore non-planar polygons, pass a tolerance <  – but note that the normal
   * is estimated from the point coordinates and thus the centroid will become
   * ill-conditioned for large deviations from the plane.
   */
  static bool ComputeCentroid(
    vtkPoints* p, int numPts, const vtkIdType* pts, double centroid[3], double tolerance);
  static bool ComputeCentroid(vtkPoints* p, int numPts, const vtkIdType* pts, double centroid[3]);
  static bool ComputeCentroid(vtkIdTypeArray* ids, vtkPoints* pts, double centroid[3]);
  ///@}

  /**
   * Compute the area of a polygon in 3D. The area is returned, as well as
   * the normal (a side effect of using this method). If you desire to
   * compute the area of a triangle, use vtkTriangleArea which is faster.
   * If pts==nullptr, point indexing is supposed to be {0, 1, ..., numPts-1}.
   * If you already have a vtkPolygon instantiated, a convenience function,
   * ComputeArea() is provided.
   */
  static double ComputeArea(vtkPoints* p, vtkIdType numPts, const vtkIdType* pts, double normal[3]);

  /**
   * Create a local s-t coordinate system for a polygon. The point p0 is
   * the origin of the local system, p10 is s-axis vector, and p20 is the
   * t-axis vector. (These are expressed in the modeling coordinate system and
   * are vectors of dimension [3].) The values l20 and l20 are the lengths of
   * the vectors p10 and p20, and n is the polygon normal.
   */
  int ParameterizePolygon(
    double p0[3], double p10[3], double& l10, double p20[3], double& l20, double n[3]);

  /**
   * Determine whether a point is inside the specified polygon. The function
   * computes the winding number to assess inclusion. It works for arbitrary
   * polygon shapes (e.g., non-convex) oriented arbitrarily in 3D
   * space. Returns 0 if the point is not in the polygon; 1 if it is.  Can
   * also return -1 to indicate a degenerate polygon. Parameters passed into
   * the method include the point in question x[3]; the polygon defined by
   * (npts,pts); the bounds of the polygon bounds[6]; and the normal n[3] to
   * the polygon. (The implementation was inspired by Dan Sunday's book
   * Practical Geometry Algorithms.) This method is thread safe.
   */
  static int PointInPolygon(double x[3], int numPts, double* pts, double bounds[6], double n[3]);

  // Needed to remove warning "member function does not override any
  // base class virtual member function"
  int Triangulate(int index, vtkIdList* ptIds, vtkPoints* pts) override
  {
    return vtkCell::Triangulate(index, ptIds, pts);
  }

  /**
   * Same as Triangulate(vtkIdList *outTris)
   * but with a first pass to split the polygon into non-degenerate polygons.
   */
  int NonDegenerateTriangulate(vtkIdList* outTris);

  /**
   * Triangulate polygon and enforce that the ratio of the smallest triangle
   * area to the polygon area is greater than a user-defined tolerance. The user
   * must provide the vtkIdList outTris. On output, the outTris list contains
   * the ids of the points defining the triangulation. The ids are ordered into
   * groups of three: each three-group defines one triangle.
   */
  int BoundedTriangulate(vtkIdList* outTris, double tol);

  /**
   * Compute the distance of a point to a polygon. The closest point on
   * the polygon is also returned. The bounds should be provided to
   * accelerate the computation.
   */
  static double DistanceToPolygon(
    double x[3], int numPts, double* pts, double bounds[6], double closest[3]);

  /**
   * Method intersects two polygons. You must supply the number of points and
   * point coordinates (npts, *pts) and the bounding box (bounds) of the two
   * polygons. Also supply a tolerance squared for controlling
   * error. The method returns 1 if there is an intersection, and 0 if
   * not. A single point of intersection x[3] is also returned if there
   * is an intersection.
   */
  static int IntersectPolygonWithPolygon(int npts, double* pts, double bounds[6], int npts2,
    double* pts2, double bounds2[6], double tol, double x[3]);

  /**
   * Intersect two convex 2D polygons to produce a line segment as output.
   * The return status of the methods indicated no intersection (returns 0);
   * a single point of intersection (returns 1); or a line segment (i.e., two
   * points of intersection, returns 2). The points of intersection are
   * returned in the arrays p0 and p1.  If less than two points of
   * intersection are generated then p1 and/or p0 may be
   * indeterminiate. Finally, if the two convex polygons are parallel, then
   * "0" is returned (i.e., no intersection) even if the triangles lie on one
   * another.
   */
  static int IntersectConvex2DCells(
    vtkCell* cell1, vtkCell* cell2, double tol, double p0[3], double p1[3]);

  ///@{
  /**
   * Set/Get the flag indicating whether to use Mean Value Coordinate for the
   * interpolation. If true, InterpolateFunctions() uses the Mean Value
   * Coordinate to compute weights. Otherwise, the conventional 1/r^2 method
   * is used. The UseMVCInterpolation parameter is set to false by default.
   */
  vtkGetMacro(UseMVCInterpolation, bool);
  vtkSetMacro(UseMVCInterpolation, bool);
  ///@}

  ///@{
  /**
   * Specify an internal tolerance for operations requiring polygon
   * triangulation.  (For example, clipping and contouring operations proceed
   * by first triangulating the polygon, and then clipping/contouring the
   * resulting triangles.)  This is a normalized tolerance value multiplied
   * by the diagonal length of the polygon bounding box. Is it used to
   * determine whether potential triangulation edges intersect one another.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, 1.0);
  vtkGetMacro(Tolerance, double);
  ///@}

protected:
  vtkPolygon();
  ~vtkPolygon() override;

  // Compute the interpolation functions using Mean Value Coordinate.
  void InterpolateFunctionsUsingMVC(const double x[3], double* weights);

  // variables used by instances of this class
  double Tolerance;        // Intersection tolerance set by public API
  double Tol;              // Internal tolerance set by ComputeBounds()
  void ComputeTolerance(); // Compute the internal tolerance Tol

  int SuccessfulTriangulation; // Stops recursive triangulation if necessary
  vtkIdList* Tris;             // Output triangulation placed here

  // These are used for internal computation.
  vtkTriangle* Triangle;
  vtkQuad* Quad;
  vtkDoubleArray* TriScalars;
  vtkLine* Line;

  // Parameter indicating whether to use Mean Value Coordinate algorithm
  // for interpolation. The parameter is false by default.
  bool UseMVCInterpolation;

  // Helper methods for triangulation------------------------------
  // Made public for external access
public:
  // Ear cut triangulation options. The order in which vertices are
  // removed are controlled by different measures. Changing this can
  // make subtle differences in some cases. Historically the
  // PERIMETER2_TO_AREA_RATIO has been used.
  enum EarCutMeasureTypes
  {
    PERIMETER2_TO_AREA_RATIO = 0,
    DOT_PRODUCT = 1,
    BEST_QUALITY = 2
  };

  ///@{
  /**
   * A fast triangulation method. Uses recursive divide and
   * conquer based on plane splitting to reduce loop into triangles.
   * The cell (e.g., triangle) is presumed properly initialized (i.e.,
   * Points and PointIds). Ears can be removed using different measures
   * (the measures indicate convexity plus characterize the local
   * geometry around each vertex).
   */
  int EarCutTriangulation(int measure = PERIMETER2_TO_AREA_RATIO);
  int EarCutTriangulation(vtkIdList* outTris, int measure = PERIMETER2_TO_AREA_RATIO);
  ///@}

  ///@{
  /**
   * A fast triangulation method. Uses recursive divide and
   * conquer based on plane splitting to reduce loop into triangles.
   * The cell (e.g., triangle) is presumed properly initialized (i.e.,
   * Points and PointIds). Unlike EarCutTriangulation(), vertices are visited
   * sequentially without preference to angle.
   */
  int UnbiasedEarCutTriangulation(int seed, int measure = PERIMETER2_TO_AREA_RATIO);
  int UnbiasedEarCutTriangulation(
    int seed, vtkIdList* outTris, int measure = PERIMETER2_TO_AREA_RATIO);
  ///@}

private:
  vtkPolygon(const vtkPolygon&) = delete;
  void operator=(const vtkPolygon&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
