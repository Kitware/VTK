// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLiutexCoreLines
 * @brief   Extract vortex core lines from the Liutex vector field
 *
 * A Liutex core line is a curve along which the gradient of the Liutex
 * magnitude R is parallel to the Liutex direction r, so that R has a critical
 * point in the plane perpendicular to the rotation axis. This filter computes
 * Liutex with vtkLiutex, takes the gradient of R with vtkGradientFilter, and
 * extracts the lines where grad(R) and the Liutex vector are parallel with the
 * parallel vectors operator. Only triangles whose three vertices have R above
 * MinimumLiutexMagnitude are searched, which removes lines through regions
 * without rotation and suppresses spurious lines in weak, noisy regions.
 *
 * The input array is a 3-component point velocity or a 9-component point
 * velocity gradient, as for vtkLiutex. The output polylines carry the input
 * point data and the Liutex arrays, interpolated to the line points. A
 * composite input gives a composite output of the same structure, with the
 * core lines of each block; a line that crosses a block boundary is split
 * there.
 *
 * Because the core line condition involves a derivative of R, which itself
 * comes from the velocity gradient, the result is sensitive to the accuracy of
 * the velocity gradient. Supplying the gradient computed by the solver helps.
 *
 * Reference:
 * Y. Gao, J. Liu, Y. Yu, C. Liu, "A Liutex based definition and identification
 * of vortex core center lines", Journal of Hydrodynamics 31, 445-454 (2019).
 *
 * @sa
 * vtkLiutex vtkParallelVectors vtkVortexCore
 */

#ifndef vtkLiutexCoreLines_h
#define vtkLiutexCoreLines_h

#include "vtkFiltersFlowPathsModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSFLOWPATHS_EXPORT vtkLiutexCoreLines : public vtkPolyDataAlgorithm
{
public:
  static vtkLiutexCoreLines* New();
  vtkTypeMacro(vtkLiutexCoreLines, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Line points are only searched where the Liutex magnitude of all three
   * vertices of a surface triangle exceeds this value, and are kept only where
   * the interpolated magnitude exceeds it. The default is 0.
   */
  vtkSetClampMacro(MinimumLiutexMagnitude, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MinimumLiutexMagnitude, double);
  ///@}

  ///@{
  /**
   * Passed to the gradient computations. The default is off.
   */
  vtkSetMacro(FasterApproximation, bool);
  vtkGetMacro(FasterApproximation, bool);
  vtkBooleanMacro(FasterApproximation, bool);
  ///@}

protected:
  vtkLiutexCoreLines();
  ~vtkLiutexCoreLines() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkLiutexCoreLines(const vtkLiutexCoreLines&) = delete;
  void operator=(const vtkLiutexCoreLines&) = delete;

  double MinimumLiutexMagnitude = 0.0;
  bool FasterApproximation = false;
};

VTK_ABI_NAMESPACE_END
#endif
