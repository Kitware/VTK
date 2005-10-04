/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLDisplayListPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLDisplayListPainter - display list painter using OpenGL.
// .SECTION Description
// Note that this painter builds separate display lists for each
// type i.e. verts,lines,polys,tstrips.

#ifndef __vtkOpenGLDisplayListPainter_h
#define __vtkOpenGLDisplayListPainter_h

#include "vtkDisplayListPainter.h"

class VTK_RENDERING_EXPORT vtkOpenGLDisplayListPainter : public vtkDisplayListPainter
{
public:
  static vtkOpenGLDisplayListPainter* New();
  vtkTypeRevisionMacro(vtkOpenGLDisplayListPainter, vtkDisplayListPainter);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release. In this case, releases the display lists.
  virtual void ReleaseGraphicsResources(vtkWindow *);
protected:
  vtkOpenGLDisplayListPainter();
  ~vtkOpenGLDisplayListPainter();

  unsigned int ListIds[4];
  vtkTimeStamp BuildTimes[4];

  void ReleaseList(int index);

  // Description:
  // If not using ImmediateModeRendering, this will build a display list,
  // if outdated and use the display list.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor, 
    unsigned long typeflags);

private:
  vtkOpenGLDisplayListPainter(const vtkOpenGLDisplayListPainter&); // Not implemented.
  void operator=(const vtkOpenGLDisplayListPainter&); // Not implemented.
};

#endif

