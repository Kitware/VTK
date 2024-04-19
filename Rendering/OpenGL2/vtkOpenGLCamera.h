// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLCamera
 * @brief   OpenGL camera
 *
 * vtkOpenGLCamera is a concrete implementation of the abstract class
 * vtkCamera.  vtkOpenGLCamera interfaces to the OpenGL rendering library.
 */

#ifndef vtkOpenGLCamera_h
#define vtkOpenGLCamera_h

#include "vtkCamera.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLRenderer;
class vtkMatrix3x3;
class vtkMatrix4x4;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLCamera : public vtkCamera
{
public:
  static vtkOpenGLCamera* New();
  vtkTypeMacro(vtkOpenGLCamera, vtkCamera);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Implement base class method.
   */
  void Render(vtkRenderer* ren) override;

  void UpdateViewport(vtkRenderer* ren) override;

  virtual void GetKeyMatrices(vtkRenderer* ren, vtkMatrix4x4*& WCVCMatrix,
    vtkMatrix3x3*& normalMatrix, vtkMatrix4x4*& VCDCMatrix, vtkMatrix4x4*& WCDCMatrix);

protected:
  vtkOpenGLCamera();
  ~vtkOpenGLCamera() override;

  vtkMatrix4x4* WCDCMatrix;
  vtkMatrix4x4* WCVCMatrix;
  vtkMatrix3x3* NormalMatrix;
  vtkMatrix4x4* VCDCMatrix;
  vtkTimeStamp KeyMatrixTime;
  vtkRenderer* LastRenderer;

private:
  vtkOpenGLCamera(const vtkOpenGLCamera&) = delete;
  void operator=(const vtkOpenGLCamera&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
