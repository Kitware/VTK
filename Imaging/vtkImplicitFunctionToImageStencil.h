/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitFunctionToImageStencil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

class VTK_EXPORT vtkImplicitFunctionToImageStencil 
  : public vtkImageStencilSource
{
public:
  static vtkImplicitFunctionToImageStencil *New();
  vtkTypeMacro(vtkImplicitFunctionToImageStencil, vtkImageStencilSource);
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
  vtkImplicitFunctionToImageStencil(const vtkImplicitFunctionToImageStencil&)
    {};
  void operator=(const vtkImplicitFunctionToImageStencil&);

  void ThreadedExecute(vtkImageStencilData *output,
		       int extent[6], int threadId);

  vtkImplicitFunction *Input;
  float Threshold;
};

#endif

