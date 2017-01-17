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
/**
 * @class   vtkOpenGLScalarsToColorsPainter
 * @brief   implementation of
 * vtkScalarsToColorsPainter for OpenGL.
 *
 * vtkOpenGLScalarsToColorsPainter is a concrete subclass of
 * vtkScalarsToColorsPainter which uses OpenGL for color mapping.
*/

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  int GetPremultiplyColorsWithAlpha(vtkActor* actor) VTK_OVERRIDE;

  /**
   * Return the texture size limit, i.e. GL_MAX_TEXTURE_SIZE.
   */
  vtkIdType GetTextureSizeLimit() VTK_OVERRIDE;

protected:
  vtkOpenGLScalarsToColorsPainter();
  ~vtkOpenGLScalarsToColorsPainter() VTK_OVERRIDE;

  vtkOpenGLTexture* InternalColorTexture;
  int AlphaBitPlanes;
  bool AcquiredGraphicsResources;
  bool SupportsSeparateSpecularColor;

  /**
   * Generates rendering primitives of appropriate type(s). Multiple types
   * of preimitives can be requested by or-ring the primitive flags.
   * Subclasses may override this method. Default implementation propagates
   * the call to Deletegate Painter, in any.
   */
  void RenderInternal(vtkRenderer* renderer, vtkActor* actor,
                              unsigned long typeflags, bool forceCompileOnly) VTK_OVERRIDE;

private:
  vtkOpenGLScalarsToColorsPainter(const vtkOpenGLScalarsToColorsPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLScalarsToColorsPainter&) VTK_DELETE_FUNCTION;
};

#endif
