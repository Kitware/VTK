/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCursor3D.h
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
// .NAME vtkImageCursor3D - Paints a cursor on top of an image or volume.
// .SECTION Description
// vtkImageCursor3D will draw a cursor on a 2d image or 3d volume.

#ifndef __vtkImageCursor3D_h
#define __vtkImageCursor3D_h

#include "vtkImageInPlaceFilter.h"

class VTK_IMAGING_EXPORT vtkImageCursor3D : public vtkImageInPlaceFilter
{
public:
  static vtkImageCursor3D *New();
  vtkTypeMacro(vtkImageCursor3D,vtkImageInPlaceFilter);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Sets/Gets the center point of the 3d cursor.
  vtkSetVector3Macro(CursorPosition, float);
  vtkGetVector3Macro(CursorPosition, float);

  // Description:
  // Sets/Gets what pixel value to draw the cursor in.
  vtkSetMacro(CursorValue, float);
  vtkGetMacro(CursorValue, float);
  
  // Description:
  // Sets/Gets the radius of the cursor. The radius determines
  // how far the axis lines project out from the cursors center.
  vtkSetMacro(CursorRadius, int);
  vtkGetMacro(CursorRadius, int);
  
  
protected:
  vtkImageCursor3D();
  ~vtkImageCursor3D() {};

  float CursorPosition[3];
  float CursorValue;
  int CursorRadius;
  
  // not threaded because it's too simple a filter
  void ExecuteData(vtkDataObject *outData);
private:
  vtkImageCursor3D(const vtkImageCursor3D&);  // Not implemented.
  void operator=(const vtkImageCursor3D&);  // Not implemented.
};



#endif



