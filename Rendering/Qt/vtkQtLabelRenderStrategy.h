/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQtLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingQtModule.h" // For export macro
#include "vtkLabelRenderStrategy.h"

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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  vtkTypeMacro(vtkQtLabelRenderStrategy, vtkLabelRenderStrategy);
  static vtkQtLabelRenderStrategy* New();

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   */
  void ComputeLabelBounds(vtkTextProperty* tprop, vtkStdString label,
                                  double bds[4]) VTK_OVERRIDE
    { this->Superclass::ComputeLabelBounds(tprop, label, bds); }
  void ComputeLabelBounds(vtkTextProperty* tprop, vtkUnicodeString label,
                                  double bds[4]) VTK_OVERRIDE;

  //@{
  /**
   * Render a label at a location in world coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   */
  void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label) VTK_OVERRIDE
    { this->Superclass::RenderLabel(x, tprop, label); }
  void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label,
                           int maxWidth) VTK_OVERRIDE
    { this->Superclass::RenderLabel(x, tprop, label, maxWidth); }
  void RenderLabel(int x[2], vtkTextProperty* tprop,
                           vtkUnicodeString label) VTK_OVERRIDE;
  void RenderLabel(int x[2], vtkTextProperty* tprop,
                           vtkUnicodeString label, int maxWidth) VTK_OVERRIDE;
  //@}

  /**
   * Start a rendering frame. Renderer must be set.
   */
  void StartFrame() VTK_OVERRIDE;

  /**
   * End a rendering frame.
   */
  void EndFrame() VTK_OVERRIDE;

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *window) VTK_OVERRIDE;

protected:
  vtkQtLabelRenderStrategy();
  ~vtkQtLabelRenderStrategy() VTK_OVERRIDE;

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
  vtkQtLabelRenderStrategy(const vtkQtLabelRenderStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkQtLabelRenderStrategy&) VTK_DELETE_FUNCTION;
};

#endif
