/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXImageWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
// .NAME vtkXImageWindow - 2D display window for X
// .SECTION Description
// vtkXImageWindow is a concrete subclass of vtkImageWindow to support
// 2D rendering in vtk. 

// .SECTION See Also
// vtkImageWindow

#ifndef __vtkXImageWindow_h
#define __vtkXImageWindow_h

#include 	<X11/Xlib.h>
#include 	<X11/Xutil.h>
#include 	<X11/cursorfont.h>
#include 	<X11/X.h>
#include 	<X11/keysym.h>

#include        "vtkImageWindow.h"

class VTK_RENDERING_EXPORT vtkXImageWindow : public vtkImageWindow
{
public:
  static vtkXImageWindow *New();
  vtkTypeMacro(vtkXImageWindow,vtkImageWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implements SetWindowName for a X window
  void SetWindowName(char* name);
  
  // Description:
  // Gets the number of colors in the pseudo color map.
  vtkGetMacro(NumberOfColors,int);
  
  // Description:
  // Gets the windows depth. For the templated function.
  vtkGetMacro(VisualDepth,int);
  
  // Description:
  // Gets the windows visual class. For the templated function.
  vtkGetMacro(VisualClass,int);
  
  // Description:
  // These are here for using a tk window.
  Window   GetParentId();
  void     SetParentId(Window);
  void     SetParentId(void *);
  void     SetDisplayId(Display *);
  void     SetDisplayId(void *);
  void     SetWindowId(Window);
  void     SetWindowId(void *);
  Window   GetWindowId();
  Display *GetDisplayId();
  GC       GetGC();

  // Description:
  // Set/Get the position of the window.
  int     *GetPosition();
  void     GetPosition(int* x, int* y) {this->vtkImageWindow::GetPosition(x, y);};
  void     SetPosition(int,int);
  void     SetPosition(int a[2]) { this->SetPosition(a[0],a[1]); };
 
  // Description:
  // Set/Get the size of the window.
  int     *GetSize();
  void     GetSize(int* x, int* y) {this->vtkImageWindow::GetSize(x, y);};
  void     SetSize(int x, int y);
  void     SetSize(int a[2]) { this->SetSize(a[0], a[1]); };
  
  // Description:
  // The GetGenericXXX functions are necessary to draw into
  // a VTKWindow.
  void *GetGenericDisplayId() {return (void*) this->DisplayId;};
  void *GetGenericWindowId() {return (void*) this->WindowId;};
  void *GetGenericParentId() {return (void*) this->ParentId;};
  void *GetGenericContext() {return (void*) this->Gc;};
  void *GetGenericDrawable();

  // Description:
  // Swaps the 2D drawing buffers.  The user should not need to 
  // use this call.  To invoke double buffering, call DoubleBufferOn
  void SwapBuffers();

  // Description:
  // Flush the buffer and swap buffers if necessary.
  void Frame();

  // Description:
  // Determine the desired depth of the window.
  virtual int      GetDesiredDepth();
  
  // Description:
  // Determine the desired colormap of the window.
  virtual Colormap GetDesiredColormap();

  // Description:
  // Determine the desired visual for the window
  virtual Visual  *GetDesiredVisual();

  // Description:
  // Return the id of the visual we are using
  Visual *GetVisualId() {return VisualId;};

  // Description:
  // Set the background color of the window.
  void SetBackgroundColor(float r, float g, float b);

  // Description:
  // Erase the contents of the window.
  void EraseWindow();

  // Description:
  // Get the pixel data of an image, transmitted as RGBRGBRGB. 
  // It is the caller's responsibility to delete the resulting 
  // array. It is very important to realize that the memory in this array
  // is organized from the bottom of the window to the top. The origin
  // of the screen is in the lower left corner. The y axis increases as
  // you go up the screen. So the storage of pixels is from left to right
  // and from bottom to top.  To maintain the same prototype as in 
  // vtkRenderWindow, the last argument is provided, but currently not used.
  unsigned char *GetPixelData(int x1, int y1, int x2, int y2, int);

  // Description:
  // Set this ImageWindow's X window id to a pre-existing window.
  void     SetWindowInfo(char *info);

  // Description:
  // Sets the X window id of the window that WILL BE created.
  void     SetParentInfo(char *info);

protected:
  vtkXImageWindow();
  ~vtkXImageWindow();

  // X stuff
  Window               ParentId;
  Window	       WindowId;
  Display             *DisplayId;
  Visual              *VisualId;
  int                  VisualDepth;
  int                  VisualClass;
  Colormap             ColorMap;
  GC                   Gc;
  int	               Offset;
  XColor               Colors[256];
  int                  NumberOfColors;
  Pixmap               Drawable;
  int                  OwnDisplay;
  int                  PixmapWidth;
  int                  PixmapHeight;

  
  void MakeDefaultWindow();
  void GetDefaultVisualInfo(XVisualInfo *info);
  Colormap MakeColorMap(Visual *visual);
  void AllocateDirectColorMap();
  void GetShiftsScalesAndMasks(int &rshift, int &gshift, int &bshift,
			       int &rscale, int &gscale, int &bscale,
			       unsigned long &rmask, unsigned long &gmask,
			       unsigned long &bmask);
private:
  vtkXImageWindow(const vtkXImageWindow&);  // Not implemented.
  void operator=(const vtkXImageWindow&);  // Not implemented.
};

#endif





