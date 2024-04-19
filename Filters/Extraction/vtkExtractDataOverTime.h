// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkExtractDataOverTime
 * @brief   extract point data from a time sequence for
 * a specified point id.
 *
 * This filter extracts the point data from a time sequence and specified index
 * and creates an output of the same type as the input but with Points
 * containing "number of time steps" points; the point and PointData
 * corresponding to the PointIndex are extracted at each time step and added to
 * the output.  A PointData array is added called "Time" (or "TimeData" if
 * there is already an array called "Time"), which is the time at each index.
 */

#ifndef vtkExtractDataOverTime_h
#define vtkExtractDataOverTime_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkPointSetAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSEXTRACTION_EXPORT vtkExtractDataOverTime : public vtkPointSetAlgorithm
{
public:
  static vtkExtractDataOverTime* New();
  vtkTypeMacro(vtkExtractDataOverTime, vtkPointSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Index of point to extract at each time step
   */
  vtkSetMacro(PointIndex, int);
  vtkGetMacro(PointIndex, int);
  ///@}

  ///@{
  /**
   * Get the number of time steps
   */
  vtkGetMacro(NumberOfTimeSteps, int);
  ///@}

protected:
  vtkExtractDataOverTime();
  ~vtkExtractDataOverTime() override = default;

  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int AllocateOutputData(vtkPointSet* input, vtkPointSet* output);

  int PointIndex;
  int CurrentTimeIndex;
  int NumberOfTimeSteps;

private:
  vtkExtractDataOverTime(const vtkExtractDataOverTime&) = delete;
  void operator=(const vtkExtractDataOverTime&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
