/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOpenGLQuadHelper
 * @brief   Class to make rendering a full screen quad easier
 *
 * vtkOpenGLQuadHelper is designed to be used by classes in VTK that
 * need to render a quad to the screen with a shader program. This happens
 * often with render passes or other advanced rendering techniques.
 *
 * Note that when releasing graphics resources instances of this
 * class should be destroyed. A common use pattern is to conditionally
 * create the instance where used and delete it in ReleaseGraphicsResources
 * and the destructor.
 *
 * Example usage:
 * @code
 * if (!this->QuadHelper)
 * {
 *   this->QuadHelper = vtkOpenGLQualHelper(renWin, vs, fs, gs);
 * }
 * renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
 * aTexture->Activate();
 * this->QuadHelper->Program->SetUniformi("aTexture", aTexture->GetTextureUnit());
 * this->QuadHelper->Render();
 * aTexture->Deactivate();
 * @endcode
 *
 * @sa vtkOpenGLRenderUtilities
 */

#ifndef vtkOpenGLQuadHelper_h
#define vtkOpenGLQuadHelper_h

#include "vtkRenderingOpenGL2Module.h" // for export macro
#include "vtkTimeStamp.h"
#include <memory> // for std::unique_ptr

class vtkOpenGLRenderWindow;
class vtkOpenGLVertexArrayObject;
class vtkShaderProgram;
class vtkGenericOpenGLResourceFreeCallback;
class vtkWindow;

// Helper class to render full screen quads
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLQuadHelper
{
public:
  vtkShaderProgram* Program;
  vtkTimeStamp ShaderSourceTime;
  vtkOpenGLVertexArrayObject* VAO;
  unsigned int ShaderChangeValue;

  // create a quadhelper with the provided shaders
  // if the vertex is nullptr
  // then the default is used. Note that this
  // class should be destroyed upon
  // ReleaseGraphicsResources
  vtkOpenGLQuadHelper(vtkOpenGLRenderWindow*, const char* vs, const char* fs, const char* gs);

  ~vtkOpenGLQuadHelper();

  // Draw the Quad, will bind the VAO for you
  void Render();

  /**
   * Release graphics resources. In general, there's no need to call this
   * explicitly, since vtkOpenGLQuadHelper will invoke it appropriately when
   * needed.
   */
  void ReleaseGraphicsResources(vtkWindow*);

private:
  vtkOpenGLQuadHelper(const vtkOpenGLQuadHelper&) = delete;
  vtkOpenGLQuadHelper& operator=(const vtkOpenGLQuadHelper&) = delete;
  std::unique_ptr<vtkGenericOpenGLResourceFreeCallback> ResourceCallback;
};

#endif // vtkOpenGLQuadHelper_h

// VTK-HeaderTest-Exclude: vtkOpenGLQuadHelper.h
