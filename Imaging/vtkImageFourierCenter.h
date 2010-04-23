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


#include "vtkImageDecomposeFilter.h"

class VTK_IMAGING_EXPORT vtkImageFourierCenter : public vtkImageDecomposeFilter
{
public:
  static vtkImageFourierCenter *New();
  vtkTypeMacro(vtkImageFourierCenter,vtkImageDecomposeFilter);

protected:
  vtkImageFourierCenter();
  ~vtkImageFourierCenter() {};

  virtual int IterativeRequestUpdateExtent(vtkInformation* in,
                                           vtkInformation* out);

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int threadId);
private:
  vtkImageFourierCenter(const vtkImageFourierCenter&);  // Not implemented.
  void operator=(const vtkImageFourierCenter&);  // Not implemented.
};

#endif










