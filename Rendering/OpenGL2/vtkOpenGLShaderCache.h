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

#ifndef vtkOpenGLShaderCache_h
#define vtkOpenGLShaderCache_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkObject.h"

class vtkShader;
class vtkShaderProgram;
class vtkWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLShaderCache : public vtkObject
{
public:
  static vtkOpenGLShaderCache *New();
  vtkTypeMacro(vtkOpenGLShaderCache, vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // make sure the specified shader is compiled, linked, and bound
  virtual vtkShaderProgram *ReadyShader(const char *vertexCode,
                                           const char *fragmentCode,
                                           const char *geometryCode);
  virtual vtkShaderProgram *ReadyShader(vtkShaderProgram *shader);

  // Description:
  // Release the current shader.  Basically go back to
  // having no shaders loaded.  This is useful for old
  // legacy code that relies on no shaders being loaded.
  void ReleaseCurrentShader();

  // Description:
  // Free up any resources being used by the provided shader
  virtual void ReleaseGraphicsResources(vtkWindow *win);

  // Description:
  // Get/Clear the last Shader bound, called by shaders as they release
  // their graphics resources
  virtual void ClearLastShaderBound() { this->LastShaderBound = NULL; }
  vtkGetObjectMacro(LastShaderBound, vtkShaderProgram);

protected:
  vtkOpenGLShaderCache();
  ~vtkOpenGLShaderCache();

  virtual vtkShaderProgram* GetShader(const char *vertexCode,
                                      const char *fragmentCode,
                                      const char *geometryCode);
  virtual int BindShader(vtkShaderProgram* shader);

  class Private;
  Private *Internal;
  vtkShaderProgram *LastShaderBound;

private:
  vtkOpenGLShaderCache(const vtkOpenGLShaderCache&);  // Not implemented.
  void operator=(const vtkOpenGLShaderCache&);  // Not implemented.

};

#endif
