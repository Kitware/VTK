// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkmHistogramSampling
 * @brief   Accelerated point to cell interpolation filter.
 *
 * vtkmHistogramSampling is a filter that transforms point data (i.e., data
 * specified at cell points) into cell data (i.e., data specified per cell).
 * The method of transformation is based on averaging the data
 * values of all points used by particular cell. This filter will also
 * pass through any existing point and cell arrays.
 *
 */

#ifndef vtkmHistogramSampling_h
#define vtkmHistogramSampling_h

#include "vtkAcceleratorsVTKmFiltersModule.h" //required for correct implementation
#include "vtkDataSetAlgorithm.h"
#include "vtkmlib/vtkmInitializer.h" // Need for initializing viskores

VTK_ABI_NAMESPACE_BEGIN
class VTKACCELERATORSVTKMFILTERS_EXPORT vtkmHistogramSampling : public vtkDataSetAlgorithm
{
public:
  vtkTypeMacro(vtkmHistogramSampling, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkmHistogramSampling* New();

  ///@{
  /**
   * Specify value to set the sampling fraction.
   */
  vtkSetMacro(SampleFraction, double);
  vtkGetMacro(SampleFraction, double);
  ///@}

  ///@{
  /**
   * Set/get the desired number of histogram bins.
   */
  vtkSetMacro(NumberOfBins, int);
  vtkGetMacro(NumberOfBins, int);
  ///@}

protected:
  vtkmHistogramSampling(){};
  ~vtkmHistogramSampling() override = default;
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkmHistogramSampling(const vtkmHistogramSampling&) = delete;
  void operator=(const vtkmHistogramSampling&) = delete;
  vtkmInitializer Initializer;

  double SampleFraction = 0.1;
  size_t NumberOfBins = 10;
};

VTK_ABI_NAMESPACE_END
#endif // vtkmHistogramSampling_h
