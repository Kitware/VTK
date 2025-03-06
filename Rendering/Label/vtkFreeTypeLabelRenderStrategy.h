// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFreeTypeLabelRenderStrategy
 * @brief   Renders labels with freetype
 *
 *
 * Uses the FreeType to render labels and compute label sizes.
 * This strategy may be used with vtkLabelPlacementMapper.
 */

#ifndef vtkFreeTypeLabelRenderStrategy_h
#define vtkFreeTypeLabelRenderStrategy_h

#include "vtkLabelRenderStrategy.h"
#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkActor2D;
class vtkTextRenderer;
class vtkTextMapper;

class VTKRENDERINGLABEL_EXPORT VTK_MARSHALAUTO vtkFreeTypeLabelRenderStrategy
  : public vtkLabelRenderStrategy
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkFreeTypeLabelRenderStrategy, vtkLabelRenderStrategy);
  static vtkFreeTypeLabelRenderStrategy* New();

  /**
   * The free type render strategy currently does not support rotation.
   */
  bool SupportsRotation() override { return false; }

  /**
   * The free type render strategy currently does not support bounded size labels.
   */
  bool SupportsBoundedSize() override { return false; }

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   */
  void ComputeLabelBounds(vtkTextProperty* tprop, vtkStdString label, double bds[4]) override;

  using vtkLabelRenderStrategy::RenderLabel;
  /**
   * Render a label at a location in world coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   */
  void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label) override;

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkFreeTypeLabelRenderStrategy();
  ~vtkFreeTypeLabelRenderStrategy() override;

  vtkTextRenderer* TextRenderer;
  vtkTextMapper* Mapper;
  vtkActor2D* Actor;

private:
  vtkFreeTypeLabelRenderStrategy(const vtkFreeTypeLabelRenderStrategy&) = delete;
  void operator=(const vtkFreeTypeLabelRenderStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
