// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLActor
 * @brief   OpenGL actor
 *
 * vtkOpenGLActor is a concrete implementation of the abstract class vtkActor.
 * vtkOpenGLActor interfaces to the OpenGL rendering library.
 */

#ifndef vtkOpenGLActor_h
#define vtkOpenGLActor_h

#include "vtkActor.h"
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationIntegerKey;
class vtkOpenGLRenderer;
class vtkMatrix4x4;
class vtkMatrix3x3;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLActor : public vtkActor
{
public:
  static vtkOpenGLActor* New();
  vtkTypeMacro(vtkOpenGLActor, vtkActor);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual actor render method.
   */
  void Render(vtkRenderer* ren, vtkMapper* mapper) override;

  virtual void GetKeyMatrices(vtkMatrix4x4*& WCVCMatrix, vtkMatrix3x3*& normalMatrix);

  /**
   * If this key is set in GetPropertyKeys(), the glDepthMask will be adjusted
   * prior to rendering translucent objects. This is useful for e.g. depth
   * peeling.

   * If GetIsOpaque() == true, the depth mask is always enabled, regardless of
   * this key. Otherwise, the depth mask is disabled for default alpha blending
   * unless this key is set.

   * If this key is set, the integer value has the following meanings:
   * 0: glDepthMask(GL_FALSE)
   * 1: glDepthMask(GL_TRUE)
   * Anything else: No change to depth mask.
   */
  static vtkInformationIntegerKey* GLDepthMaskOverride();

protected:
  vtkOpenGLActor();
  ~vtkOpenGLActor() override;

  vtkMatrix4x4* MCWCMatrix;
  vtkMatrix3x3* NormalMatrix;
  vtkTransform* NormalTransform;
  vtkTimeStamp KeyMatrixTime;

private:
  vtkOpenGLActor(const vtkOpenGLActor&) = delete;
  void operator=(const vtkOpenGLActor&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
