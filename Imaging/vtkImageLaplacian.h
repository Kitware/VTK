/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageLaplacian.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
// .NAME vtkImageLaplacian - Computes divergence of gradient.
// .SECTION Description
// vtkimageLaplacian computes the Laplacian (like a second derivative)
// of a scalar image.  The operation is the same as taking the
// divergence after a gradient.  Boundaries are handled, so the input
// is the same as the output.  The output is always float.
// Dimensionality determines how the input regions are interpreted.
// (images, or volumes). The Dimensionality defaults to two.



#ifndef __vtkimageLaplacian_h
#define __vtkimageLaplacian_h


#include "vtkImageToImageFilter.h"

class VTK_IMAGING_EXPORT vtkImageLaplacian : public vtkImageToImageFilter
{
public:
  static vtkImageLaplacian *New();
  vtkTypeMacro(vtkImageLaplacian,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Determines how the input is interpreted (set of 2d slices ...)
  vtkSetClampMacro(Dimensionality,int,2,3);
  vtkGetMacro(Dimensionality,int);
  
protected:
  vtkImageLaplacian();
  ~vtkImageLaplacian() {};

  int Dimensionality;

  void ComputeInputUpdateExtent(int inExt[6], int outExt[6]);
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int ext[6], int id);
private:
  vtkImageLaplacian(const vtkImageLaplacian&);  // Not implemented.
  void operator=(const vtkImageLaplacian&);  // Not implemented.
};

#endif



