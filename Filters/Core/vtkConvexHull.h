// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkConvexHull
 * @brief   Convex hull and point-in-hull testing for 1-D, 2-D, and 3-D point sets.
 *
 * vtkConvexHull is a vtkPointSetAlgorithm that computes the convex hull of a
 * vtkPointSet input and optionally produces a vtkPolyData output representing
 * the hull geometry. The fastest known algorithm is chosen for each dimension:
 *   - Dimension=1  O(n)       — scan for min/max along the line direction.
 *   - Dimension=2  O(n log n) — Andrew's monotone chain for coplanar point sets.
 *   - Dimension=3  O(n log n) — Quickhull for general 3-D point sets.
 *
 * The hull is represented as a set of oriented half-planes {Normal, D} such
 * that a point P is inside the hull iff P·Normal ≤ D for every plane.
 *
 * Static overloads of ComputeConvexHull() are provided for use in tight loops
 * (e.g., per-cell queries) where VTK pipeline overhead is undesirable.  They
 * write directly into a caller-supplied std::vector<vtkConvexHull::Plane> whose
 * capacity is preserved across calls to avoid repeated heap activity.
 *
 * @sa vtkHull
 */

#ifndef vtkConvexHull_h
#define vtkConvexHull_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"
#include "vtkVector.h"        // For vtkVector3D
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN

class vtkDataArray;
class vtkPolyData;

//------------------------------------------------------------------------------
class VTKFILTERSCORE_EXPORT VTK_MARSHALAUTO vtkConvexHull : public vtkPointSetAlgorithm
{
public:
  static vtkConvexHull* New();
  vtkTypeMacro(vtkConvexHull, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Half-plane representation: point P is inside the hull iff P·Normal ≤ D
   * for every plane in the set.
   */
  struct Plane
  {
    vtkVector3d Normal;
    double D{ 0.0 };
  };

  ///@{
  /// Embedding dimension (1, 2, or 3).
  vtkSetClampMacro(Dimension, int, 1, 3);
  vtkGetMacro(Dimension, int);
  ///@}

  ///@{
  /**
   * Control whether the pipeline pass generates vtkPolyData hull geometry
   * (default: on). When off, only the half-plane representation is computed
   * and IsPointWithinConvexHull() becomes available without geometry cost.
   */
  vtkSetMacro(GeneratePolyData, bool);
  vtkGetMacro(GeneratePolyData, bool);
  vtkBooleanMacro(GeneratePolyData, bool);
  ///@}

  /**
   * Return true iff the point (x, y, z) lies inside (or on the boundary of)
   * the hull computed by the most recent pipeline update.
   */
  bool IsPointWithinConvexHull(double x, double y, double z, double tol = 1e-8) const;
  bool IsPointWithinConvexHull(const double point[3], double tol = 1e-8) const;

  /**
   * Return true iff \a p lies inside (or on the boundary of) the given half-planes.
   * For use with planes obtained via ComputeConvexHull() without a live pipeline object.
   */
  static bool IsPointInside(
    const vtkVector3d& p, const std::vector<Plane>& planes, double tol = 1e-8);

  /**
   * Compute the convex hull of the points in \a points (a vtkDataArray with
   * 3 components) for the given \a dimension and write the half-planes into
   * \a planes.  The capacity of \a planes is preserved across calls.
   */
  static void ComputeConvexHull(vtkDataArray* points, int dimension, std::vector<Plane>& planes);

  /**
   * Compute the convex hull of \a numPoints points stored in \a points for
   * the given \a dimension and write the half-planes into \a planes.
   * The capacity of \a planes is preserved across calls.
   */
  static void ComputeConvexHull(
    const vtkVector3d* points, int numPoints, int dimension, std::vector<Plane>& planes);

protected:
  vtkConvexHull();
  ~vtkConvexHull() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillOutputPortInformation(int port, vtkInformation* info) override;

private:
  vtkConvexHull(const vtkConvexHull&) = delete;
  void operator=(const vtkConvexHull&) = delete;

  int Dimension{ 3 };
  bool GeneratePolyData{ true };
  std::vector<Plane> HullPlanes;

  static void Compute1D(
    const vtkVector3d* pts, vtkIdType n, std::vector<Plane>& planes, vtkPolyData* geom);
  static void Compute2D(
    const vtkVector3d* pts, vtkIdType n, std::vector<Plane>& planes, vtkPolyData* geom);
  static void Compute3D(
    const vtkVector3d* pts, vtkIdType n, std::vector<Plane>& planes, vtkPolyData* geom);
};

VTK_ABI_NAMESPACE_END
#endif // vtkConvexHull_h
