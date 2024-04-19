// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageSeparableConvolution
 * @brief    3 1D convolutions on an image
 *
 * vtkImageSeparableConvolution performs a convolution along the X, Y,
 * and Z axes of an image, based on the three different 1D convolution
 * kernels.  The kernels must be of odd size, and are considered to be
 * centered at (int)((kernelsize - 1) / 2.0 ).  If a kernel is nullptr,
 * that dimension is skipped.  This filter is designed to efficiently
 * convolve separable filters that can be decomposed into 1 or more 1D
 * convolutions.  It also handles arbitrarily large kernel sizes, and
 * uses edge replication to handle boundaries.
 */

#ifndef vtkImageSeparableConvolution_h
#define vtkImageSeparableConvolution_h

#include "vtkImageDecomposeFilter.h"
#include "vtkImagingGeneralModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkFloatArray;

class VTKIMAGINGGENERAL_EXPORT vtkImageSeparableConvolution : public vtkImageDecomposeFilter
{
public:
  static vtkImageSeparableConvolution* New();
  vtkTypeMacro(vtkImageSeparableConvolution, vtkImageDecomposeFilter);

  // Set the X convolution kernel, a null value indicates no convolution to
  // be done.  The kernel must be of odd length
  virtual void SetXKernel(vtkFloatArray*);
  vtkGetObjectMacro(XKernel, vtkFloatArray);

  // Set the Y convolution kernel, a null value indicates no convolution to
  // be done The kernel must be of odd length
  virtual void SetYKernel(vtkFloatArray*);
  vtkGetObjectMacro(YKernel, vtkFloatArray);

  // Set the Z convolution kernel, a null value indicates no convolution to
  // be done The kernel must be of odd length
  virtual void SetZKernel(vtkFloatArray*);
  vtkGetObjectMacro(ZKernel, vtkFloatArray);

  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overload standard modified time function. If kernel arrays are modified,
   * then this object is modified as well.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkImageSeparableConvolution();
  ~vtkImageSeparableConvolution() override;

  vtkFloatArray* XKernel;
  vtkFloatArray* YKernel;
  vtkFloatArray* ZKernel;

  int IterativeRequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int IterativeRequestInformation(vtkInformation* in, vtkInformation* out) override;
  int IterativeRequestUpdateExtent(vtkInformation* in, vtkInformation* out) override;

private:
  vtkImageSeparableConvolution(const vtkImageSeparableConvolution&) = delete;
  void operator=(const vtkImageSeparableConvolution&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
