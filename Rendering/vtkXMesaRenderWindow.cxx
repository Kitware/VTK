/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMesaRenderWindow.cxx
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
#define VTK_IMPLEMENT_MESA_CXX

#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/gl.h"
#include "MangleMesaInclude/osmesa.h"
#include "vtkXMesaRenderWindow.h"
#include "vtkMesaRenderer.h"
#include "vtkMesaProperty.h"
#include "vtkMesaTexture.h"
#include "vtkMesaCamera.h"
#include "vtkMesaLight.h"
#include "vtkMesaActor.h"
#include "vtkMesaPolyDataMapper.h"


#define vtkXOpenGLRenderWindow vtkXMesaRenderWindow
#define vtkOpenGLRenderWindow vtkMesaRenderWindow
#define vtkOpenGLRenderer vtkMesaRenderer
#define vtkOpenGLProperty vtkMesaProperty
#define vtkOpenGLTexture vtkMesaTexture
#define vtkOpenGLCamera vtkMesaCamera
#define vtkOpenGLLight vtkMesaLight
#define vtkOpenGLActor vtkMesaActor
#define vtkOpenGLPolyDataMapper vtkMesaPolyDataMapper
#define vtkOSMesaDestroyWindow vtkOSMangleMesaDestroyWindow
#define vtkOSMesaCreateWindow vtkOSMangleMesaCreateWindow
#define vtkXOpenGLRenderWindowPredProc vtkXMesaRenderWindowPredProc
#define vtkXOpenGLRenderWindowFoundMatch vtkXMesaRenderWindowFoundMatch
#define vtkXError vtkMesaXError
#define vtkXOpenGLRenderWindowTryForVisual vtkXMesaRenderWindowTryForVisual
#define VTK_OPENGL_HAS_OSMESA 1
// now include the source for vtkXOpenGLRenderWindow
#include "vtkXOpenGLRenderWindow.cxx"

//-----------------------------------------------------------------------------
vtkXMesaRenderWindow* vtkXMesaRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXMesaRenderWindow");
  if(ret)
    {
    return (vtkXMesaRenderWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkXMesaRenderWindow;
}


