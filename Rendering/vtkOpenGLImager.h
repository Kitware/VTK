/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImager.h
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
// .NAME vtkImager - Renders into part of a ImageWindow
// .SECTION Description
// vtkImager is the 2D counterpart to vtkRenderer. An Imager renders
// 2D actors into a viewport of an image window. 

// .SECTION See Also
//  vtkImageWindow vtkViewport
   

#ifndef __vtkOpenGLImager_h
#define __vtkOpenGLImager_h

#include "vtkImager.h"

class VTK_EXPORT vtkOpenGLImager : public vtkImager
{ 
public:
  static vtkOpenGLImager *New();
  vtkTypeMacro(vtkOpenGLImager,vtkImager);

  // Description:
  // Renders an imager.  Passes Render message on the 
  // the imager's actor2D collection.
  int RenderOpaqueGeometry();

  // Description:
  // Erase the contents of the imager in the window.
  void Erase();

protected:
  vtkOpenGLImager() {};
  ~vtkOpenGLImager() {};
  vtkOpenGLImager(const vtkOpenGLImager&);
  void operator=(const vtkOpenGLImager&);
};


#endif




