/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMapToWindowLevelColors.h
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
// .NAME vtkImageMapToWindowLevelColors - map the input image through a lookup table and window / level it
// .SECTION Description
// The vtkImageMapToWindowLevelColors filter will take an input image of any
// valid scalar type, and map the first component of the image through a
// lookup table.  This resulting color will be modulated with value obtained
// by a window / level operation. The result is an image of type 
// VTK_UNSIGNED_CHAR. If the lookup table is not set, or is set to NULL, then 
// the input data will be passed through if it is already of type 
// UNSIGNED_CHAR.
//
// .SECTION See Also
// vtkLookupTable vtkScalarsToColors

#ifndef __vtkImageMapToWindowLevelColors_h
#define __vtkImageMapToWindowLevelColors_h


#include "vtkImageMapToColors.h"

class VTK_EXPORT vtkImageMapToWindowLevelColors : public vtkImageMapToColors
{
public:
  static vtkImageMapToWindowLevelColors *New();
  vtkTypeMacro(vtkImageMapToWindowLevelColors,vtkImageToImageFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / Get the Window to use -> modulation will be performed on the 
  // color based on (S - (L - W/2))/W where S is the scalar value, L is
  // the level and W is the window.
  vtkSetMacro( Window, float );
  vtkGetMacro( Window, float );
  
  // Description:
  // Set / Get the Level to use -> modulation will be performed on the 
  // color based on (S - (L - W/2))/W where S is the scalar value, L is
  // the level and W is the window.
  vtkSetMacro( Level, float );
  vtkGetMacro( Level, float );
  
protected:
  vtkImageMapToWindowLevelColors();
  ~vtkImageMapToWindowLevelColors();
  vtkImageMapToWindowLevelColors(const vtkImageMapToWindowLevelColors&) {};
  void operator=(const vtkImageMapToWindowLevelColors&) {};

  void ExecuteInformation(vtkImageData *inData, vtkImageData *outData);
  void ExecuteInformation(){this->vtkImageMapToColors::ExecuteInformation();};
  void ThreadedExecute(vtkImageData *inData, vtkImageData *outData,
		       int extent[6], int id);
  void ExecuteData(vtkDataObject *output);
  
  float Window;
  float Level;
  
};

#endif







