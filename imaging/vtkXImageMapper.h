/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXImageMapper.h
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
// .NAME vtkXImageMapper - 2D image display support for X windows
// .SECTION Description
// vtkXImageMapper is a concrete subclass of vtkImageMapper that
// renders images under X windows.

// .SECTION See Also
// vtkImageMapper

#ifndef __vtkXImageMapper_h
#define __vtkXImageMapper_h

#include "vtkImageMapper.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkImageData.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/X.h>
#include <X11/keysym.h>

class vtkActor2D;

class VTK_EXPORT vtkXImageMapper : public vtkImageMapper
{
public:
  vtkXImageMapper();
  ~vtkXImageMapper();

  static vtkXImageMapper *New() {return new vtkXImageMapper;};

  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data, vtkActor2D* actor);

  // Description:
  // Returns the X specific compositing operator.
  int GetCompositingMode(vtkActor2D* actor);

  // Description:
  // Returns the depth of the X window
  int GetXWindowDepth(vtkWindow* window);

  // Description:
  // Returns the visual id of the window
  void GetXWindowVisualId(vtkWindow* window, Visual* visualID);

  // Description:
  // Returns the visual class of the window
  int GetXWindowVisualClass(vtkWindow* window);
 
  // Description:
  // Returns the color map used by the window
  void GetXWindowColors(vtkWindow* window, XColor colors[], int ncolors);

  // Description:
  // Returns the color masks used by the window.
  void GetXWindowColorMasks(vtkWindow *window, unsigned long *rmask,
      			    unsigned long *gmask, unsigned long *bmask);

  // Description:
  // Gets the number of colors in the pseudo color map.
  vtkGetMacro(NumberOfColors,int);

protected:

  XImage          *Image;
  unsigned char   *DataOut;
  int             DataOutSize;
  int             NumberOfColors;
};


#endif




