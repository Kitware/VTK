/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindow.h
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
// .NAME vtkXRenderWindow - a rendering window for the X Window system
// .SECTION Description
// vtkXRenderWindow is a subclass of the abstract class
// vtkRenderWindow. vtkXRenderWindow interfaces to the X Window system and
// provides some methods that are common to any vtkRenderWindow
// subclass that renders under X Windows. The vtkXRenderWindowInteractor
// makes heavy use of these common methods.

// .SECTION see also
// vtkRenderWindow vtkXRenderWindowInteractor

#ifndef __vtkXRenderWindow_h
#define __vtkXRenderWindow_h

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkRenderWindow.h"

class VTK_EXPORT vtkXRenderWindow : public vtkRenderWindow
{
public:
  vtkTypeMacro(vtkXRenderWindow,vtkRenderWindow);
  static vtkXRenderWindow *New();
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Xwindow get set functions
  virtual void *GetGenericDisplayId() {return (void *)this->GetDisplayId();};
  virtual void *GetGenericWindowId()  {return (void *)this->WindowId;};
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
  virtual int      GetDesiredDepth()    = 0;
  virtual Colormap GetDesiredColormap() = 0;
  virtual Visual  *GetDesiredVisual()   = 0;
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

protected:
  vtkXRenderWindow();
  ~vtkXRenderWindow();
  vtkXRenderWindow(const vtkXRenderWindow&) {};
  void operator=(const vtkXRenderWindow&) {};

  Window   ParentId;
  Window   WindowId;
  Window   NextWindowId;
  Display *DisplayId;
  Colormap ColorMap;
  int      OwnWindow;
  int      OwnDisplay;
  int      ScreenSize[2];
  int      CursorHidden;
};

#endif
