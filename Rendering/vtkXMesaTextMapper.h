/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMesaTextMapper.h
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
// .NAME vtkXOpenGLTextMapper - 2D Text annotation support for windows
// .SECTION Description
// vtkXMesaTextMapper provides 2D text annotation support for vtk under
// Xwindows.  Normally the user should use vtktextMapper which in turn will
// use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkXMesaTextMapper_h
#define __vtkXMesaTextMapper_h

#include "vtkXTextMapper.h"

class VTK_RENDERING_EXPORT vtkXMesaTextMapper : public vtkXTextMapper
{
public:
  vtkTypeMacro(vtkXMesaTextMapper,vtkXTextMapper);
  static vtkXMesaTextMapper *New();

  // Description:
  // Actally draw the text.
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor);
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) {};

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // An internal function used for caching font display lists.
  static int GetListBaseForFont(vtkTextMapper *tm, vtkViewport *vp,
				Font);

protected:
  vtkXMesaTextMapper();
  ~vtkXMesaTextMapper() {};
private:
  vtkXMesaTextMapper(const vtkXMesaTextMapper&);  // Not implemented.
  void operator=(const vtkXMesaTextMapper&);  // Not implemented.
};


#endif

