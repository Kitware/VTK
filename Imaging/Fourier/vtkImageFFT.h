// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkImageFFT
 * @brief    Fast Fourier Transform.
 *
 * vtkImageFFT implements a fast Fourier transform.  The input
 * can have real or complex data in any components and data types, but
 * the output is always complex doubles with real values in component0, and
 * imaginary values in component1.  The filter is fastest for images that
 * have power of two sizes.  The filter uses a butterfly diagram for each
 * prime factor of the dimension.  This makes images with prime number dimensions
 * (i.e. 17x17) much slower to compute.  Multi dimensional (i.e volumes)
 * FFT's are decomposed so that each axis executes serially.
 */

#ifndef vtkImageFFT_h
#define vtkImageFFT_h

#include "vtkImageFourierFilter.h"
#include "vtkImagingFourierModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIMAGINGFOURIER_EXPORT vtkImageFFT : public vtkImageFourierFilter
{
public:
  static vtkImageFFT* New();
  vtkTypeMacro(vtkImageFFT, vtkImageFourierFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkImageFFT() = default;
  ~vtkImageFFT() override = default;

  int IterativeRequestInformation(vtkInformation* in, vtkInformation* out) override;
  int IterativeRequestUpdateExtent(vtkInformation* in, vtkInformation* out) override;

  void ThreadedRequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed(outputVector), vtkImageData*** inDataVec,
    vtkImageData** outDataVec, int outExt[6], int threadId) override;

private:
  vtkImageFFT(const vtkImageFFT&) = delete;
  void operator=(const vtkImageFFT&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
