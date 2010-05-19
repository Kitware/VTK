/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFFT.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageFFT -  Fast Fourier Transform.
// .SECTION Description
// vtkImageFFT implements a  fast Fourier transform.  The input
// can have real or complex data in any components and data types, but
// the output is always complex doubles with real values in component0, and
// imaginary values in component1.  The filter is fastest for images that
// have power of two sizes.  The filter uses a butterfly fitlers for each
// prime factor of the dimension.  This makes images with prime number dimensions 
// (i.e. 17x17) much slower to compute.  Multi dimensional (i.e volumes) 
// FFT's are decomposed so that each axis executes in series.


#ifndef __vtkImageFFT_h
#define __vtkImageFFT_h


#include "vtkImageFourierFilter.h"

class VTK_IMAGING_EXPORT vtkImageFFT : public vtkImageFourierFilter
{
public:
  static vtkImageFFT *New();
  vtkTypeMacro(vtkImageFFT,vtkImageFourierFilter);


  // Description:
  // Used internally for streaming and threads.  
  // Splits output update extent into num pieces.
  // This method needs to be called num times.  Results must not overlap for
  // consistent starting extent.  Subclass can override this method.
  // This method returns the number of pieces resulting from a
  // successful split.  This can be from 1 to "total".  
  // If 1 is returned, the extent cannot be split.
  int SplitExtent(int splitExt[6], int startExt[6], 
                  int num, int total);

protected:
  vtkImageFFT() {};
  ~vtkImageFFT() {};

  virtual int IterativeRequestInformation(vtkInformation* in,
                                          vtkInformation* out);
  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);
  
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int threadId);
private:
  vtkImageFFT(const vtkImageFFT&);  // Not implemented.
  void operator=(const vtkImageFFT&);  // Not implemented.
};

#endif










