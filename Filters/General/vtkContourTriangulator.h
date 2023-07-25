// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkContourTriangulator
 * @brief   Fill all 2D contours to create polygons
 *
 * vtkContourTriangulator will generate triangles to fill all of the 2D
 * contours in its input. The input to the filter is a set of lines (not
 * polylines) which when joined form loops. The contours may be concave, and
 * may even contain holes i.e. a contour may contain an internal contour that
 * is wound in the opposite direction (as compared to the outer polygon
 * normal) to indicate that it is a hole.
 *
 * @warning
 * The triangulation of is done in O(n) time for simple convex
 * inputs, but for non-convex inputs the worst-case time is O(n^2*m^2)
 * where n is the number of points and m is the number of holes.
 * The best triangulation algorithms, in contrast, are O(n log n).
 * The resulting triangles may be quite narrow, the algorithm does
 * not attempt to produce high-quality triangles.
 *
 * @sa
 * vtkClipClosedSurface vtkPolygon
 *
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
 */

#ifndef vtkContourTriangulator_h
#define vtkContourTriangulator_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkIdList;

class VTKFILTERSGENERAL_EXPORT vtkContourTriangulator : public vtkPolyDataAlgorithm
{
public:
  ///@{
  /**
   * Standard methods for instantiation, obtaining type information, and printing.
   */
  static vtkContourTriangulator* New();
  vtkTypeMacro(vtkContourTriangulator, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Check if there was a triangulation failure in the last update.
   */
  vtkGetMacro(TriangulationError, int);
  ///@}

  ///@{
  /**
   * Generate errors when the triangulation fails.  Note that triangulation
   * failures are often minor, because they involve tiny triangles that are
   * too small to see.
   */
  vtkSetMacro(TriangulationErrorDisplay, vtkTypeBool);
  vtkBooleanMacro(TriangulationErrorDisplay, vtkTypeBool);
  vtkGetMacro(TriangulationErrorDisplay, vtkTypeBool);
  ///@}

  /**
   * A robust method for triangulating a polygon.  It cleans up the polygon
   * and then applies the ear-cut triangulation.  A zero return value
   * indicates that triangulation failed.
   */
  static int TriangulatePolygon(vtkIdList* polygon, vtkPoints* points, vtkCellArray* triangles);

  /**
   * Given some closed contour lines, create a triangle mesh that fills
   * those lines.  The input lines do not have to be in tail-to-tip order.
   * Only numLines starting from firstLine will be used.  Note that holes
   * can be indicated by contour loops whose normals are in the opposite
   * direction to the provided normal.
   */
  static int TriangulateContours(vtkPolyData* data, vtkIdType firstLine, vtkIdType numLines,
    vtkCellArray* outputPolys, const double normal[3], vtkPolyDataAlgorithm* self = nullptr);

protected:
  vtkContourTriangulator();
  ~vtkContourTriangulator() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int TriangulationError;
  vtkTypeBool TriangulationErrorDisplay;

private:
  vtkContourTriangulator(const vtkContourTriangulator&) = delete;
  void operator=(const vtkContourTriangulator&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
