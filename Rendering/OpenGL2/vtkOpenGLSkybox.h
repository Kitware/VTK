// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLSkybox
 * @brief   OpenGL Skybox
 *
 * vtkOpenGLSkybox is a concrete implementation of the abstract class vtkSkybox.
 * vtkOpenGLSkybox interfaces to the OpenGL rendering library.
 */

#ifndef vtkOpenGLSkybox_h
#define vtkOpenGLSkybox_h

#include "vtkMatrix3x3.h"
#include "vtkNew.h"                    // for ivars
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkSkybox.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkOpenGLActor;
class vtkOpenGLPolyDataMapper;
class vtkOverrideAttribute;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLSkybox : public vtkSkybox
{
public:
  static vtkOpenGLSkybox* New();
  static vtkOverrideAttribute* CreateOverrideAttributes();
  vtkTypeMacro(vtkOpenGLSkybox, vtkSkybox);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Actual Skybox render method.
   */
  void Render(vtkRenderer* ren, vtkMapper* mapper) override;

  /**
   * Installs an observer on the mapper UpdateShaderEvent
   * that updates uniform values.
   */
  void SetMapper(vtkMapper* mapper) override;

protected:
  vtkOpenGLSkybox();
  ~vtkOpenGLSkybox() override;

  int LastProjection;
  bool LastGammaCorrect;
  float LastCameraPosition[3];

  void UpdateUniforms(vtkObject*, unsigned long, void*);

  vtkNew<vtkOpenGLPolyDataMapper> CubeMapper;
  vtkNew<vtkOpenGLActor> OpenGLActor;
  vtkRenderer* CurrentRenderer;

private:
  vtkOpenGLSkybox(const vtkOpenGLSkybox&) = delete;
  void operator=(const vtkOpenGLSkybox&) = delete;

  vtkNew<vtkMatrix3x3> RotationMatrix;
};

#define vtkOpenGLSkybox_OVERRIDE_ATTRIBUTES vtkOpenGLSkybox::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END
#endif
