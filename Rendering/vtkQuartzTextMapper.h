/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzTextMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkQuartzTextMapper - 2D Text annotation support for windows
// .SECTION Description
// vtkQuartzTextMapper provides 2D text annotation support for vtk under
// Xwindows.  Normally the user should use vtktextMapper which in turn will
// use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkQuartzTextMapper_h
#define __vtkQuartzTextMapper_h

#include "vtkTextMapper.h"

class VTK_RENDERING_EXPORT vtkQuartzTextMapper : public vtkTextMapper
{
public:
  vtkTypeMacro(vtkQuartzTextMapper,vtkTextMapper);
  static vtkQuartzTextMapper *New();

  // Description:
  // Actally draw the text.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // What is the size of the rectangle required to draw this
  // mapper ?
  void GetSize(vtkViewport* viewport, int size[2]);

protected:
  vtkQuartzTextMapper();
  ~vtkQuartzTextMapper();

  vtkTimeStamp  BuildTime;
  int LastSize[2];
  void *Font;
private:
  vtkQuartzTextMapper(const vtkQuartzTextMapper&) {};  // Not implemented.
  void operator=(const vtkQuartzTextMapper&) {};  // Not implemented.
};


#endif

