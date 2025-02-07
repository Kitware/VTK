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
 *
 * @sa
 * vtkCellQuality
 */

#ifndef vtkCellValidator_h
#define vtkCellValidator_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro

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

  // Description:
  // Construct to compute the validity of cells.
  static vtkCellValidator* New();

  enum State : short
  {
    Valid = 0x0,
    WrongNumberOfPoints = 0x01,
    IntersectingEdges = 0x02,
    IntersectingFaces = 0x04,
    NoncontiguousEdges = 0x08,
    Nonconvex = 0x10,
    FacesAreOrientedIncorrectly = 0x20,
  };

  friend State operator&(State a, State b)
  {
    return static_cast<State>(static_cast<short>(a) & static_cast<short>(b));
  }
  friend State operator|(State a, State b)
  {
    return static_cast<State>(static_cast<short>(a) | static_cast<short>(b));
  }
  friend State& operator&=(State& a, State b) { return a = a & b; }

  friend State& operator|=(State& a, State b) { return a = a | b; }

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

protected:
  vtkCellValidator();
  ~vtkCellValidator() override = default;

  double Tolerance;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  static bool NoIntersectingEdges(vtkCell* cell, double tolerance);
  static bool NoIntersectingFaces(vtkCell* cell, double tolerance);
  static bool ContiguousEdges(vtkCell* twoDimensionalCell, double tolerance);
  static bool Convex(vtkCell* cell, double tolerance);
  static bool FacesAreOrientedCorrectly(vtkCell* threeDimensionalCell, double tolerance);

private:
  vtkCellValidator(const vtkCellValidator&) = delete;
  void operator=(const vtkCellValidator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
