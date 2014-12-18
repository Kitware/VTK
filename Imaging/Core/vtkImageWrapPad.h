/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWrapPad.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageWrapPad - Makes an image larger by wrapping existing data.
// .SECTION Description
// vtkImageWrapPad performs a modulo operation on the output pixel index
// to determine the source input index.  The new image extent of the
// output has to be specified.  Input has to be the same scalar type as
// output.


#ifndef vtkImageWrapPad_h
#define vtkImageWrapPad_h


#include "vtkImagingCoreModule.h" // For export macro
#include "vtkImagePadFilter.h"

class vtkInformation;
class vtkInformationVector;

class VTKIMAGINGCORE_EXPORT vtkImageWrapPad : public vtkImagePadFilter
{
public:
  static vtkImageWrapPad *New();
  vtkTypeMacro(vtkImageWrapPad,vtkImagePadFilter);

protected:
  vtkImageWrapPad() {}
  ~vtkImageWrapPad() {}

  void ComputeInputUpdateExtent (int inExt[6], int outExt[6], int wExt[6]);
  void ThreadedRequestData (vtkInformation* request,
                            vtkInformationVector** inputVector,
                            vtkInformationVector* outputVector,
                            vtkImageData ***inData, vtkImageData **outData,
                            int ext[6], int id);
private:
  vtkImageWrapPad(const vtkImageWrapPad&);  // Not implemented.
  void operator=(const vtkImageWrapPad&);  // Not implemented.
};

#endif



// VTK-HeaderTest-Exclude: vtkImageWrapPad.h
