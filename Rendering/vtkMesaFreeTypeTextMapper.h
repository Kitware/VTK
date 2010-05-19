/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaFreeTypeTextMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaFreeTypeTextMapper - 2D Text annotation support
// .SECTION Description
// vtkMesaFreeTypeTextMapper provides 2D text annotation support for VTK
// using the FreeType and FTGL libraries. Normally the user should use 
// vtktextMapper which in turn will use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef __vtkMesaFreeTypeTextMapper_h
#define __vtkMesaFreeTypeTextMapper_h

#include "vtkTextMapper.h"

//BTX
class FTFont;
//ETX

class VTK_RENDERING_EXPORT vtkMesaFreeTypeTextMapper : public vtkTextMapper
{
public:
  vtkTypeMacro(vtkMesaFreeTypeTextMapper,vtkTextMapper);
  static vtkMesaFreeTypeTextMapper *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Actally draw the text.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // What is the size of the rectangle required to draw this
  // mapper ?
  virtual void GetSize(vtkViewport* viewport, int size[2]);

protected:
  vtkMesaFreeTypeTextMapper();
  ~vtkMesaFreeTypeTextMapper();

  vtkTimeStamp  SizeBuildTime;
  int LastSize[2];
  int LastLargestDescender;

private:
  vtkMesaFreeTypeTextMapper(const vtkMesaFreeTypeTextMapper&);  // Not implemented.
  void operator=(const vtkMesaFreeTypeTextMapper&);  // Not implemented.
};

#endif
