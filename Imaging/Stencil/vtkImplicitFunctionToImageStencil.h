/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionToImageStencil.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImplicitFunctionToImageStencil - clip an image with a function
// .SECTION Description
// vtkImplicitFunctionToImageStencil will convert a vtkImplicitFunction into
// a stencil that can be used with vtkImageStencil or with other classes
// that apply a stencil to an image.
// .SECTION see also
// vtkImplicitFunction vtkImageStencil vtkPolyDataToImageStencil

#ifndef vtkImplicitFunctionToImageStencil_h
#define vtkImplicitFunctionToImageStencil_h


#include "vtkImagingStencilModule.h" // For export macro
#include "vtkImageStencilSource.h"

class vtkImplicitFunction;

class VTKIMAGINGSTENCIL_EXPORT vtkImplicitFunctionToImageStencil : public vtkImageStencilSource
{
public:
  static vtkImplicitFunctionToImageStencil *New();
  vtkTypeMacro(vtkImplicitFunctionToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the implicit function to convert into a stencil.
  virtual void SetInput(vtkImplicitFunction*);
  vtkGetObjectMacro(Input, vtkImplicitFunction);

  // Description:
  // Set the threshold value for the implicit function.
  vtkSetMacro(Threshold, double);
  vtkGetMacro(Threshold, double);

  // Description:
  // Override GetMTime() to account for the implicit function.
  unsigned long GetMTime();

protected:
  vtkImplicitFunctionToImageStencil();
  ~vtkImplicitFunctionToImageStencil();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  vtkImplicitFunction *Input;
  double Threshold;

private:
  vtkImplicitFunctionToImageStencil(const vtkImplicitFunctionToImageStencil&);  // Not implemented.
  void operator=(const vtkImplicitFunctionToImageStencil&);  // Not implemented.
};

#endif

