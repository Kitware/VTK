// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkSurfaceReconstructionFilter
 * @brief   reconstructs a surface from unorganized points
 *
 * vtkSurfaceReconstructionFilter takes a list of points assumed to lie on
 * the surface of a solid 3D object. A signed measure of the distance to the
 * surface is computed and sampled on a regular grid. The grid can then be
 * contoured at zero to extract the surface. The default values for
 * neighborhood size and sample spacing should give reasonable results for
 * most uses but can be set if desired. This procedure is based on the PhD
 * work of Hugues Hoppe: http://www.research.microsoft.com/~hoppe
 */

#ifndef vtkSurfaceReconstructionFilter_h
#define vtkSurfaceReconstructionFilter_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingHybridModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGHYBRID_EXPORT vtkSurfaceReconstructionFilter : public vtkImageAlgorithm
{
public:
  vtkTypeMacro(vtkSurfaceReconstructionFilter, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct with NeighborhoodSize=20.
   */
  static vtkSurfaceReconstructionFilter* New();

  ///@{
  /**
   * Specify the number of neighbors each point has, used for estimating the
   * local surface orientation.  The default value of 20 should be OK for
   * most applications, higher values can be specified if the spread of
   * points is uneven. Values as low as 10 may yield adequate results for
   * some surfaces. Higher values cause the algorithm to take longer. Higher
   * values will cause errors on sharp boundaries.
   */
  vtkGetMacro(NeighborhoodSize, int);
  vtkSetMacro(NeighborhoodSize, int);
  ///@}

  ///@{
  /**
   * Specify the spacing of the 3D sampling grid. If not set, a
   * reasonable guess will be made.
   */
  vtkGetMacro(SampleSpacing, double);
  vtkSetMacro(SampleSpacing, double);
  ///@}

protected:
  vtkSurfaceReconstructionFilter();
  ~vtkSurfaceReconstructionFilter() override = default;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int NeighborhoodSize;
  double SampleSpacing;

  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkSurfaceReconstructionFilter(const vtkSurfaceReconstructionFilter&) = delete;
  void operator=(const vtkSurfaceReconstructionFilter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
