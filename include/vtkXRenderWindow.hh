/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindow.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkXRenderWindow - a rendering window for the X Window system
// .SECTION Description
// vtkXRenderWindow is a subclass of the abstract class
// vtkRenderWindow. vtkXRenderWindow interfaces to the X Window system and
// provides some methods that are common to any vtkRenderWindow
// subclass that renders under X Windows. The vtkXRenderWindowInteractor
// makes heavy use of these common methods.

// .SECTION see also
// vtkRenderWindow vtkXRenderWindowInteractor

#ifndef __vtkXRenderWindow_hh
#define __vtkXRenderWindow_hh

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkRenderWindow.hh"

class vtkXRenderWindow : public vtkRenderWindow
{
public:
  vtkXRenderWindow();
  ~vtkXRenderWindow();
  char *GetClassName() {return "vtkXRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // supply base class virtual function
  vtkRenderWindowInteractor *MakeRenderWindowInteractor();

  // Xwindow get set functions
  int     *GetSize();
  int     *GetScreenSize();
  int     *GetPosition();
  Display *GetDisplayId();
  void     SetDisplayId(Display *);
  void     SetDisplayId(void *);
  Window   GetWindowId();
  void     SetWindowId(Window);
  void     SetWindowId(void *);
  void     SetNextWindowId(Window);
  virtual int      GetDesiredDepth()    = 0;
  virtual Colormap GetDesiredColormap() = 0;
  virtual Visual  *GetDesiredVisual()   = 0;

protected:
  Window   WindowId;
  Window   NextWindowId;
  Display *DisplayId;
  Colormap ColorMap;
  int      OwnWindow;
  int      ScreenSize[2];

};

#endif
