/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPolyDataMapper.h
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
// .NAME vtkOpenGLPolyDataMapper - a PolyDataMapper for the OpenGL library
// .SECTION Description
// vtkOpenGLPolyDataMapper is a subclass of vtkPolyDataMapper.
// vtkOpenGLPolyDataMapper is a geometric PolyDataMapper for the OpenGL 
// rendering library.

#ifndef __vtkOpenGLPolyDataMapper_h
#define __vtkOpenGLPolyDataMapper_h

#include "vtkPolyDataMapper.h"
#include <stdlib.h>
#ifndef VTK_IMPLEMENT_MESA_CXX
  #ifdef __APPLE__
    #include <OpenGL/gl.h>
  #else
    #include <GL/gl.h>
  #endif
#endif

class vtkProperty;
class vtkRenderWindow;
class vtkOpenGLRenderer;

class VTK_RENDERING_EXPORT vtkOpenGLPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkOpenGLPolyDataMapper *New();
  vtkTypeMacro(vtkOpenGLPolyDataMapper,vtkPolyDataMapper);

  // Description:
  // Implement superclass render method.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw method for OpenGL.
  virtual void Draw(vtkRenderer *ren, vtkActor *a);
  
  //BTX  begin tcl exclude
  
  // Description:
  // Get the lmcolor property, this is a pretty important little 
  // function.  It determines how vertex colors will be handled  
  // in gl.  When a PolyDataMapper has vertex colors it will use this 
  // method to determine what lmcolor mode to set.               
  GLenum GetLmcolorMode(vtkProperty *prop);
  //ETX

protected:
  vtkOpenGLPolyDataMapper();
  ~vtkOpenGLPolyDataMapper();

  int ListId;
private:
  vtkOpenGLPolyDataMapper(const vtkOpenGLPolyDataMapper&);  // Not implemented.
  void operator=(const vtkOpenGLPolyDataMapper&);  // Not implemented.
};

#endif
