/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTextActor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkOpenGLTextActor
 * @brief   vtkTextActor override.
*/

#ifndef vtkOpenGLTextActor_h
#define vtkOpenGLTextActor_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkTextActor.h"

class vtkOpenGLGL2PSHelper;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLTextActor: public vtkTextActor
{
public:
  static vtkOpenGLTextActor* New();
  vtkTypeMacro(vtkOpenGLTextActor, vtkTextActor)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  int RenderOverlay(vtkViewport* viewport) override;

protected:
  vtkOpenGLTextActor();
  ~vtkOpenGLTextActor() override;

  int RenderGL2PS(vtkViewport *viewport, vtkOpenGLGL2PSHelper *gl2ps);

private:
  vtkOpenGLTextActor(const vtkOpenGLTextActor&) = delete;
  void operator=(const vtkOpenGLTextActor&) = delete;
};

#endif // vtkOpenGLTextActor_h
