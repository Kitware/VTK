/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMesaTextMapper.h
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
// .NAME vtkXOpenGLTextMapper - 2D Text annotation support for windows
// .SECTION Description
// vtkXMesaTextMapper provides 2D text annotation support for vtk under
// Xwindows.  Normally the user should use vtktextMapper which in turn will
// use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkXMesaTextMapper_h
#define __vtkXMesaTextMapper_h

#include "vtkXTextMapper.h"

class VTK_RENDERING_EXPORT vtkXMesaTextMapper : public vtkXTextMapper
{
public:
  vtkTypeRevisionMacro(vtkXMesaTextMapper,vtkXTextMapper);
  static vtkXMesaTextMapper *New();

  // Description:
  // Actally draw the text.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // An internal function used for caching font display lists.
  static int GetListBaseForFont(vtkTextMapper *tm, vtkViewport *vp,
                                Font);

protected:
  vtkXMesaTextMapper();
  ~vtkXMesaTextMapper() {};
private:
  vtkXMesaTextMapper(const vtkXMesaTextMapper&);  // Not implemented.
  void operator=(const vtkXMesaTextMapper&);  // Not implemented.
};


#endif

