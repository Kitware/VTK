/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLLightingPainter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLLightingPainter
 * @brief   painter that manages lighting.
 *
 * This painter manages lighting.
 * Ligting is disabled when rendering points/lines and no normals are present
 * or rendering Polygons/TStrips and representation is points and no normals
 * are present.
*/

#ifndef vtkOpenGLLightingPainter_h
#define vtkOpenGLLightingPainter_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkLightingPainter.h"

class vtkWindow;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLLightingPainter : public vtkLightingPainter
{
public:
  static vtkOpenGLLightingPainter* New();
  vtkTypeMacro(vtkOpenGLLightingPainter, vtkLightingPainter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * This painter overrides GetTimeToDraw() to never pass the request to the
   * delegate. This is done since this class may propagate a single render
   * request multiple times to the delegate. In that case the time accumulation
   * responsibility is borne by the painter causing the multiple rendering
   * requests i.e. this painter itself.
   */
  double GetTimeToDraw() VTK_OVERRIDE
    { return this->TimeToDraw; }

protected:
  vtkOpenGLLightingPainter();
  ~vtkOpenGLLightingPainter() VTK_OVERRIDE;

  /**
   * Setups lighting state before calling render on delegate
   * painter.
   */
  void RenderInternal(vtkRenderer *renderer,
                              vtkActor *actor,
                              unsigned long typeflags,
                              bool forceCompileOnly) VTK_OVERRIDE;

private:
  vtkOpenGLLightingPainter(const vtkOpenGLLightingPainter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOpenGLLightingPainter&) VTK_DELETE_FUNCTION;
};

#endif
