/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper.h
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
// .NAME vtkMesaPolyDataMapper - a PolyDataMapper for the Mesa library
// .SECTION Description
// vtkMesaPolyDataMapper is a subclass of vtkPolyDataMapper.
// vtkMesaPolyDataMapper is a geometric PolyDataMapper for the Mesa 
// rendering library.

#ifndef __vtkMesaPolyDataMapper_h
#define __vtkMesaPolyDataMapper_h

#include "vtkPolyDataMapper.h"
#include <stdlib.h>
#include "vtkToolkits.h"

#ifdef VTK_MANGLE_MESA
#define USE_MGL_NAMESPACE
#include "mesagl.h"
#else
#include "GL/gl.h"
#endif

class vtkProperty;
class vtkRenderWindow;
class vtkMesaRenderer;
class vtkTimerLog;

class VTK_EXPORT vtkMesaPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkMesaPolyDataMapper *New();
  vtkTypeMacro(vtkMesaPolyDataMapper,vtkPolyDataMapper);

  // Description:
  // Implement superclass render method.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw method for Mesa.
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
  vtkMesaPolyDataMapper();
  ~vtkMesaPolyDataMapper();
  vtkMesaPolyDataMapper(const vtkMesaPolyDataMapper&) {};
  void operator=(const vtkMesaPolyDataMapper&) {};

  int ListId;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for the previous render
};

#endif
