/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXTextMapper.h
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
// .NAME vtkXTextMapper - 2D Text annotation support for X
// .SECTION Description
// vtkXTextMapper provides 2D text annotation support for vtx under Xwindows.
// Normally the user should use vtktextMapper which in turn will use
// this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkXTextMapper_h
#define __vtkXTextMapper_h

#include "vtkTextMapper.h"

#include        <X11/Xlib.h>
#include        <X11/Xutil.h>
#include        <X11/cursorfont.h>
#include        <X11/X.h>
#include        <X11/keysym.h>


class VTK_RENDERING_EXPORT vtkXTextMapper : public vtkTextMapper
{
public:
  vtkTypeRevisionMacro(vtkXTextMapper,vtkTextMapper);
  static vtkXTextMapper *New();

  // Description:
  // What is the size of the rectangle required to draw this
  // mapper ?
  void GetSize(vtkViewport* viewport, int size[2]);

  // Description:
  // Get the available system font size matching a font size.
  virtual int GetSystemFontSize(int size);

protected:
  vtkXTextMapper();
  ~vtkXTextMapper() {};

  // Description:
  // Actually get the size of the rectangle.
  void DetermineSize(vtkViewport *viewport, int size[2]);

  Font CurrentFont;

  // Size of the Text.
  vtkTimeStamp  SizeMTime;
  int           Size[2];
  int           ViewportSize[2];
private:
  vtkXTextMapper(const vtkXTextMapper&);  // Not implemented.
  void operator=(const vtkXTextMapper&);  // Not implemented.
};




#endif



