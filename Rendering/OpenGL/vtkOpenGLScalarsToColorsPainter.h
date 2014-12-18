/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLScalarsToColorsPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLScalarsToColorsPainter - implementation of
// vtkScalarsToColorsPainter for OpenGL.
// .SECTION Description
// vtkOpenGLScalarsToColorsPainter is a concrete subclass of
// vtkScalarsToColorsPainter which uses OpenGL for color mapping.

#ifndef vtkOpenGLScalarsToColorsPainter_h
#define vtkOpenGLScalarsToColorsPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkScalarsToColorsPainter.h"

class vtkOpenGLTexture;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLScalarsToColorsPainter :
  public vtkScalarsToColorsPainter
{
public:
  static vtkOpenGLScalarsToColorsPainter* New();
  vtkTypeMacro(vtkOpenGLScalarsToColorsPainter,
    vtkScalarsToColorsPainter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  virtual int GetPremultiplyColorsWithAlpha(vtkActor* actor);

  // Description:
  // Return the texture size limit, i.e. GL_MAX_TEXTURE_SIZE.
  virtual vtkIdType GetTextureSizeLimit();

protected:
  vtkOpenGLScalarsToColorsPainter();
  ~vtkOpenGLScalarsToColorsPainter();

  vtkOpenGLTexture* InternalColorTexture;
  int AlphaBitPlanes;
  bool AcquiredGraphicsResources;
  bool SupportsSeparateSpecularColor;

  // Description:
  // Generates rendering primitives of appropriate type(s). Multiple types
  // of preimitives can be requested by or-ring the primitive flags.
  // Subclasses may override this method. Default implementation propagates
  // the call to Deletegate Painter, in any.
  virtual void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly);

private:
  vtkOpenGLScalarsToColorsPainter(const vtkOpenGLScalarsToColorsPainter&); // Not implemented.
  void operator=(const vtkOpenGLScalarsToColorsPainter&); // Not implemented.
};

#endif
