// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQuantizePolyDataPoints
 * @brief   quantizes x,y,z coordinates of points
 *
 * vtkQuantizePolyDataPoints is a subclass of vtkCleanPolyData and
 * inherits the functionality of vtkCleanPolyData with the addition that
 * it quantizes the point coordinates before inserting into the point list.
 * The user should set QFactor to a positive value (0.25 by default) and all
 * {x,y,z} coordinates will be quantized to that grain size.
 *
 * A tolerance of zero is expected, though positive values may be used, the
 * quantization will take place before the tolerance is applied.
 *
 * @warning
 * Merging points can alter topology, including introducing non-manifold
 * forms. Handling of degenerate cells is controlled by switches in
 * vtkCleanPolyData.
 *
 * @warning
 * If you wish to operate on a set of coordinates that has no cells, you must
 * add a vtkPolyVertex cell with all of the points to the PolyData
 * (or use a vtkVertexGlyphFilter) before using the vtkCleanPolyData filter.
 *
 * @sa
 * vtkCleanPolyData
 */

#ifndef vtkQuantizePolyDataPoints_h
#define vtkQuantizePolyDataPoints_h

#include "vtkCleanPolyData.h"
#include "vtkFiltersGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkQuantizePolyDataPoints : public vtkCleanPolyData
{
public:
  static vtkQuantizePolyDataPoints* New();
  vtkTypeMacro(vtkQuantizePolyDataPoints, vtkCleanPolyData);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify quantization grain size. Default is 0.25
   */
  vtkSetClampMacro(QFactor, double, 1E-5, VTK_FLOAT_MAX);
  vtkGetMacro(QFactor, double);
  ///@}

  /**
   * Perform quantization on a point
   */
  void OperateOnPoint(double in[3], double out[3]) override;

  /**
   * Perform quantization on bounds
   */
  void OperateOnBounds(double in[6], double out[6]) override;

protected:
  vtkQuantizePolyDataPoints();
  ~vtkQuantizePolyDataPoints() override = default;

  double QFactor;

private:
  vtkQuantizePolyDataPoints(const vtkQuantizePolyDataPoints&) = delete;
  void operator=(const vtkQuantizePolyDataPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
