/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFourierCenter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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
// Input and output are assumed to be floats.

#ifndef __vtkImageFourierCenter_h
#define __vtkImageFourierCenter_h


#include "vtkImageDecomposeFilter.h"

class VTK_IMAGING_EXPORT vtkImageFourierCenter : public vtkImageDecomposeFilter
{
public:
  static vtkImageFourierCenter *New();
  vtkTypeRevisionMacro(vtkImageFourierCenter,vtkImageDecomposeFilter);

  // Description:
  // This is an internal method that should not be called by the user.
  virtual void IterativeExecuteData(vtkImageData *in, vtkImageData *out) {
    this->MultiThread(in, out); };
  
protected:
  vtkImageFourierCenter();
  ~vtkImageFourierCenter() {};

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int outExt[6], int threadId);
private:
  vtkImageFourierCenter(const vtkImageFourierCenter&);  // Not implemented.
  void operator=(const vtkImageFourierCenter&);  // Not implemented.
};

#endif










