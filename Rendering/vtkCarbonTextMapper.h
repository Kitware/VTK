/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkCarbonTextMapper.h
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
// .NAME vtkCarbonTextMapper - 2D Text annotation support on OSX-Carbon
// .SECTION Description
// vtkCarbonTextMapper provides 2D text annotation support for vtk under
// OSX-Carbon.  The user should use vtktextMapper which in turn will
// use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkCarbonTextMapper_h
#define __vtkCarbonTextMapper_h

#include "vtkTextMapper.h"
#include <Carbon/Carbon.h>

class VTK_RENDERING_EXPORT vtkCarbonTextMapper : public vtkTextMapper
{
public:
  vtkTypeRevisionMacro(vtkCarbonTextMapper,vtkTextMapper);
  static vtkCarbonTextMapper *New();

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
  virtual int GetListBaseForFont(vtkViewport *vp);

  // Description:
  // What is the size of the rectangle required to draw this
  // mapper ?
  void GetSize(vtkViewport* viewport, int size[2]);  
  
protected:
    vtkCarbonTextMapper();
  ~vtkCarbonTextMapper();


  vtkTimeStamp  BuildTime;
  int LastSize[2];
  FontInfo myFontInfo;
  short currentFontNum; // last used FontNumber (for GLList loading)
  
private:
    vtkCarbonTextMapper(const vtkCarbonTextMapper&);  // Not implemented.
  void operator=(const vtkCarbonTextMapper&);  // Not implemented.
};


#endif

