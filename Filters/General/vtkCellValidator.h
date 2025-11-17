// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellValidator
 * @brief   validates cells in a dataset
 *
 *
 * vtkCellValidator accepts as input a dataset and adds integral cell data
 * to it corresponding to the "validity" of each cell. The validity field
 * encodes a bitfield for identifying problems that prevent a cell from standard
 * use, including:
 *
 *   WrongNumberOfPoints: filters assume that a cell has access to the
 *                        appropriate number of points that comprise it. This
 *                        assumption is often tacit, resulting in unexpected
 *                        behavior when the condition is not met. This check
 *                        simply confirms that the cell has the minimum number
 *                        of points needed to describe it.
 *
 *   IntersectingEdges: cells that incorrectly describe the order of their
 *                      points often manifest with intersecting edges or
 *                      intersecting faces. Given a tolerance, this check
 *                      ensures that two edges from a two-dimensional cell
 *                      are separated by at least the tolerance (discounting
 *                      end-to-end connections).
 *
 *   IntersectingFaces: cells that incorrectly describe the order of their
 *                      points often manifest with intersecting edges or
 *                      intersecting faces. Given a tolerance, this check
 *                      ensures that two faces from a three-dimensional cell
 *                      do not intersect.
 *
 *   NoncontiguousEdges: another symptom of incorrect point ordering within a
 *                       cell is the presence of noncontiguous edges where
 *                       contiguous edges are otherwise expected. Given a
 *                       tolerance, this check ensures that edges around the
 *                       perimeter of a two-dimensional cell are contiguous.
 *
 *   Nonconvex: many algorithms implicitly require that all input three-
 *              dimensional cells be convex. This check uses the generic
 *              convexity checkers implemented in vtkPolygon and vtkPolyhedron
 *              to test this requirement.
 *
 *   FacesAreOrientedIncorrectly: All three-dimensional cells have an implicit
 *                                expectation for the orientation of their
 *                                faces. While the convention is unfortunately
 *                                inconsistent across cell types, it is usually
 *                                required that cell faces point outward. This
 *                                check tests that the faces of a cell point in
 *                                the direction required by the cell type,
 *                                taking into account the cell types with
 *                                nonstandard orientation requirements.
 *
 *  NonPlanarFaces: The vertices for a face do not all lie in the same plane, so
 *                  the normal and origin of the plane in which the face lies cannot
 *                  be accurately determined.
 *
 *  DegenerateFaces:  A face is collapsed to a line or a point through repeated
 *                    collocated vertices. This is distinct from WrongNumberOfPoints,
 *                    which indicates there are too few points. In this case, there
 *                    are enough points but they are topologically or geometrically
 *                    degenerate. Topological degeneracy is when connectivity entries
 *                    are repeated. Geometric degeneracy is when point coordinates
 *                    for topologically distinct points are coincident, collinear, or
 *                    coplanar when they ought not to be.
 *
 * CoincidentPoints: A cell is otherwise valid but has coincident points, which may
 *                   arise from distinct entries in vtkPoints with duplicate coordinates
 *                   or from repeated use of the same connectivity entry.
 * @sa
 * vtkCellQuality
 */

#ifndef vtkCellValidator_h
#define vtkCellValidator_h

#include "vtkCellStatus.h" // For enum class.
#include "vtkDataSetAlgorithm.h"
#include "vtkDeprecation.h"          // For VTK_DEPRECATED_IN_9_6_0.
#include "vtkFiltersGeneralModule.h" // For export macro.

VTK_ABI_NAMESPACE_BEGIN
class vtkCell;
class vtkGenericCell;
class vtkEmptyCell;
class vtkVertex;
class vtkPolyVertex;
class vtkLine;
class vtkPolyLine;
class vtkTriangle;
class vtkTriangleStrip;
class vtkPolygon;
class vtkPixel;
class vtkQuad;
class vtkTetra;
class vtkVoxel;
class vtkHexahedron;
class vtkWedge;
class vtkPyramid;
class vtkPentagonalPrism;
class vtkHexagonalPrism;
class vtkQuadraticEdge;
class vtkQuadraticTriangle;
class vtkQuadraticQuad;
class vtkQuadraticPolygon;
class vtkQuadraticTetra;
class vtkQuadraticHexahedron;
class vtkQuadraticWedge;
class vtkQuadraticPyramid;
class vtkBiQuadraticQuad;
class vtkTriQuadraticHexahedron;
class vtkTriQuadraticPyramid;
class vtkQuadraticLinearQuad;
class vtkQuadraticLinearWedge;
class vtkBiQuadraticQuadraticWedge;
class vtkBiQuadraticQuadraticHexahedron;
class vtkBiQuadraticTriangle;
class vtkCubicLine;
class vtkConvexPointSet;
class vtkPolyhedron;
class vtkLagrangeCurve;
class vtkLagrangeTriangle;
class vtkLagrangeQuadrilateral;
class vtkLagrangeTetra;
class vtkLagrangeHexahedron;
class vtkLagrangeWedge;
class vtkBezierCurve;
class vtkBezierTriangle;
class vtkBezierQuadrilateral;
class vtkBezierTetra;
class vtkBezierHexahedron;
class vtkBezierWedge;

class VTKFILTERSGENERAL_EXPORT vtkCellValidator : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkCellValidator, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Construct to compute the validity of cells.
  static vtkCellValidator* New();

  using State = vtkCellStatus;

  static void PrintState(State state, ostream& os, vtkIndent indent);

  static State Check(vtkGenericCell*, double tolerance);
  static State Check(vtkCell*, double tolerance);

  static State Check(vtkEmptyCell*, double tolerance);
  static State Check(vtkVertex*, double tolerance);
  static State Check(vtkPolyVertex*, double tolerance);
  static State Check(vtkLine*, double tolerance);
  static State Check(vtkPolyLine*, double tolerance);
  static State Check(vtkTriangle*, double tolerance);
  static State Check(vtkTriangleStrip*, double tolerance);
  static State Check(vtkPolygon*, double tolerance);
  static State Check(vtkPixel*, double tolerance);
  static State Check(vtkQuad*, double tolerance);
  static State Check(vtkTetra*, double tolerance);
  static State Check(vtkVoxel*, double tolerance);
  static State Check(vtkHexahedron*, double tolerance);
  static State Check(vtkWedge*, double tolerance);
  static State Check(vtkPyramid*, double tolerance);
  static State Check(vtkPentagonalPrism*, double tolerance);
  static State Check(vtkHexagonalPrism*, double tolerance);
  static State Check(vtkQuadraticEdge*, double tolerance);
  static State Check(vtkQuadraticTriangle*, double tolerance);
  static State Check(vtkQuadraticQuad*, double tolerance);
  static State Check(vtkQuadraticPolygon*, double tolerance);
  static State Check(vtkQuadraticTetra*, double tolerance);
  static State Check(vtkQuadraticHexahedron*, double tolerance);
  static State Check(vtkQuadraticWedge*, double tolerance);
  static State Check(vtkQuadraticPyramid*, double tolerance);
  static State Check(vtkBiQuadraticQuad*, double tolerance);
  static State Check(vtkTriQuadraticHexahedron*, double tolerance);
  static State Check(vtkTriQuadraticPyramid*, double tolerance);
  static State Check(vtkQuadraticLinearQuad*, double tolerance);
  static State Check(vtkQuadraticLinearWedge*, double tolerance);
  static State Check(vtkBiQuadraticQuadraticWedge*, double tolerance);
  static State Check(vtkBiQuadraticQuadraticHexahedron*, double tolerance);
  static State Check(vtkBiQuadraticTriangle*, double tolerance);
  static State Check(vtkCubicLine*, double tolerance);
  static State Check(vtkConvexPointSet*, double tolerance);
  static State Check(vtkPolyhedron*, double tolerance);
  static State Check(vtkLagrangeCurve*, double tolerance);
  static State Check(vtkLagrangeTriangle*, double tolerance);
  static State Check(vtkLagrangeQuadrilateral*, double tolerance);
  static State Check(vtkLagrangeTetra*, double tolerance);
  static State Check(vtkLagrangeHexahedron*, double tolerance);
  static State Check(vtkLagrangeWedge*, double tolerance);
  static State Check(vtkBezierCurve*, double tolerance);
  static State Check(vtkBezierTriangle*, double tolerance);
  static State Check(vtkBezierQuadrilateral*, double tolerance);
  static State Check(vtkBezierTetra*, double tolerance);
  static State Check(vtkBezierHexahedron*, double tolerance);
  static State Check(vtkBezierWedge*, double tolerance);

  ///@{
  /**
   * Set/Get the tolerance. This value is used as an epsilon for floating point
   * equality checks throughout the cell checking process. The default value is
   * FLT_EPSILON.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

  ///@{
  /// Set/get whether to compute a per-cell tolerance that is a quarter of
  /// the length of the shortest non-degenerate edge.
  ///
  /// This setting is off by default. If enabled, the \a Tolerance ivar is ignored
  /// unless the cell has no edges (i.e., vertex cells) or all its edges have zero
  /// length – in which case \a Tolerance is used.
  ///
  /// This setting is independent of PlanarityTolerance.
  vtkSetMacro(AutoTolerance, vtkTypeBool);
  vtkGetMacro(AutoTolerance, vtkTypeBool);
  vtkBooleanMacro(AutoTolerance, vtkTypeBool);
  ///@}

  ///@{
  /// Set/get a planarity tolerance.
  ///
  /// This tolerance thresholds the ratio of the distance a planar polygonal
  /// cell (or cell face) protrudes out of its plane compared to the largest
  /// distance between a cell (or cell face) center and any of its corner points.
  /// It defaults to 0.1; any cells which protrude more than 10% of their radius
  /// out of the plane will be marked invalid.
  ///
  /// These methods are static so that calls to static Check() methods need not
  /// pass multiple tolerances and other validation parameters. This also means
  /// SetPlanarityTolerance is not thread-safe and should not be called when any
  /// other thread may be calling GetPlanarityTolerance().
  ///
  /// If the planarity tolerance is set to 0 or a negative value, planarity will
  /// not be tested.
  static void SetPlanarityTolerance(double tolerance);
  static double GetPlanarityTolerance();
  ///@}
protected:
  vtkCellValidator();
  ~vtkCellValidator() override = default;

  double Tolerance;
  vtkTypeBool AutoTolerance = false;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  static bool NoIntersectingEdges(vtkCell* cell, double tolerance);
  VTK_DEPRECATED_IN_9_6_0("Do not use or make NoIntersectingFacesStatus protected and use it.")
  static bool NoIntersectingFaces(vtkCell* cell, double tolerance);
  static bool ContiguousEdges(vtkCell* twoDimensionalCell, double tolerance);
  static State Convex(vtkCell* cell, double tolerance);
  static bool FacesAreOrientedCorrectly(vtkCell* threeDimensionalCell, double tolerance);

private:
  vtkCellValidator(const vtkCellValidator&) = delete;
  void operator=(const vtkCellValidator&) = delete;

  static State NoIntersectingFacesStatus(vtkCell* cell, double tolerance);
};

VTK_ABI_NAMESPACE_END
#endif
