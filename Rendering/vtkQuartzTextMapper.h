/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzTextMapper.h
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
// .NAME vtkQuartzTextMapper - 2D Text annotation support for windows
// .SECTION Description
// vtkQuartzTextMapper provides 2D text annotation support for vtk under
// Xwindows.  Normally the user should use vtktextMapper which in turn will
// use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkQuartzTextMapper_h
#define __vtkQuartzTextMapper_h

#include "vtkTextMapper.h"

class VTK_RENDERING_EXPORT vtkQuartzTextMapper : public vtkTextMapper
{
public:
  vtkTypeRevisionMacro(vtkQuartzTextMapper,vtkTextMapper);
  static vtkQuartzTextMapper *New();

  // Description:
  // Actally draw the text.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // What is the size of the rectangle required to draw this
  // mapper ?
  void GetSize(vtkViewport* viewport, int size[2]);

protected:
  vtkQuartzTextMapper();
  ~vtkQuartzTextMapper();

  vtkTimeStamp  BuildTime;
  int LastSize[2];
  void *Font;
private:
  vtkQuartzTextMapper(const vtkQuartzTextMapper&) {};  // Not implemented.
  void operator=(const vtkQuartzTextMapper&) {};  // Not implemented.
};


#endif

