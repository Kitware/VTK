/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32PolyDataMapper2D.h
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
// .NAME vtkWin32PolyDataMapper2D - (obsolete) 2D PolyData support for windows
// .SECTION Description
// vtkWin32PolyDataMapper2D provides 2D PolyData annotation support for 
// vtk under windows.  Normally the user should use vtkPolyDataMapper2D 
// which in turn will use this class.

// .SECTION See Also
// vtkPolyDataMapper2D

#ifndef __vtkWin32PolyDataMapper2D_h
#define __vtkWin32PolyDataMapper2D_h

#include "vtkPolyDataMapper2D.h"

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_EXPORT vtkWin32PolyDataMapper2D : public vtkPolyDataMapper2D
{
public:
  vtkTypeMacro(vtkWin32PolyDataMapper2D,vtkPolyDataMapper2D);
  static vtkWin32PolyDataMapper2D *New();

  // Description:
  // Actually draw the poly data.
  virtual void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

protected:
  vtkWin32PolyDataMapper2D() {};
  ~vtkWin32PolyDataMapper2D() {};
  vtkWin32PolyDataMapper2D(const vtkWin32PolyDataMapper2D&);
  void operator=(const vtkWin32PolyDataMapper2D&);
  
};
#endif

#endif

