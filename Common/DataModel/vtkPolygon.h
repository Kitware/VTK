// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPolygon
 * @brief   a cell that represents an n-sided polygon
 *
 * vtkPolygon is a concrete implementation of vtkCell to represent a 2D
 * n-sided polygon. The polygons cannot have any internal holes, and cannot
 * self-intersect. Define the polygon with n-points ordered in the counter-
 * clockwise direction; do not repeat the last point. Also note that the
 * polygon may be defined in 3D space; i.e., it is not constrained to the
 * x-y plane.
 */

#ifndef vtkPolygon_h
#define vtkPolygon_h

#include "vtkCell.h"
#include "vtkCellStatus.h"            // For return type
#include "vtkCommonDataModelModule.h" // For export macro

#include <cmath>   // For std::sqrt in the quality ear clip
#include <utility> // For std::swap in the quality ear clip
#include <vector>  // For ear-clip scratch buffers

VTK_ABI_NAMESPACE_BEGIN
class vtkDoubleArray;
class vtkIdTypeArray;
class vtkLine;
class vtkPoints;
class vtkQuad;
class vtkTriangle;
class vtkPriorityQueue;
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
  static vtkCellStatus ComputeNormal(vtkPoints* p, int numPts, const vtkIdType* pts, double n[3]);
  static vtkCellStatus ComputeNormal(vtkPoints* p, double n[3]);
  static vtkCellStatus ComputeNormal(vtkIdTypeArray* ids, vtkPoints* pts, double n[3]);
  ///@}

  /**
   * Compute the polygon normal from an array of points. This version assumes
   * that the polygon is convex, and looks for the first valid normal.
   */
  static vtkCellStatus ComputeNormal(int numPts, double* pts, double n[3]);

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
   *
   * Note that in order to test convexity, the polygon must have a well-defined
   * normal vector (i.e., have at least 3 points that are not collinear) and
   * should also be planar to within some tolerance (if there are 4 or more points).
   * Thus some variants of this method may return vtkCellStatus to indicate problems
   * other than convexity.
   *
   * A default planarity tolerance is used in variants that do not explicitly accept one.
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
  static vtkCellStatus ComputeCentroid(
    vtkPoints* p, int numPts, const vtkIdType* pts, double centroid[3], double tolerance);
  static bool ComputeCentroid(vtkPoints* p, int numPts, const vtkIdType* pts, double centroid[3]);
  static bool ComputeCentroid(vtkIdTypeArray* ids, vtkPoints* pts, double centroid[3]);
  ///@}

  /**
   * Compute a circle interior to a polygon. While this method does not enforce that the
   * polygon is convex, concave polygons may produce unusual results. The incircle
   * algorithm is simple: first the centroid is determined, then the minimum radius
   * from the centroid to the polygon edges is returned. If the polygon is regular,
   * then the method will produce an incircle.
   */
  static bool ComputeInteriorCircle(
    vtkPoints* p, int numPts, const vtkIdType* ids, double center[3], double& radius2);

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
   * Compute the area of a polygon from a flat array of 3D point coordinates
   * (packed as x0,y0,z0, x1,y1,z1, ...). The unit outward normal is returned
   * via @a normal as a side effect. Uses Newell's method, which is correct for
   * planar polygons regardless of convexity or point traversal order.
   * This is the flat-array counterpart to ComputeArea(vtkPoints*,...).
   */
  static double ComputeArea(int numPts, double* pts, double normal[3]);

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
  ~vtkPolygon() override = default;

  // Compute the interpolation functions using Mean Value Coordinate.
  void InterpolateFunctionsUsingMVC(const double x[3], double* weights);

  // variables used by instances of this class
  double Tolerance;        // Intersection tolerance set by public API
  double Tol;              // Internal tolerance set by ComputeBounds()
  void ComputeTolerance(); // Compute the internal tolerance Tol

  int SuccessfulTriangulation;     // Stops recursive triangulation if necessary
  vtkSmartPointer<vtkIdList> Tris; // Output triangulation placed here

  // These are used for internal computation.
  vtkSmartPointer<vtkTriangle> Triangle;
  vtkSmartPointer<vtkQuad> Quad;
  vtkSmartPointer<vtkDoubleArray> TriScalars;
  vtkSmartPointer<vtkLine> Line;
  vtkSmartPointer<vtkPriorityQueue> EarClipQueue; // reused ear-clip removal queue

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

  ///@{
  /**
   * Templated, allocation-free ear-clip triangulation of a simple (possibly
   * non-convex) 3D polygon, used by the rendering primitive builders. Unlike
   * the quality EarCutTriangulation(), which is a
   * geometry-quality, vtkPoints/vtkIdList-based instance method, these operate
   * directly on caller-provided point and connectivity accessors with zero
   * copies and no per-call allocation, and reproduce the historical
   * fan-from-vertex-0 for convex polygons so existing image baselines are
   * preserved. This forward-walk family is the rendering fast path; the quality,
   * measure-ordered triangulation used elsewhere is the EarCutTriangulation()
   * instance method.
   *
   * Template parameters:
   *   PointsRange - points[i] returns a 3-component, []-indexable coordinate
   *   CellIter    - cell[i] returns the polygon-local i-th vertex's point id
   *   EmitFn      - emit(localA, localB, localC, edgeMask): localA/B/C are
   *                 polygon-local indices (0..cellSize-1); edgeMask is a 3-bit
   *                 polygon-boundary mask (bit 0:(A,B) bit 1:(B,C) bit 2:(C,A)).
   *
   * CompactPolygonRing() fills @a ring with the polygon-local indices of the
   * distinct vertices (dropping coincident consecutive and wrap-around
   * vertices) and returns its size. EarClipTriangleCount() returns the number
   * of triangles EarClipPolygon3D() will emit (max(0, distinct - 2)).
   * EarClipPolygon3D() performs the triangulation, invoking @a emit once per
   * triangle and using the caller-provided scratch buffers (resized as needed,
   * so no allocation occurs in steady state).
   */
  template <typename PointsRange, typename CellIter>
  static int CompactPolygonRing(
    const PointsRange& points, CellIter cell, int cellSize, std::vector<int>& ring);
  template <typename PointsRange, typename CellIter>
  static vtkIdType EarClipTriangleCount(
    const PointsRange& points, CellIter cell, int cellSize, std::vector<int>& ring);
  template <typename PointsRange, typename CellIter, typename EmitFn>
  static void EarClipPolygon3D(const PointsRange& points, CellIter cell, int cellSize,
    std::vector<int>& prevBuf, std::vector<int>& nextBuf, std::vector<int>& ring, EmitFn&& emit);
  ///@}

private:
  vtkPolygon(const vtkPolygon&) = delete;
  void operator=(const vtkPolygon&) = delete;
};

//------------------------------------------------------------------------------
template <typename PointsRange, typename CellIter>
int vtkPolygon::CompactPolygonRing(
  const PointsRange& points, CellIter cell, int cellSize, std::vector<int>& ring)
{
  // Build the ring of distinct vertices, dropping consecutive coincident
  // vertices (degenerate edges). Banded contouring and clipping routinely emit
  // polygons with coincident consecutive vertices when a scalar lands exactly
  // on a clip value; the historical fan triangulation tolerated these by
  // skipping zero-area triangles, and the ear clip must do the same or it
  // computes a bogus normal / ear test at the duplicated vertex.
  ring.clear();
  for (int i = 0; i < cellSize; ++i)
  {
    if (!ring.empty())
    {
      auto pPrev = points[cell[ring.back()]];
      auto pCur = points[cell[i]];
      if (pPrev[0] == pCur[0] && pPrev[1] == pCur[1] && pPrev[2] == pCur[2])
      {
        continue; // coincident with previous kept vertex
      }
    }
    ring.push_back(i);
  }
  // Drop the last vertex if it coincides with the first (wrap-around duplicate).
  if (ring.size() >= 2)
  {
    auto pFirst = points[cell[ring.front()]];
    auto pLast = points[cell[ring.back()]];
    if (pFirst[0] == pLast[0] && pFirst[1] == pLast[1] && pFirst[2] == pLast[2])
    {
      ring.pop_back();
    }
  }
  return static_cast<int>(ring.size());
}

//------------------------------------------------------------------------------
template <typename PointsRange, typename CellIter>
vtkIdType vtkPolygon::EarClipTriangleCount(
  const PointsRange& points, CellIter cell, int cellSize, std::vector<int>& ring)
{
  const int m = vtkPolygon::CompactPolygonRing(points, cell, cellSize, ring);
  return m >= 3 ? static_cast<vtkIdType>(m - 2) : 0;
}

//------------------------------------------------------------------------------
template <typename PointsRange, typename CellIter, typename EmitFn>
void vtkPolygon::EarClipPolygon3D(const PointsRange& points, CellIter cell, int cellSize,
  std::vector<int>& prevBuf, std::vector<int>& nextBuf, std::vector<int>& ring, EmitFn&& emit)
{
  const int m = vtkPolygon::CompactPolygonRing(points, cell, cellSize, ring);
  if (m < 3)
  {
    return; // fully degenerate (collapses to a point or segment): no triangles
  }

  // ring[k] is the polygon-local index of the k-th distinct vertex. Work in
  // compacted space 0..m-1; map back through ring[] when emitting and looking
  // up coordinates.
  auto P = [&](int k) { return points[cell[ring[k]]]; };

  // Compute polygon normal via Newell's method over the compacted ring.
  double normal[3] = { 0.0, 0.0, 0.0 };
  {
    auto pLast = P(m - 1);
    double xp = pLast[0], yp = pLast[1], zp = pLast[2];
    for (int i = 0; i < m; ++i)
    {
      auto pi = P(i);
      double x = pi[0], y = pi[1], z = pi[2];
      normal[0] += (yp - y) * (zp + z);
      normal[1] += (zp - z) * (xp + x);
      normal[2] += (xp - x) * (yp + y);
      xp = x;
      yp = y;
      zp = z;
    }
  }

  // Emit a triangle given compacted-ring indices, computing the polygon-
  // boundary edge mask from compacted-ring adjacency, then mapping to local.
  auto emitRing = [&](int a, int b, int c)
  {
    auto isBoundary = [m](int x, int y) -> int
    {
      const int d = (y - x + m) % m;
      return (d == 1 || d == m - 1) ? 1 : 0;
    };
    int mask = isBoundary(a, b);
    mask |= isBoundary(b, c) << 1;
    mask |= isBoundary(c, a) << 2;
    emit(ring[a], ring[b], ring[c], mask);
  };

  const double normLen2 = normal[0] * normal[0] + normal[1] * normal[1] + normal[2] * normal[2];
  if (normLen2 == 0.0)
  {
    // Zero-area (collinear) polygon. No meaningful triangulation; fall back to
    // a fan over the compacted ring so the emit count matches m - 2.
    for (int i = 1; i < m - 1; ++i)
    {
      emitRing(0, i, i + 1);
    }
    return;
  }

  // Doubly-linked circular list over the compacted ring 0..m-1.
  prevBuf.resize(m);
  nextBuf.resize(m);
  for (int i = 0; i < m; ++i)
  {
    prevBuf[i] = (i + m - 1) % m;
    nextBuf[i] = (i + 1) % m;
  }

  auto isEar = [&](int b) -> bool
  {
    const int a = prevBuf[b];
    const int c = nextBuf[b];
    auto pa = P(a);
    auto pb = P(b);
    auto pc = P(c);
    const double ax = pa[0], ay = pa[1], az = pa[2];
    const double bx = pb[0], by = pb[1], bz = pb[2];
    const double cx = pc[0], cy = pc[1], cz = pc[2];

    // Convexity at b: ((b - a) x (c - b)) . normal > 0
    const double e1x = bx - ax, e1y = by - ay, e1z = bz - az;
    const double e2x = cx - bx, e2y = cy - by, e2z = cz - bz;
    const double crx = e1y * e2z - e1z * e2y;
    const double cry = e1z * e2x - e1x * e2z;
    const double crz = e1x * e2y - e1y * e2x;
    if (crx * normal[0] + cry * normal[1] + crz * normal[2] <= 0.0)
    {
      return false; // reflex or zero-area
    }

    // No other vertex in the remaining polygon may lie strictly inside
    // triangle (a, b, c). Use the polygon normal as the projection axis for
    // a same-side test against each triangle edge.
    for (int q = nextBuf[c]; q != a; q = nextBuf[q])
    {
      auto pq = P(q);
      const double qx = pq[0], qy = pq[1], qz = pq[2];
      auto sideOf = [&](double sx, double sy, double sz, double ex, double ey, double ez) -> double
      {
        const double dx = ex - sx, dy = ey - sy, dz = ez - sz;
        const double rx = qx - sx, ry = qy - sy, rz = qz - sz;
        const double tcrx = dy * rz - dz * ry;
        const double tcry = dz * rx - dx * rz;
        const double tcrz = dx * ry - dy * rx;
        return tcrx * normal[0] + tcry * normal[1] + tcrz * normal[2];
      };
      const double s1 = sideOf(ax, ay, az, bx, by, bz);
      const double s2 = sideOf(bx, by, bz, cx, cy, cz);
      const double s3 = sideOf(cx, cy, cz, ax, ay, az);
      if (s1 > 0.0 && s2 > 0.0 && s3 > 0.0)
      {
        return false;
      }
    }
    return true;
  };

  // Clip ears walking forward from vertex 1, keeping the clipped vertex's
  // successor as the next candidate. For a convex polygon this clips vertices
  // 1, 2, 3, ... in order while vertex 0 remains the common apex, reproducing
  // exactly the fan-from-vertex-0 triangulation VTK has always emitted (and
  // that existing image baselines were generated with). Triangulation is not
  // invariant under interior diagonals: per-triangle texture-coordinate and
  // Gouraud interpolation depend on the diagonals chosen, so a convex polygon
  // must keep producing the fan. For non-convex polygons reflex vertices are
  // skipped and the walk still finds valid ears.
  int remaining = m;
  int current = 1 % m;
  int safetyBudget = 2 * m;
  while (remaining > 3 && safetyBudget > 0)
  {
    if (isEar(current))
    {
      const int a = prevBuf[current];
      const int c = nextBuf[current];
      emitRing(a, current, c);
      nextBuf[a] = c;
      prevBuf[c] = a;
      current = c;
      --remaining;
      safetyBudget = 2 * m;
    }
    else
    {
      current = nextBuf[current];
      --safetyBudget;
    }
  }

  if (remaining > 3)
  {
    // Ear-clip stalled. Emit remaining vertices as a fan from `current` to
    // satisfy the emit-pass triangle count. Result will be visually wrong for
    // a non-convex remainder, but degenerate input has no correct triangulation.
    const int head = current;
    int v = nextBuf[head];
    int vn = nextBuf[v];
    while (vn != head)
    {
      emitRing(head, v, vn);
      v = vn;
      vn = nextBuf[vn];
    }
    return;
  }

  // Final triangle: the three vertices still in the list. Emit it anchored at
  // prev(current) so that for a convex polygon (where current has advanced to
  // the second-to-last vertex with vertex 0 still the apex) the triangle is
  // (0, m-2, m-1) - the same vertex order and boundary-edge mask the fan emits.
  const int b = current;
  const int a = prevBuf[b];
  const int c = nextBuf[b];
  emitRing(a, b, c);
}

VTK_ABI_NAMESPACE_END
#endif
