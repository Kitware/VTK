// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLBatchedLabeledDataMapper
 * @brief   OpenGL backend for vtkBatchedLabeledDataMapper
 *
 * OpenGL implementation of vtkBatchedLabeledDataMapper. Renders all labels in a
 * single batched draw call using a glyph atlas texture and a geometry shader.
 *
 * @sa
 * vtkBatchedLabeledDataMapper
 */

#ifndef vtkOpenGLBatchedLabeledDataMapper_h
#define vtkOpenGLBatchedLabeledDataMapper_h

#include "vtkBatchedLabeledDataMapper.h"
#include "vtkNew.h"                    // For vtkNew
#include "vtkRenderingOpenGL2Module.h" // For export macro
#include "vtkWrappingHints.h"          // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkOpenGLBatchedLabeledDataMapperInternals;
class vtkOpenGLRenderWindow;
class vtkOverrideAttribute;
class vtkTextureObject;

class VTKRENDERINGOPENGL2_EXPORT VTK_MARSHALAUTO vtkOpenGLBatchedLabeledDataMapper
  : public vtkBatchedLabeledDataMapper
{
public:
  static vtkOpenGLBatchedLabeledDataMapper* New();
  vtkTypeMacro(vtkOpenGLBatchedLabeledDataMapper, vtkBatchedLabeledDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Returns an override-attribute chain that selects this class for the OpenGL backend.
  static vtkOverrideAttribute* CreateOverrideAttributes();

  using vtkBatchedLabeledDataMapper::SetLabelTextProperty;
  /// Calls the superclass and invalidates the helper's VBO attribute mapping so
  /// it is rebuilt on the next render with the new property's glyph atlas layout.
  void SetLabelTextProperty(vtkTextProperty* p, int type) override;

  /// Rebuilds labels and the glyph atlas if the pipeline is stale, then issues
  /// the batched draw call through the internal OpenGL helper.
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor) override;
  /// Releases the atlas texture object and the helper's GPU resources.
  void ReleaseGraphicsResources(vtkWindow*) override;

protected:
  vtkOpenGLBatchedLabeledDataMapper();
  ~vtkOpenGLBatchedLabeledDataMapper() override;

  void UploadGlyphAtlas(vtkImageData* atlas) override;
  void ActivateGlyphTexture() override;
  void DeactivateGlyphTexture() override;

private:
  vtkOpenGLBatchedLabeledDataMapper(const vtkOpenGLBatchedLabeledDataMapper&) = delete;
  void operator=(const vtkOpenGLBatchedLabeledDataMapper&) = delete;

  vtkNew<vtkTextureObject> GlyphsTO;
  vtkNew<vtkOpenGLBatchedLabeledDataMapperInternals> Helper;
  vtkNew<vtkActor> DummyActor;

  friend class vtkOpenGLBatchedLabeledDataMapperInternals;

  void SetupHelper();
  bool HelperSetup = false;
};

#define vtkOpenGLBatchedLabeledDataMapper_OVERRIDE_ATTRIBUTES                                      \
  vtkOpenGLBatchedLabeledDataMapper::CreateOverrideAttributes()
VTK_ABI_NAMESPACE_END

#endif
