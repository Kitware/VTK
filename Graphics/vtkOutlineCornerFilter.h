/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutlineCornerFilter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Sebastien Barre who developed this class.


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
// .NAME vtkOutlineCornerFilter - create wireframe outline corners for arbitrary data set
// .SECTION Description
// vtkOutlineCornerFilter is a filter that generates wireframe outline corners of any 
// data set. The outline consists of the eight corners of the dataset 
// bounding box.

#ifndef __vtkOutlineCornerFilter_h
#define __vtkOutlineCornerFilter_h

#include "vtkDataSetToPolyDataFilter.h"
class vtkOutlineCornerSource;

class VTK_GRAPHICS_EXPORT vtkOutlineCornerFilter : public vtkDataSetToPolyDataFilter
{
public:
  vtkTypeMacro(vtkOutlineCornerFilter,vtkDataSetToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct outline corner filter with default corner factor = 0.2
  static vtkOutlineCornerFilter *New();

  // Description:
  // Set/Get the factor that controls the relative size of the corners
  // to the length of the corresponding bounds
  vtkSetClampMacro(CornerFactor, float, 0.001, 0.5);
  vtkGetMacro(CornerFactor, float);

protected:
  vtkOutlineCornerFilter();
  ~vtkOutlineCornerFilter();
  vtkOutlineCornerFilter(const vtkOutlineCornerFilter&);
  void operator=(const vtkOutlineCornerFilter&);

  vtkOutlineCornerSource *OutlineCornerSource;
  void Execute();
  void ExecuteInformation();

  float CornerFactor;
};

#endif


