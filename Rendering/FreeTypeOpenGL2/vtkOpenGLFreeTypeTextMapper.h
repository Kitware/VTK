/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFreeTypeTextMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLFreeTypeTextMapper - 2D Text annotation support
// .SECTION Description
// vtkOpenGLFreeTypeTextMapper provides 2D text annotation support for VTK
// using the FreeType and FTGL libraries. Normally the user should use
// vtktextMapper which in turn will use this class.

// .SECTION See Also
// vtkTextMapper

#ifndef vtkOpenGLFreeTypeTextMapper_h
#define vtkOpenGLFreeTypeTextMapper_h

#include "vtkRenderingFreeTypeOpenGL2Module.h" // For export macro
#include "vtkTextMapper.h"

class VTKRENDERINGFREETYPEOPENGL2_EXPORT vtkOpenGLFreeTypeTextMapper
  : public vtkTextMapper
{
public:
  vtkTypeMacro(vtkOpenGLFreeTypeTextMapper, vtkTextMapper);
  static vtkOpenGLFreeTypeTextMapper *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void SetInput(const char *inputString);

  // Description:
  // Actally draw the text.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor);

  // Description:
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Return the the size of the rectangle required to draw this
  // mapper.
  virtual void GetSize(vtkViewport* viewport, int size[2]);

protected:
  vtkOpenGLFreeTypeTextMapper();
  ~vtkOpenGLFreeTypeTextMapper();

  vtkTimeStamp  SizeBuildTime;
  int LastSize[2];
  int LastLargestDescender;
  int LineSize;
  int NumberOfLines;
  int NumberOfLinesAllocated;
  vtkTextMapper **TextLines;

  // Description:
  // These functions are used to parse, process, and render multiple lines
  char *NextLine(const char *input, int lineNum);
  void GetMultiLineSize(vtkViewport* viewport, int size[2]);
  void RenderOverlayMultipleLines(vtkViewport *viewport, vtkActor2D *actor);

private:
  vtkOpenGLFreeTypeTextMapper(const vtkOpenGLFreeTypeTextMapper&);  // Not implemented.
  void operator=(const vtkOpenGLFreeTypeTextMapper&);  // Not implemented.
};

#endif
