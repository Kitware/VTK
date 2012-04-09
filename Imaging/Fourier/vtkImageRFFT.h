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
// .NAME vtkImageRFFT -  Reverse Fast Fourier Transform.
// .SECTION Description
// vtkImageRFFT implements the reverse fast Fourier transform.  The input
// can have real or complex data in any components and data types, but
// the output is always complex doubles with real values in component0, and
// imaginary values in component1.  The filter is fastest for images that
// have power of two sizes.  The filter uses a butterfly fitlers for each
// prime factor of the dimension.  This makes images with prime number dimensions
// (i.e. 17x17) much slower to compute.  Multi dimensional (i.e volumes)
// FFT's are decomposed so that each axis executes in series.
// In most cases the RFFT will produce an image whose imaginary values are all
// zero's. In this case vtkImageExtractComponents can be used to remove
// this imaginary components leaving only the real image.

// .SECTION See Also
// vtkImageExtractComponenents



#ifndef __vtkImageRFFT_h
#define __vtkImageRFFT_h


#include "vtkImagingFourierModule.h" // For export macro
#include "vtkImageFourierFilter.h"

class VTKIMAGINGFOURIER_EXPORT vtkImageRFFT : public vtkImageFourierFilter
{
public:
  static vtkImageRFFT *New();
  vtkTypeMacro(vtkImageRFFT,vtkImageFourierFilter);


  // Description:
  // For streaming and threads.  Splits output update extent into num pieces.
  // This method needs to be called num times.  Results must not overlap for
  // consistent starting extent.  Subclass can override this method.  This
  // method returns the number of pieces resulting from a successful split.
  // This can be from 1 to "total".  If 1 is returned, the extent cannot be
  // split.
  int SplitExtent(int splitExt[6], int startExt[6],
                  int num, int total);

protected:
  vtkImageRFFT() {};
  ~vtkImageRFFT() {};

  virtual int IterativeRequestInformation(vtkInformation* in,
                                          vtkInformation* out);
  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);

  virtual void ThreadedRequestData(
    vtkInformation* vtkNotUsed( request ),
    vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed( outputVector ),
    vtkImageData ***inDataVec,
    vtkImageData **outDataVec,
    int outExt[6],
    int threadId);
private:
  vtkImageRFFT(const vtkImageRFFT&);  // Not implemented.
  void operator=(const vtkImageRFFT&);  // Not implemented.
};

#endif










// VTK-HeaderTest-Exclude: vtkImageRFFT.h
