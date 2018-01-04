/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageRFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImageRFFT
 * @brief    Reverse Fast Fourier Transform.
 *
 * vtkImageRFFT implements the reverse fast Fourier transform.  The input
 * can have real or complex data in any components and data types, but
 * the output is always complex doubles with real values in component0, and
 * imaginary values in component1.  The filter is fastest for images that
 * have power of two sizes.  The filter uses butterfly filters for each
 * prime factor of the dimension.  This makes images with prime number dimensions
 * (i.e. 17x17) much slower to compute.  Multi dimensional (i.e volumes)
 * FFT's are decomposed so that each axis executes in series.
 * In most cases the RFFT will produce an image whose imaginary values are all
 * zero's. In this case vtkImageExtractComponents can be used to remove
 * this imaginary components leaving only the real image.
 *
 * @sa
 * vtkImageExtractComponenents
*/

#ifndef vtkImageRFFT_h
#define vtkImageRFFT_h


#include "vtkImagingFourierModule.h" // For export macro
#include "vtkImageFourierFilter.h"

class VTKIMAGINGFOURIER_EXPORT vtkImageRFFT : public vtkImageFourierFilter
{
public:
  static vtkImageRFFT *New();
  vtkTypeMacro(vtkImageRFFT,vtkImageFourierFilter);

protected:
  vtkImageRFFT() {}
  ~vtkImageRFFT() override {}

  int IterativeRequestInformation(vtkInformation* in,
                                          vtkInformation* out) override;
  int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out) override;

  void ThreadedRequestData(
    vtkInformation* vtkNotUsed( request ),
    vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed( outputVector ),
    vtkImageData ***inDataVec,
    vtkImageData **outDataVec,
    int outExt[6],
    int threadId) override;
private:
  vtkImageRFFT(const vtkImageRFFT&) = delete;
  void operator=(const vtkImageRFFT&) = delete;
};

#endif










// VTK-HeaderTest-Exclude: vtkImageRFFT.h
