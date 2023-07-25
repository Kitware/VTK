// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractEnclosedPoints
 * @brief   extract points inside of a closed polygonal surface
 *
 * vtkExtractEnclosedPoints is a filter that evaluates all the input points
 * to determine whether they are contained within an enclosing surface. Those
 * within the surface are sent to the output. The enclosing surface is
 * specified through a second input to the filter.
 *
 * Note: as a derived class of vtkPointCloudFilter, additional methods are
 * available for generating an in/out mask, and also extracting points
 * outside of the enclosing surface.
 *
 * @warning
 * The filter assumes that the surface is closed and manifold. A boolean flag
 * can be set to force the filter to first check whether this is true. If false,
 * all points will be marked outside. Note that if this check is not performed
 * and the surface is not closed, the results are undefined.
 *
 * @warning
 * This class has been threaded with vtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * VTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @warning
 * The filter vtkSelectEnclosedPoints marks points as to in/out of the
 * enclosing surface, and operates on any dataset type, producing an output
 * dataset of the same type as the input. Then, thresholding and masking
 * filters can be used to extract parts of the dataset. This filter
 * (vtkExtractEnclosedPoints) is meant to operate on point clouds represented
 * by vtkPolyData, and produces vtkPolyData on output, so it is more
 * efficient for point processing. Note that this filter delegates many of
 * its methods to vtkSelectEnclosedPoints.
 *
 * @sa
 * vtkSelectEnclosedPoints vtkExtractPoints
 */

#ifndef vtkExtractEnclosedPoints_h
#define vtkExtractEnclosedPoints_h

#include "vtkFiltersPointsModule.h" // For export macro
#include "vtkPointCloudFilter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSPOINTS_EXPORT vtkExtractEnclosedPoints : public vtkPointCloudFilter
{
public:
  ///@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  static vtkExtractEnclosedPoints* New();
  vtkTypeMacro(vtkExtractEnclosedPoints, vtkPointCloudFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set the surface to be used to test for containment. Two methods are
   * provided: one directly for vtkPolyData, and one for the output of a
   * filter.
   */
  void SetSurfaceData(vtkPolyData* pd);
  void SetSurfaceConnection(vtkAlgorithmOutput* algOutput);
  ///@}

  ///@{
  /**
   * Return a pointer to the enclosing surface.
   */
  vtkPolyData* GetSurface();
  vtkPolyData* GetSurface(vtkInformationVector* sourceInfo);
  ///@}

  ///@{
  /**
   * Specify whether to check the surface for closure. If on, then the
   * algorithm first checks to see if the surface is closed and manifold.
   */
  vtkSetMacro(CheckSurface, vtkTypeBool);
  vtkBooleanMacro(CheckSurface, vtkTypeBool);
  vtkGetMacro(CheckSurface, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Specify the tolerance on the intersection. The tolerance is expressed as
   * a fraction of the diagonal of the bounding box of the enclosing surface.
   */
  vtkSetClampMacro(Tolerance, double, 0.0, VTK_FLOAT_MAX);
  vtkGetMacro(Tolerance, double);
  ///@}

protected:
  vtkExtractEnclosedPoints();
  ~vtkExtractEnclosedPoints() override;

  vtkTypeBool CheckSurface;
  double Tolerance;

  // Internal structures for managing the intersection testing
  vtkPolyData* Surface;

  // Satisfy vtkPointCloudFilter superclass API
  int FilterPoints(vtkPointSet* input) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkExtractEnclosedPoints(const vtkExtractEnclosedPoints&) = delete;
  void operator=(const vtkExtractEnclosedPoints&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
