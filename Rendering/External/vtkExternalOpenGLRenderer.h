/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExternalOpenGLRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExternalOpenGLRenderer - OpenGL renderer
// .SECTION Description
// vtkExternalOpenGLRenderer is a secondary implementation of the class
// vtkOpenGLRenderer. vtkExternalOpenGLRenderer interfaces to the
// OpenGL graphics library. This class provides API to preserve the color and
// depth buffers, thereby allowing external applications to manage the OpenGL
// buffers. This becomes very useful when there are multiple OpenGL applications
// sharing the same OpenGL context.
//
// vtkExternalOpenGLRenderer makes sure that the camera used in the scene if of
// type vtkExternalOpenGLCamera. It manages light and camera transformations for
// VTK objects in the OpenGL context.
//
// \sa vtkExternalOpenGLCamera

#ifndef __vtkExternalOpenGLRenderer_h
#define __vtkExternalOpenGLRenderer_h

#include "vtkRenderingExternalModule.h" // For export macro
#include "vtkOpenGLRenderer.h"

class VTKRENDERINGEXTERNAL_EXPORT vtkExternalOpenGLRenderer :
  public vtkOpenGLRenderer
{
public:
  static vtkExternalOpenGLRenderer *New();
  vtkTypeMacro(vtkExternalOpenGLRenderer, vtkOpenGLRenderer);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Implementation for the Clear method that clears the buffer if requested
  void Clear(void);

  // Description:
  // Synchronize camera and light parameters
  void Render(void);

  // Description:
  // Normally the vtkOpenGLRenderer clears the color buffer before rendering a
  // new frame. When this flag is true, the color buffer is not cleared. This
  // can be helpful when there are multiple visualization systems
  // sharing the same context. Default value is 1.
  vtkGetMacro(PreserveColorBuffer, int);
  vtkSetMacro(PreserveColorBuffer, int);
  vtkBooleanMacro(PreserveColorBuffer, int);

  // Description:
  // Create a new Camera sutible for use with this type of Renderer.
  // This function creates the vtkExternalOpenGLCamera.
  vtkCamera* MakeCamera();

protected:
  vtkExternalOpenGLRenderer();
  ~vtkExternalOpenGLRenderer();

  int PreserveColorBuffer;

private:
  vtkExternalOpenGLRenderer(const vtkExternalOpenGLRenderer&);  // Not implemented.
  void operator=(const vtkExternalOpenGLRenderer&);  // Not implemented.
};

#endif //__vtkExternalOpenGLRenderer_h
