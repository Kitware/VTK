/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageWindow.h
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
// .NAME vtkOpenGLImageWindow - OpenGL Imageing window
// .SECTION Description
// vtkOpenGLImageWindow is a concrete implementation of the abstract
// class vtkImageWindow. vtkOpenGLImageer interfaces to the standard
// OpenGL graphics library in the Windows/NT environment..

#ifndef __vtkOpenGLImageWindow_h
#define __vtkOpenGLImageWindow_h

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkXImageWindow.h"
#include "GL/glx.h"


class VTK_RENDERING_EXPORT vtkOpenGLImageWindow : public vtkXImageWindow
{
public:
  static vtkOpenGLImageWindow *New();
  vtkTypeMacro(vtkOpenGLImageWindow,vtkXImageWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the window for rendering.
  virtual void MakeDefaultWindow();

  // Description:
  // Swap the front and back buffers if double buffering is being used.
  void SwapBuffers();

  // Description:
  // Flush the buffers and swap if necessary
  void Frame();

  // Description:
  // Draw the contents of the window
  void Render();

  // Description:
  // Xwindow get set functions
  virtual void *GetGenericDisplayId() {return (void *)this->DisplayId;};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext();
  virtual void *GetGenericDrawable()  {return (void *)this->WindowId;};
  
  // Description:
  // Get the X properties of an ideal rendering window.
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  XVisualInfo     *GetDesiredVisualInfo();
  virtual int      GetDesiredDepth();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *,
			    int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2,float *,int front,
                                int blend=0);

  // Description:
  // Make this windows OpenGL context the current context.
  void MakeCurrent();

  // Description:
  // Erase the window contents 
  virtual void EraseWindow() {this->vtkImageWindow::EraseWindow();};

  GLXContext ContextId;

protected:
  vtkOpenGLImageWindow();
  ~vtkOpenGLImageWindow();
  vtkOpenGLImageWindow(const vtkOpenGLImageWindow&);
  void operator=(const vtkOpenGLImageWindow&);
};


#endif

