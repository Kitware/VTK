/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSynchronizeTimeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSynchronizeTimeFilter
 * @brief   Set "close" time step values from the second input to the first
 *
 * Synchronize time step values in the first input to time step
 * values in the second input that are considered close enough.
 * The outputted data set is from the first input and the number of
 * output time steps is also equal to the number of time steps in
 * the first input. Time step values in the first input that are
 * "close" to time step values in the second input are replaced
 * with the value from the second input. Close is determined to
 * be if the difference is less than RelativeTolerance multiplied
 * by the time range of the first input.
 */

#ifndef vtkSynchronizeTimeFilter_h
#define vtkSynchronizeTimeFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include <vector> // Use of dynamically allocated array

class VTKFILTERSGENERAL_EXPORT vtkSynchronizeTimeFilter : public vtkPassInputTypeAlgorithm
{
public:
  static vtkSynchronizeTimeFilter* New();
  vtkTypeMacro(vtkSynchronizeTimeFilter, vtkPassInputTypeAlgorithm);

  /**
   * Specify the input that we may potentially replace time
   * steps with. SetInputConnection() should be used for providing the data
   * set that will actually be output from this filter.
   */
  void SetSourceConnection(vtkAlgorithmOutput* algOutput);

  /**
   * Set/get the relative tolerance for comparing time step
   * values to see if they are close enough to be considered
   * identical.
   */
  vtkSetClampMacro(RelativeTolerance, double, 0, VTK_DOUBLE_MAX);
  vtkGetMacro(RelativeTolerance, double);

protected:
  vtkSynchronizeTimeFilter();
  ~vtkSynchronizeTimeFilter() override;

  /**
   * Helper methods for getting the input time value or output time
   * value given the output time value or input time value, respectively.
   */
  double GetInputTimeValue(double outputTimeValue);
  double GetOutputTimeValue(double inputTimeValue);

  int RequestInformation(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestUpdateExtent(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

private:
  vtkSynchronizeTimeFilter(const vtkSynchronizeTimeFilter&) = delete;
  void operator=(const vtkSynchronizeTimeFilter&) = delete;

  /**
   * Copies of the time steps for both the input and
   * the output values.
   */
  std::vector<double> InputTimeStepValues;
  std::vector<double> OutputTimeStepValues;

  /**
   * The relative tolerance for comparing time step values to see if they
   * are close enough to be considered identical. The default value is 0.00001
   */
  double RelativeTolerance;
};

#endif
