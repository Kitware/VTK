/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierCenter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageFourierCenter - Shifts constant frequency to center for
// display.
// .SECTION Description
// Is used for dispaying images in frequency space.  FFT converts spatial
// images into frequency space, but puts the zero frequency at the origin.
// This filter shifts the zero frequency to the center of the image.
// Input and output are assumed to be doubles.

#ifndef __vtkImageFourierCenter_h
#define __vtkImageFourierCenter_h


#include "vtkImagingFourierModule.h" // For export macro
#include "vtkImageDecomposeFilter.h"

class VTKIMAGINGFOURIER_EXPORT vtkImageFourierCenter : public vtkImageDecomposeFilter
{
public:
  static vtkImageFourierCenter *New();
  vtkTypeMacro(vtkImageFourierCenter,vtkImageDecomposeFilter);

protected:
  vtkImageFourierCenter();
  ~vtkImageFourierCenter() {};

  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);

  virtual void ThreadedRequestData(
    vtkInformation* vtkNotUsed( request ),
    vtkInformationVector** vtkNotUsed( inputVector ),
    vtkInformationVector* outputVector,
    vtkImageData ***inDataVec,
    vtkImageData **outDataVec,
    int outExt[6],
    int threadId);
private:
  vtkImageFourierCenter(const vtkImageFourierCenter&);  // Not implemented.
  void operator=(const vtkImageFourierCenter&);  // Not implemented.
};

#endif










// VTK-HeaderTest-Exclude: vtkImageFourierCenter.h
