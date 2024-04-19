// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageGaussianSmooth
 * @brief   Performs a gaussian convolution.
 *
 * vtkImageGaussianSmooth implements a convolution of the input image
 * with a gaussian. Supports from one to three dimensional convolutions.
 */

#ifndef vtkImageGaussianSmooth_h
#define vtkImageGaussianSmooth_h

#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkThreadedImageAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGGENERAL_EXPORT vtkImageGaussianSmooth : public vtkThreadedImageAlgorithm
{
public:
  vtkTypeMacro(vtkImageGaussianSmooth, vtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates an instance of vtkImageGaussianSmooth with the following
   * defaults: Dimensionality 3, StandardDeviations( 2, 2, 2),
   * Radius Factors ( 1.5, 1.5, 1.5)
   */
  static vtkImageGaussianSmooth* New();

  ///@{
  /**
   * Sets/Gets the Standard deviation of the gaussian in pixel units.
   */
  vtkSetVector3Macro(StandardDeviations, double);
  void SetStandardDeviation(double std) { this->SetStandardDeviations(std, std, std); }
  void SetStandardDeviations(double a, double b) { this->SetStandardDeviations(a, b, 0.0); }
  vtkGetVector3Macro(StandardDeviations, double);
  ///@}

  /**
   * Sets/Gets the Standard deviation of the gaussian in pixel units.
   * These methods are provided for compatibility with old scripts
   */
  void SetStandardDeviation(double a, double b) { this->SetStandardDeviations(a, b, 0.0); }
  void SetStandardDeviation(double a, double b, double c) { this->SetStandardDeviations(a, b, c); }

  ///@{
  /**
   * Sets/Gets the Radius Factors of the gaussian (no unit).
   * The radius factors determine how far out the gaussian kernel will
   * go before being clamped to zero.
   */
  vtkSetVector3Macro(RadiusFactors, double);
  void SetRadiusFactors(double f, double f2) { this->SetRadiusFactors(f, f2, 1.5); }
  void SetRadiusFactor(double f) { this->SetRadiusFactors(f, f, f); }
  vtkGetVector3Macro(RadiusFactors, double);
  ///@}

  ///@{
  /**
   * Set/Get the dimensionality of this filter. This determines whether
   * a one, two, or three dimensional gaussian is performed.
   */
  vtkSetMacro(Dimensionality, int);
  vtkGetMacro(Dimensionality, int);
  ///@}

protected:
  vtkImageGaussianSmooth();
  ~vtkImageGaussianSmooth() override;

  int Dimensionality;
  double StandardDeviations[3];
  double RadiusFactors[3];

  void ComputeKernel(double* kernel, int min, int max, double std);
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void InternalRequestUpdateExtent(int*, int*);
  void ExecuteAxis(int axis, vtkImageData* inData, int inExt[6], vtkImageData* outData,
    int outExt[6], int* pcycle, int target, int* pcount, int total, vtkInformation* inInfo);
  void ThreadedRequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector, vtkImageData*** inData, vtkImageData** outData,
    int outExt[6], int id) override;

private:
  vtkImageGaussianSmooth(const vtkImageGaussianSmooth&) = delete;
  void operator=(const vtkImageGaussianSmooth&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
