/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXImageWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkXImageWindow - 2D display window for X
// .SECTION Description
// vtkXImageWindow is a concrete subclass of vtkImageWindow to support
// contains 2D rendering in vtk. 

// .SECTION See Also
// vtkImageWindow

#ifndef __vtkXImageWindow_h
#define __vtkXImageWindow_h

#include 	<X11/Xlib.h>
#include 	<X11/Xutil.h>
#include 	<X11/cursorfont.h>
#include 	<X11/X.h>
#include 	<X11/keysym.h>
#include    	<fstream.h>
#include 	<stdlib.h>
#include 	<iostream.h>

#include        "vtkImageWindow.h"

class VTK_EXPORT vtkXImageWindow : public vtkImageWindow
{
public:
  vtkXImageWindow();
  ~vtkXImageWindow();

  static vtkXImageWindow *New() {return new vtkXImageWindow;};
  const char *GetClassName() {return "vtkXImageWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // output to the viewer.
  vtkXImageWindow *GetOutput(){return this;};
  
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
  int     *GetPosition();
  void     SetPosition(int,int);

  int     *GetSize();
  void     SetSize(int x, int y);

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

  void GetShiftsAndMasks(int &rshift, int &gshift, int &bshift,
			 unsigned long &rmask, unsigned long &gmask,
			 unsigned long &bmask);

protected:
  // X stuff
  Window               ParentId;
  Window	           WindowId;
  Display             *DisplayId;
  Visual              *VisualId;
  int                  VisualDepth;
  int                  VisualClass;
  Colormap             ColorMap;
  GC                   Gc;
  Pixmap               IconPixmap;
  XEvent               report;
  int	               Offset;
  XColor               Colors[256];
  int                  NumberOfColors;
  Pixmap               Drawable;
  int                  OwnDisplay;
  
  void MakeDefaultWindow();
  void GetDefaultVisualInfo(XVisualInfo *info);
  Colormap MakeColorMap(Visual *visual);
  void AllocateDirectColorMap();
};

#endif





