/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlankStructuredGridWithImage.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkBlankStructuredGridWithImage - blank a structured grid with an image
// .SECTION Description
// This filter can be used to set the blanking in a structured grid with 
// an image. The filter takes two inputs: the structured grid to blank, 
// and the image used to set the blanking. Make sure that the dimensions of
// both the image and the structured grid are identical.
//
// Note that the image is interpreted as follows: zero values indicate that
// the structured grid point is blanked; non-zero values indicate that the
// structured grid point is visible. The blanking data must be unsigned char.

// .SECTION See Also
// vtkStructuredGrid

#ifndef __vtkBlankStructuredGridWithImage_h
#define __vtkBlankStructuredGridWithImage_h

#include "vtkStructuredGridToStructuredGridFilter.h"

class vtkImageData;

class VTK_EXPORT vtkBlankStructuredGridWithImage : public vtkStructuredGridToStructuredGridFilter
{
public:
  static vtkBlankStructuredGridWithImage *New();
  vtkTypeMacro(vtkBlankStructuredGridWithImage,vtkStructuredGridToStructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input image used to perform the blanking.
  void SetBlankingInput(vtkImageData *input);
  vtkImageData *GetBlankingInput();

protected:
  vtkBlankStructuredGridWithImage() {this->NumberOfRequiredInputs = 2;}
  ~vtkBlankStructuredGridWithImage() {}
  vtkBlankStructuredGridWithImage(const vtkBlankStructuredGridWithImage&);
  void operator=(const vtkBlankStructuredGridWithImage&);

  void Execute();
};

#endif


