/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMesaRenderWindow.h
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
// .NAME vtkXMesaRenderWindow - Mesa rendering window
// .SECTION Description
// vtkXMesaRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
// library. Application programmers should normally use vtkRenderWindow
// instead of the OpenGL specific version.

#ifndef __vtkXMesaRenderWindow_h
#define __vtkXMesaRenderWindow_h

#include "MangleMesaInclude/gl_mangle.h"
#include "MangleMesaInclude/glx_mangle.h"
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkMesaRenderWindow.h"
#include "MangleMesaInclude/glx.h"
#include "MangleMesaInclude/osmesa.h"



// There used to be some condition for setting this.
// Until I see the need, I am not going to duplicate the code.
#define MESA



class vtkIdList;

class VTK_EXPORT vtkXMesaRenderWindow : public vtkMesaRenderWindow
{
protected:
  GLXContext ContextId;
  
public:
  static vtkXMesaRenderWindow *New();
  vtkTypeMacro(vtkXMesaRenderWindow,vtkMesaRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Begin the rendering process.
  virtual void Start(void);

  // Description:
  // End the rendering process and display the image.
  virtual void Frame(void);

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
  // Make this window the current OpenGL context.
  void MakeCurrent();
  
  // Description:
  // Xwindow get set functions
  virtual void *GetGenericDisplayId() {return (void *)this->GetDisplayId();};
  virtual void *GetGenericWindowId();
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext();
  virtual void *GetGenericDrawable()  {return (void *)this->WindowId;};
  
  // Description:
  // Get the current size of the window in pixels.
  virtual int     *GetSize();

  // Description:
  // Get the size of the screen in pixels
  virtual int     *GetScreenSize();

  // Description:
  // Get the position in screen coordinates (pixels) of the window.
  virtual int     *GetPosition();

  // Description:
  // Get this RenderWindow's X display id.
  Display *GetDisplayId();

  // Description:
  // Set the X display id for this RenderWindow to use to a pre-existing 
  // X display id.
  void     SetDisplayId(Display *);
  void     SetDisplayId(void *);

  // Description:
  // Get this RenderWindow's parent X window id.
  Window   GetParentId();

  // Description:
  // Sets the parent of the window that WILL BE created.
  void     SetParentId(Window);
  void     SetParentId(void *);
  
  // Description:
  // Get this RenderWindow's X window id.
  Window   GetWindowId();

  // Description:
  // Set this RenderWindow's X window id to a pre-existing window.
  void     SetWindowId(Window);
  void     SetWindowId(void *);

  // Description:
  // Specify the X window id to use if a WindowRemap is done.
  void     SetNextWindowId(Window);
  void     SetWindowName(char *);

  // Description:
  // Move the window to a new position on the display.
  void     SetPosition(int,int);
  void     SetPosition(int a[2]) {this->SetPosition(a[0], a[1]);};
  
  // Description:
  // Hide or Show the mouse cursor, it is nice to be able to hide the
  // default cursor if you want VTK to display a 3D cursor instead.
  void HideCursor();
  void ShowCursor();

  // Description:
  // Get the properties available at runtime that would result in 
  // the best Xwindow for 3D rendering.
  virtual  int GetEventPending();
  
  // Description:
  // Set this RenderWindow's X window id to a pre-existing window.
  void     SetWindowInfo(char *info);

  // Description:
  // Sets the X window id of the window that WILL BE created.
  void     SetParentInfo(char *info);

  // Description:
  // This computes the size of the render window 
  // before calling the supper classes render
  void Render();  

  // Description:
  // Render without displaying the window.
  void SetOffScreenRendering(int i);

protected:
  vtkXMesaRenderWindow();
  ~vtkXMesaRenderWindow();
  vtkXMesaRenderWindow(const vtkXMesaRenderWindow&);
  void operator=(const vtkXMesaRenderWindow&);

  Window   ParentId;
  Window   WindowId;
  Window   NextWindowId;
  Display *DisplayId;
  Colormap ColorMap;
  int      OwnWindow;
  int      OwnDisplay;
  int      ScreenSize[2];
  int      CursorHidden;

  // OffScreen stuff
  OSMesaContext OffScreenContextId;
  void *OffScreenWindow;
  int ScreenMapped;
  // Looks like this just stores DoubleBuffer.
  int ScreenDoubleBuffer;
};



#endif
