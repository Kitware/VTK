/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencil.h
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
// .NAME vtkImageStencil - combine images via a cookie-cutter operation
// .SECTION Description
// vtkImageStencil will combine two images together using a stencil.
// The stencil should be provided in the form of a vtkImageStencilData,


#ifndef __vtkImageStencil_h
#define __vtkImageStencil_h

#include "vtkImageToImageFilter.h"
#include "vtkImageStencilData.h"

class VTK_IMAGING_EXPORT vtkImageStencil : public vtkImageToImageFilter
{
public:
  static vtkImageStencil *New();
  vtkTypeRevisionMacro(vtkImageStencil, vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the stencil to use.  The stencil can be created
  // from a vtkImplicitFunction or a vtkPolyData.
  virtual void SetStencil(vtkImageStencilData *stencil);
  vtkImageStencilData *GetStencil();

  // Description:
  // Reverse the stencil.
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);

  // Description:
  // NOTE: Not yet implemented, use SetBackgroundValue instead.
  // Set the second input.  This image will be used for the 'outside' of the
  // stencil.  If not set, the output voxels will be filled with
  // BackgroundValue instead.
  virtual void SetBackgroundInput(vtkImageData *input);
  vtkImageData *GetBackgroundInput();

  // Description:
  // Set the default output value to use when the second input is not set.
  void SetBackgroundValue(float val) {
    this->SetBackgroundColor(val,val,val,val); };
  float GetBackgroundValue() {
    return this->BackgroundColor[0]; };

  // Description:
  // Set the default color to use when the second input is not set.
  // This is like SetBackgroundValue, but for multi-component images.
  vtkSetVector4Macro(BackgroundColor, float);
  vtkGetVector4Macro(BackgroundColor, float);

protected:
  vtkImageStencil();
  ~vtkImageStencil();

  void ExecuteInformation() {
    this->vtkImageToImageFilter::ExecuteInformation(); };
  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);

  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
                       int extent[6], int id);
  
  int ReverseStencil;
  float BackgroundColor[4];
private:
  vtkImageStencil(const vtkImageStencil&);  // Not implemented.
  void operator=(const vtkImageStencil&);  // Not implemented.
};

#endif













