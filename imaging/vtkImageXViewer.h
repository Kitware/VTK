/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageXViewer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


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
// .NAME vtkImageXViewer - Display a 2d image in an XWindow.
// .SECTION Description
// vtkImageXViewer displays a 2d image in an X window.

#ifndef __vtkImageXViewer_h
#define __vtkImageXViewer_h

#include 	<X11/Xlib.h>
#include 	<X11/Xutil.h>
#include 	<X11/cursorfont.h>
#include 	<X11/X.h>
#include 	<X11/keysym.h>
#include    	<fstream.h>
#include 	<stdlib.h>
#include 	<iostream.h>

#include 	"vtkImageViewer.h"

class VTK_EXPORT vtkImageXViewer : public vtkImageViewer {
public:
  vtkImageXViewer();
  ~vtkImageXViewer();
  char *GetClassName() {return "vtkImageXViewer";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  XColor *GetColors();
  void Render(void);
  
  void SetWindow(int win);
  int GetWindow();
  
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
  // Window/level information used by templated function.
  float GetColorShift();
  float GetColorScale();

  // Description:
  // These are here for using a tk window.
  void SetDisplayId(Display *);
  void SetDisplayId(void *);
  void SetWindowId(Window);
  void SetWindowId(void *);
  
protected:
  // X stuff
  Window	       WindowId;
  Display             *DisplayId;
  Visual              *VisualId;
  int                  VisualDepth;
  int                  VisualClass;
  Colormap             ColorMap;
  GC                   Gc;
  Pixmap               IconPixmap;
  XImage              *Image;
  XEvent               report;
  int	               Offset;
  XColor               Colors[256];
  int                  NumberOfColors;
  
  
  Window MakeDefaultWindow(int width, int height);
  void GetDefaultVisualInfo(XVisualInfo *info);
  Colormap MakeColorMap(Visual *visual);
  void AllocateDirectColorMap();
  Display *GetDisplayId();
  Visual *GetVisualId();
};

#endif


