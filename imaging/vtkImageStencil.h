/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencil.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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
// .NAME vtkImageStencil - combine images via a cookie-cutter operation
// .SECTION Description
//  vtkImageStencil will combine two images give a vtkImplicitFunction as
//  a stencil.  More generally, a vtkImageClippingExtents object can be
//  used to allow clipping with a broad variety of vtk object types,
//  e.g. a closed vtkPolyData surface.

#ifndef __vtkImageStencil_h
#define __vtkImageStencil_h

#include "vtkImageMultipleInputFilter.h"
#include "vtkImplicitFunction.h"
#include "vtkImageClippingExtents.h"

class VTK_EXPORT vtkImageStencil : public vtkImageMultipleInputFilter
{
public:
  static vtkImageStencil *New();
  vtkTypeMacro(vtkImageStencil, vtkImageMultipleInputFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the input.  This image will be used for the 'inside' of the stencil.
  virtual void SetInput(vtkImageData *input);
  virtual void SetInput(int n, vtkImageData *input);

  // Description:
  // NOTE: Not yet implemented, use SetDefaultValue instead.
  // Set the second input.  This image will be used for the 'outside' of the
  // stencil.  If not set, the output voxels will be filled with
  // DefaultValue instead.
  virtual void SetInput2(vtkImageData *input);
  vtkImageData *GetInput2() { return this->GetInput(1); };

  // Description:
  // Set the implicit function to use as a stencil.
  vtkSetObjectMacro(StencilFunction, vtkImplicitFunction);
  vtkGetObjectMacro(StencilFunction, vtkImplicitFunction);

  // Description:
  // Specify a vtkImageClippingExtents object to use instead of 
  // setting a StencilFunction.  The vtkImageClippingExtents
  // is a class for efficiently specifying an image stencil
  // for large images.
  vtkSetObjectMacro(ClippingExtents, vtkImageClippingExtents);
  vtkGetObjectMacro(ClippingExtents, vtkImageClippingExtents);

  // Description:
  // Reverse the stencil.
  vtkSetMacro(ReverseStencil, int);
  vtkBooleanMacro(ReverseStencil, int);
  vtkGetMacro(ReverseStencil, int);

  // Description:
  // Set the default output value to use when the second input is not set.
  void SetDefaultValue(float val) { this->SetDefaultColor(val,val,val,val); };
  float GetDefaultValue() { return this->DefaultColor[0]; };

  // Description:
  // Set the default color to use when the second input is not set.
  // This is like SetDefaultValue1, but for multi-component images.
  vtkSetVector4Macro(DefaultColor, float);
  vtkGetVector4Macro(DefaultColor, float);

protected:
  vtkImageStencil();
  ~vtkImageStencil();
  vtkImageStencil(const vtkImageStencil&) {};
  void operator=(const vtkImageStencil&) {};

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6], int n);
  
  void ExecuteInformation(vtkImageData **inDatas, vtkImageData *outData);

  void ThreadedExecute(vtkImageData **inDatas, vtkImageData *outData,
                       int extent[6], int id);
  
  vtkImplicitFunction *StencilFunction;
  vtkImageClippingExtents *ClippingExtents;
  int ReverseStencil;
  float DefaultColor[4];
};

#endif













