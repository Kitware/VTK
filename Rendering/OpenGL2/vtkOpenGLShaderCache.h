// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLShaderCache
 * @brief   manage Shader Programs within a context
 *
 * vtkOpenGLShaderCache manages shader program compilation and binding
 */

#ifndef vtkOpenGLShaderCache_h
#define vtkOpenGLShaderCache_h

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkShader.h"                 // for vtkShader::Type
#include <map>                         // for methods

VTK_ABI_NAMESPACE_BEGIN
class vtkTransformFeedback;
class vtkShaderProgram;
class vtkWindow;
class vtkOpenGLRenderWindow;

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLShaderCache : public vtkObject
{
public:
  static vtkOpenGLShaderCache* New();
  vtkTypeMacro(vtkOpenGLShaderCache, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get whether the GLSL version macro in the shader must be the same as OpenGL version.
   * When true, the `#version xyz` macro is defined such that:
   *  x = OpenGLMajorVersion
   *  y = OpenGLMinorVersion
   *  z = 0
   */
  vtkSetMacro(SyncGLSLShaderVersion, bool);
  vtkGetMacro(SyncGLSLShaderVersion, bool);
  vtkBooleanMacro(SyncGLSLShaderVersion, bool);
  ///@}

  // make sure the specified shaders are compiled, linked, and bound
  virtual vtkShaderProgram* ReadyShaderProgram(const char* vertexCode, const char* fragmentCode,
    const char* geometryCode, vtkTransformFeedback* cap = nullptr);
  virtual vtkShaderProgram* ReadyShaderProgram(const char* vertexCode, const char* fragmentCode,
    const char* geometryCode, const char* tessControlCode, const char* tessEvalCode,
    vtkTransformFeedback* cap = nullptr);

  // make sure the specified shaders are compiled, linked, and bound
  // will increment the reference count on the shaders if it
  // needs to keep them around
  virtual vtkShaderProgram* ReadyShaderProgram(
    std::map<vtkShader::Type, vtkShader*> shaders, vtkTransformFeedback* cap = nullptr);

  // make sure the specified shaders are compiled, linked, and bound
  virtual vtkShaderProgram* ReadyShaderProgram(
    vtkShaderProgram* shader, vtkTransformFeedback* cap = nullptr);

  /**
   * Release the current shader.  Basically go back to
   * having no shaders loaded.  This is useful for old
   * legacy code that relies on no shaders being loaded.
   */
  void ReleaseCurrentShader();

  /**
   * Free up any resources being used by the provided shader
   */
  virtual void ReleaseGraphicsResources(vtkWindow* win);

  /**
   * Get/Clear the last Shader bound, called by shaders as they release
   * their graphics resources
   */
  virtual void ClearLastShaderBound() { this->LastShaderBound = nullptr; }
  vtkGetObjectMacro(LastShaderBound, vtkShaderProgram);

  // Set the time in seconds elapsed since the first render
  void SetElapsedTime(float val) { this->ElapsedTime = val; }

protected:
  vtkOpenGLShaderCache();
  ~vtkOpenGLShaderCache() override;

  // perform System and Output replacements in place. Returns
  // the number of outputs
  virtual unsigned int ReplaceShaderValues(std::string& VSSource, std::string& FSSource,
    std::string& GSSource, std::string& TCSSource, std::string& TESSource);

  virtual vtkShaderProgram* GetShaderProgram(const char* vertexCode, const char* fragmentCode,
    const char* geometryCode, const char* tessControlCode, const char* tessEvalCode);
  virtual vtkShaderProgram* GetShaderProgram(std::map<vtkShader::Type, vtkShader*> shaders);
  virtual int BindShader(vtkShaderProgram* shader);

  class Private;
  Private* Internal;
  vtkShaderProgram* LastShaderBound;

  int OpenGLMajorVersion;
  int OpenGLMinorVersion;
  bool SyncGLSLShaderVersion;

  float ElapsedTime;

private:
  vtkOpenGLShaderCache(const vtkOpenGLShaderCache&) = delete;
  void operator=(const vtkOpenGLShaderCache&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
