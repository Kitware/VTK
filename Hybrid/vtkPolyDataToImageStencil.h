/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.h
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
// .NAME vtkPolyDataToImageStencil - clip an image with polydata
// .SECTION Description
// vtkPolyDataToImageStencil will convert a vtkPolyData into an image
// that can be used with vtkImageStecil or other vtk classes that apply
// a stencil to an image.
// .SECTION see also
// vtkPolyData vtkImageStencil vtkImplicitFunctionToImageStencil

#ifndef __vtkPolyDataToImageStencil_h
#define __vtkPolyDataToImageStencil_h


#include "vtkImageStencilSource.h"
#include "vtkPolyData.h"
#include "vtkOBBTree.h"

class VTK_HYBRID_EXPORT vtkPolyDataToImageStencil : public vtkImageStencilSource
{
public:
  static vtkPolyDataToImageStencil *New();
  vtkTypeMacro(vtkPolyDataToImageStencil, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the polydata to convert into a stencil.
  void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput();

  // Description:
  // Set the tolerance for doing spatial searches of the polydata.
  vtkSetMacro(Tolerance, float);
  vtkGetMacro(Tolerance, float);

protected:
  vtkPolyDataToImageStencil();
  ~vtkPolyDataToImageStencil();
  vtkPolyDataToImageStencil(const vtkPolyDataToImageStencil&)
    {};
  void operator=(const vtkPolyDataToImageStencil&);

  void ExecuteData(vtkDataObject *out);
  void ThreadedExecute(vtkImageStencilData *output,
		       int extent[6], int threadId);

  float Tolerance;
  vtkOBBTree *OBBTree;
};

#endif
