/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMirrorPad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageMirrorPad - Extra pixels are filled by mirror images.
// .SECTION Description
// vtkImageMirrorPad makes an image larger by filling extra pixels with
// a mirror image of the original image (mirror at image boundaries).


#ifndef __vtkImageMirrorPad_h
#define __vtkImageMirrorPad_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImagePadFilter.h"

class VTKIMAGINGCORE_EXPORT vtkImageMirrorPad : public vtkImagePadFilter
{
public:
  static vtkImageMirrorPad *New();
  vtkTypeMacro(vtkImageMirrorPad,vtkImagePadFilter);

protected:
  vtkImageMirrorPad() {}
  ~vtkImageMirrorPad() {}

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int wExt[6]);
  void ThreadedRequestData (vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector,
                            vtkImageData ***inData, vtkImageData **outData,
                            int ext[6], int id);
private:
  vtkImageMirrorPad(const vtkImageMirrorPad&);  // Not implemented.
  void operator=(const vtkImageMirrorPad&);  // Not implemented.
};

#endif



// VTK-HeaderTest-Exclude: vtkImageMirrorPad.h
