/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLShaderCache - manage Shader Programs within a context
// .SECTION Description
// vtkOpenGLShaderCache manages shader program compilation and binding

#ifndef __vtkOpenGLShaderCache_h
#define __vtkOpenGLShaderCache_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

#include "vtkglShader.h"
#include "vtkglShaderProgram.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLShaderCache : public vtkObject
{
public:
  static vtkOpenGLShaderCache *New();
  vtkTypeMacro(vtkOpenGLShaderCache, vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // store the program and shaders in a simple struct
  struct CachedShaderProgram
  {
    vtkgl::Shader VS;
    vtkgl::Shader FS;
    vtkgl::Shader GS;
    vtkgl::ShaderProgram Program;
    bool Compiled;
    std::string md5Hash;
  };

  // make sure the specified shader is compiled, linked, and bound
  virtual CachedShaderProgram *ReadyShader(const char *vertexCode,
                                           const char *fragmentCode,
                                           const char *geometryCode);
  virtual CachedShaderProgram *ReadyShader(CachedShaderProgram *shader);

protected:
  vtkOpenGLShaderCache();
  ~vtkOpenGLShaderCache();

  virtual CachedShaderProgram* GetShader(const char *vertexCode,
                                         const char *fragmentCode,
                                         const char *geometryCode);
  virtual int CompileShader(CachedShaderProgram* shader);
  virtual int BindShader(CachedShaderProgram* shader);

  class Private;
  Private *Internal;
  CachedShaderProgram *LastShaderBound;

private:
  vtkOpenGLShaderCache(const vtkOpenGLShaderCache&);  // Not implemented.
  void operator=(const vtkOpenGLShaderCache&);  // Not implemented.

};

#endif
