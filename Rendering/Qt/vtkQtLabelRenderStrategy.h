// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkQtLabelRenderStrategy
 * @brief   Renders labels with Qt
 *
 *
 * This class uses Qt to render labels and compute sizes. The labels are
 * rendered to a QImage, then EndFrame() converts that image to a vtkImageData
 * and textures the image onto a quad spanning the render area.
 */

#ifndef vtkQtLabelRenderStrategy_h
#define vtkQtLabelRenderStrategy_h

#include "vtkLabelRenderStrategy.h"
#include "vtkRenderingQtModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkLabelSizeCalculator;
class vtkLabeledDataMapper;
class vtkPlaneSource;
class vtkPolyDataMapper2D;
class vtkQImageToImageSource;
class vtkTexture;
class vtkTexturedActor2D;
class vtkTextureMapToPlane;

class VTKRENDERINGQT_EXPORT vtkQtLabelRenderStrategy : public vtkLabelRenderStrategy
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkQtLabelRenderStrategy, vtkLabelRenderStrategy);
  static vtkQtLabelRenderStrategy* New();

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   */
  void ComputeLabelBounds(vtkTextProperty* tprop, vtkStdString label, double bds[4]) override;

  ///@{
  /**
   * Render a label at a location in world coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   */
  void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label) override;
  void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label, int maxWidth) override;
  ///@}

  /**
   * Start a rendering frame. Renderer must be set.
   */
  void StartFrame() override;

  /**
   * End a rendering frame.
   */
  void EndFrame() override;

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow* window) override;

protected:
  vtkQtLabelRenderStrategy();
  ~vtkQtLabelRenderStrategy() override;

  class Internals;
  Internals* Implementation;

  vtkQImageToImageSource* QImageToImage;
  vtkPlaneSource* PlaneSource;
  vtkTextureMapToPlane* TextureMapToPlane;
  vtkTexture* Texture;
  vtkPolyDataMapper2D* Mapper;
  vtkTexturedActor2D* Actor;
  bool AntialiasText; // Should the text be antialiased, inherited from render window.

private:
  vtkQtLabelRenderStrategy(const vtkQtLabelRenderStrategy&) = delete;
  void operator=(const vtkQtLabelRenderStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
