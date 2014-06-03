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
// .NAME vtkOpenGL2ShaderCache - manage Shader Programs within a context
// .SECTION Description
// vtkOpenGL2ShaderCache manages shader program compilation and binding

#ifndef __vtkOpenGL2ShaderCache_h
#define __vtkOpenGL2ShaderCache_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

#include "vtkglShader.h"
#include "vtkglShaderProgram.h"

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGL2ShaderCache : public vtkObject
{
public:
  static vtkOpenGL2ShaderCache *New();
  vtkTypeMacro(vtkOpenGL2ShaderCache, vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // store the program and shaders in a simple struct
  struct CachedShaderProgram
  {
    vtkgl::Shader VS;
    vtkgl::Shader FS;
    vtkgl::ShaderProgram Program;
    bool Compiled;
    std::string md5Hash;
  };

  // make sure the specified shader is compiled, linked, and bound
  virtual CachedShaderProgram *ReadyShader(const char *vertexCode, const char *fragmentCode);
  virtual CachedShaderProgram *ReadyShader(CachedShaderProgram *shader);

protected:
  vtkOpenGL2ShaderCache();
  ~vtkOpenGL2ShaderCache();

  virtual CachedShaderProgram* GetShader(const char *vertexCode, const char *fragmentCode);
  virtual int CompileShader(CachedShaderProgram* shader);
  virtual int BindShader(CachedShaderProgram* shader);

  class Private;
  Private *Internal;
  CachedShaderProgram *LastShaderBound;

private:
  vtkOpenGL2ShaderCache(const vtkOpenGL2ShaderCache&);  // Not implemented.
  void operator=(const vtkOpenGL2ShaderCache&);  // Not implemented.

};

/*
  Shader vs;
  Shader fs;
  const char* vsFile;
  const char* fsFile;
  ShaderProgram program;
  BufferObject ibo;
  VertexArrayObject vao;
  size_t indexCount;
  // These are client side objects for multi draw where IBOs are not used.
  std::vector<ptrdiff_t> offsetArray;
  std::vector<unsigned int> elementsArray;
  vtkTimeStamp buildTime;
  vtkTimeStamp attributeUpdateTime;
*/

#endif
