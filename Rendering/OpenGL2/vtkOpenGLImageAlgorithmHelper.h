// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLImageAlgorithmHelper
 * @brief   Help image algorithms use the GPU
 *
 * Designed to make it easier to accelerate an image algorithm on the GPU
 */

#ifndef vtkOpenGLImageAlgorithmHelper_h
#define vtkOpenGLImageAlgorithmHelper_h

#include "vtkObject.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro

#include "vtkOpenGLHelper.h" // used for ivars
#include "vtkSmartPointer.h" // for ivar

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderWindow;
class vtkRenderWindow;
class vtkImageData;
class vtkDataArray;

class vtkOpenGLImageAlgorithmCallback
{
public:
  virtual void InitializeShaderUniforms(vtkShaderProgram* /* program */) {}
  virtual void UpdateShaderUniforms(vtkShaderProgram* /* program */, int /* zExtent */) {}
  virtual ~vtkOpenGLImageAlgorithmCallback() = default;
  vtkOpenGLImageAlgorithmCallback() = default;

private:
  vtkOpenGLImageAlgorithmCallback(const vtkOpenGLImageAlgorithmCallback&) = delete;
  void operator=(const vtkOpenGLImageAlgorithmCallback&) = delete;
};

class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLImageAlgorithmHelper : public vtkObject
{
public:
  static vtkOpenGLImageAlgorithmHelper* New();
  vtkTypeMacro(vtkOpenGLImageAlgorithmHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Execute(vtkOpenGLImageAlgorithmCallback* cb, vtkImageData* inImage, vtkDataArray* inData,
    vtkImageData* outData, int outExt[6], const char* vertexCode, const char* fragmentCode,
    const char* geometryCode);

  /**
   * Set the render window to get the OpenGL resources from
   */
  void SetRenderWindow(vtkRenderWindow* renWin);

protected:
  vtkOpenGLImageAlgorithmHelper();
  ~vtkOpenGLImageAlgorithmHelper() override;

  vtkSmartPointer<vtkOpenGLRenderWindow> RenderWindow;
  vtkOpenGLHelper Quad;

private:
  vtkOpenGLImageAlgorithmHelper(const vtkOpenGLImageAlgorithmHelper&) = delete;
  void operator=(const vtkOpenGLImageAlgorithmHelper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
