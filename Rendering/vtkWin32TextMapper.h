/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32TextMapper.h
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
// .NAME vtkWin32TextMapper - 2D Text annotation support for windows
// .SECTION Description
// vtkWin32TextMapper provides 2D text annotation support for vtk under
// Xwindows.  Normally the user should use vtktextMapper which in turn will
// use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkWin32TextMapper_h
#define __vtkWin32TextMapper_h

#include "vtkTextMapper.h"

class VTK_RENDERING_EXPORT vtkWin32TextMapper : public vtkTextMapper
{
public:
  vtkTypeRevisionMacro(vtkWin32TextMapper,vtkTextMapper);
  static vtkWin32TextMapper *New();

  // Description:
  // What is the size of the rectangle required to draw this
  // mapper ?
  void GetSize(vtkViewport* viewport, int size[2]);

protected:
  vtkWin32TextMapper();
  ~vtkWin32TextMapper();

  vtkTimeStamp  BuildTime;
  int LastSize[2];
  HFONT Font;
private:
  vtkWin32TextMapper(const vtkWin32TextMapper&);  // Not implemented.
  void operator=(const vtkWin32TextMapper&);  // Not implemented.
};


#endif

