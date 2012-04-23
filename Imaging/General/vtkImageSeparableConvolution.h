/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeparableConvolution.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageSeparableConvolution -  3 1D convolutions on an image
// .SECTION Description
// vtkImageSeparableConvolution performs a convolution along the X, Y,
// and Z axes of an image, based on the three different 1D convolution
// kernels.  The kernels must be of odd size, and are considered to be
// centered at (int)((kernelsize - 1) / 2.0 ).  If a kernel is NULL,
// that dimension is skipped.  This filter is designed to efficiently
// convolve separable filters that can be decomposed into 1 or more 1D
// convolutions.  It also handles arbitrarly large kernel sizes, and
// uses edge replication to handle boundaries.

#ifndef __vtkImageSeparableConvolution_h
#define __vtkImageSeparableConvolution_h


#include "vtkImagingGeneralModule.h" // For export macro
#include "vtkImageDecomposeFilter.h"

class vtkFloatArray;

class VTKIMAGINGGENERAL_EXPORT vtkImageSeparableConvolution : public vtkImageDecomposeFilter
{
public:
  static vtkImageSeparableConvolution *New();
  vtkTypeMacro(vtkImageSeparableConvolution,vtkImageDecomposeFilter);


  // Set the X convolution kernel, a null value indicates no convolution to
  // be done.  The kernel must be of odd length
  virtual void SetXKernel(vtkFloatArray*);
  vtkGetObjectMacro ( XKernel, vtkFloatArray );

  // Set the Y convolution kernel, a null value indicates no convolution to
  // be done The kernel must be of odd length
  virtual void SetYKernel(vtkFloatArray*);
  vtkGetObjectMacro ( YKernel, vtkFloatArray );

  // Set the Z convolution kernel, a null value indicates no convolution to
  // be done The kernel must be of odd length
  virtual void SetZKernel(vtkFloatArray*);
  vtkGetObjectMacro ( ZKernel, vtkFloatArray );

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Overload standard modified time function. If kernel arrays are modified,
  // then this object is modified as well.
  unsigned long int GetMTime();

protected:
  vtkImageSeparableConvolution();
  ~vtkImageSeparableConvolution();

  vtkFloatArray* XKernel;
  vtkFloatArray* YKernel;
  vtkFloatArray* ZKernel;

  virtual int IterativeRequestData(vtkInformation*,
                                   vtkInformationVector**,
                                   vtkInformationVector*);

  virtual int IterativeRequestInformation(vtkInformation* in,
                                          vtkInformation* out);
  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);

private:
  vtkImageSeparableConvolution(const vtkImageSeparableConvolution&);  // Not implemented.
  void operator=(const vtkImageSeparableConvolution&);  // Not implemented.
};

#endif










