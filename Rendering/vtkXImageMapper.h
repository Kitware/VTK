/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXImageMapper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

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

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_RENDERING_EXPORT vtkXImageMapper : public vtkImageMapper
{
public:
  static vtkXImageMapper *New();

  vtkTypeRevisionMacro(vtkXImageMapper,vtkImageMapper);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Handle the render method.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) {
    this->RenderStart(viewport,actor);}

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data, vtkActor2D* actor);

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
  // Returns a pseudo color mapping from 0 255 to 50 199
  void GetXColors(int colors[]);

  // Description:
  // Returns the color masks used by the window.
  void GetXWindowColorMasks(vtkWindow *window, unsigned long *rmask,
                            unsigned long *gmask, unsigned long *bmask);

  // Description:
  // Gets the number of colors in the pseudo color map.
  vtkGetMacro(NumberOfColors,int);

protected:
  vtkXImageMapper();
  ~vtkXImageMapper();

  XImage          *Image;
  unsigned char   *DataOut;
  int             DataOutSize;
  int             NumberOfColors;
private:
  vtkXImageMapper(const vtkXImageMapper&);  // Not implemented.
  void operator=(const vtkXImageMapper&);  // Not implemented.
};
#endif

#endif




