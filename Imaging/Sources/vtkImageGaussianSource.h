// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageGaussianSource
 * @brief   Create an image with Gaussian pixel values.
 *
 * vtkImageGaussianSource just produces images with pixel values determined
 * by a Gaussian.
 */

#ifndef vtkImageGaussianSource_h
#define vtkImageGaussianSource_h

#include "vtkImageAlgorithm.h"
#include "vtkImagingSourcesModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGSOURCES_EXPORT vtkImageGaussianSource : public vtkImageAlgorithm
{
public:
  static vtkImageGaussianSource* New();
  vtkTypeMacro(vtkImageGaussianSource, vtkImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set/Get the extent of the whole output image.
   */
  void SetWholeExtent(int xMinx, int xMax, int yMin, int yMax, int zMin, int zMax);

  ///@{
  /**
   * Set/Get the center of the Gaussian.
   */
  vtkSetVector3Macro(Center, double);
  vtkGetVector3Macro(Center, double);
  ///@}

  ///@{
  /**
   * Set/Get the Maximum value of the gaussian
   */
  vtkSetMacro(Maximum, double);
  vtkGetMacro(Maximum, double);
  ///@}

  ///@{
  /**
   * Set/Get the standard deviation of the gaussian
   */
  vtkSetMacro(StandardDeviation, double);
  vtkGetMacro(StandardDeviation, double);
  ///@}

protected:
  vtkImageGaussianSource();
  ~vtkImageGaussianSource() override = default;

  double StandardDeviation;
  int WholeExtent[6];
  double Center[3];
  double Maximum;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkImageGaussianSource(const vtkImageGaussianSource&) = delete;
  void operator=(const vtkImageGaussianSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
