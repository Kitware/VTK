/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32TextMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
// .NAME vtkWin32TextMapper - 2D Text annotation support for windows
// .SECTION Description
// vtkWin32TextMapper provides 2D text annotation support for vtk under
// Xwindows.  Normally the user should use vtktextMapper which in turn will
// use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkWin32TextMapper_h
#define __vtkWin32TextMapper_h

#include "vtkTextMapper.h"

class VTK_RENDERING_EXPORT vtkWin32TextMapper : public vtkTextMapper
{
public:
  vtkTypeMacro(vtkWin32TextMapper,vtkTextMapper);
  static vtkWin32TextMapper *New();

  // Description:
  // What is the size of the rectangle required to draw this
  // mapper ?
  void GetSize(vtkViewport* viewport, int size[2]);

protected:
  vtkWin32TextMapper();
  ~vtkWin32TextMapper();
  vtkWin32TextMapper(const vtkWin32TextMapper&);
  void operator=(const vtkWin32TextMapper&);

  vtkTimeStamp  BuildTime;
  int LastSize[2];
  HFONT Font;
};


#endif

