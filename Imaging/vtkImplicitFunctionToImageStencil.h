/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionToImageStencil.h
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
// .NAME vtkImplicitFunctionToImageStencil - clip an image with a function
// .SECTION Description
// vtkImplicitFunctionToImageStencil will convert a vtkImplicitFunction into
// a stencil that can be used with vtkImageStencil or with other classes
// that apply a stencil to an image.
// .SECTION see also
// vtkImplicitFunction vtkImageStencil vtkPolyDataToImageStencil

#ifndef __vtkImplicitFunctionToImageStencil_h
#define __vtkImplicitFunctionToImageStencil_h


#include "vtkImageStencilSource.h"
#include "vtkImplicitFunction.h"

class VTK_IMAGING_EXPORT vtkImplicitFunctionToImageStencil : public vtkImageStencilSource
{
public:
  static vtkImplicitFunctionToImageStencil *New();
  vtkTypeRevisionMacro(vtkImplicitFunctionToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the implicit function to convert into a stencil.
  vtkSetObjectMacro(Input, vtkImplicitFunction);
  vtkGetObjectMacro(Input, vtkImplicitFunction);

  // Description:
  // Set the threshold value for the implicit function.
  vtkSetMacro(Threshold, float);
  vtkGetMacro(Threshold, float);

protected:
  vtkImplicitFunctionToImageStencil();
  ~vtkImplicitFunctionToImageStencil();

  void ThreadedExecute(vtkImageStencilData *output,
                       int extent[6], int threadId);

  vtkImplicitFunction *Input;
  float Threshold;
private:
  vtkImplicitFunctionToImageStencil(const vtkImplicitFunctionToImageStencil&);  // Not implemented.
  void operator=(const vtkImplicitFunctionToImageStencil&);  // Not implemented.
};

#endif

