/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderWindow.h
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
// .NAME vtkMesaRenderWindow - Mesa rendering window
// .SECTION Description
// vtkMesaRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkMesaRenderer interfaces to the Mesa graphics
// library. Application programmers should normally use vtkRenderWindow
// instead of the Mesa specific version.

#ifndef __vtkMesaRenderWindow_h
#define __vtkMesaRenderWindow_h

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkToolkits.h"
#include "vtkXRenderWindow.h"

// include Mesa header files
#ifdef VTK_MANGLE_MESA
#define USE_MGL_NAMESPACE
#include "mesagl.h"
#include "mesaglx.h"
#else
#include "GL/gl.h"
#include "GL/glx.h"
#endif

// version 3.2 and newer do not #define MESA, so 
// look for the only #define in gl.h that is unique to 
// mesa, and if defined, then define MESA
#ifdef GL_MESA_window_pos
#define MESA
#endif


// if we really have the mesa headers then include off screen rendering
#ifdef MESA
#ifndef VTK_MANGLE_MESA
#include "GL/osmesa.h"
#endif
#endif

class vtkIdList;

class VTK_EXPORT vtkMesaRenderWindow : public vtkOpenGLRenderWindow
{
protected:
// If mesa is not defined it means that vtk was configured with-mesa
// but found non-mesa header files, so define OffScreenContextId as
// a void*, so this class will compile anyway.
#ifdef MESA
  OSMesaContext OffScreenContextId;
#else
  void* OffScreenContextId;
#endif
  GLXContext ContextId;

public:
  static vtkMesaRenderWindow *New();
  vtkTypeMacro(vtkMesaRenderWindow,vtkOpenGLRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  virtual void Start(void);

  // Description:
  // End the rendering process and display the image.
  virtual void Frame(void);

  // Description:
  // Specify various window parameters.
  virtual void WindowConfigure(void);

  // Description:
  // Initialize the window for rendering.
  virtual void WindowInitialize(void);

  // Description:
  // Initialize the rendering window.
  virtual void Initialize(void);

  // Description:
  // Change the window to fill the entire screen.
  virtual void SetFullScreen(int);

  // Description:
  // Resize the window.
  virtual void WindowRemap(void);

  // Description:
  // Set the preferred window size to full screen.
  virtual void PrefFullScreen(void);

  // Description:
  // Specify the size of the rendering window.
  virtual void SetSize(int,int);
  virtual void SetSize(int a[2]) {this->SetSize(a[0], a[1]);};

  // Description:
  // Get the X properties of an ideal rendering window.
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  virtual XVisualInfo     *GetDesiredVisualInfo();
  virtual int      GetDesiredDepth();

  // Description:
  // Prescribe that the window be created in a stereo-capable mode. This
  // method must be called before the window is realized. This method
  // overrides the superclass method since this class can actually check
  // whether the window has been realized yet.
  virtual void SetStereoCapableWindow(int capable);

  // Description:
  // Make this window the current Mesa context.
  void MakeCurrent();
  
  // Description:
  // Override the default implementation so that we can actively switch between
  // on and off screen rendering.
  virtual void SetOffScreenRendering(int i);
  
  // Description:
  // Xwindow get set functions
  virtual void *GetGenericWindowId();
  virtual void *GetGenericContext();

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();
  
protected:
  void *OffScreenWindow;
  int ScreenDoubleBuffer;
  int ScreenMapped;
  vtkMesaRenderWindow();
  ~vtkMesaRenderWindow();
  vtkMesaRenderWindow(const vtkMesaRenderWindow&);
  void operator=(const vtkMesaRenderWindow&);
};

#endif
