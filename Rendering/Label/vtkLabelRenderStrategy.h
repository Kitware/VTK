// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLabelRenderStrategy
 * @brief   Superclass for label rendering implementations.
 *
 *
 * These methods should only be called within a mapper.
 */

#ifndef vtkLabelRenderStrategy_h
#define vtkLabelRenderStrategy_h

#include "vtkObject.h"
#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkWrappingHints.h"        // For VTK_MARSHALAUTO

#include "vtkStdString.h" // For string support

VTK_ABI_NAMESPACE_BEGIN
class vtkRenderer;
class vtkWindow;
class vtkTextProperty;

class VTKRENDERINGLABEL_EXPORT VTK_MARSHALAUTO vtkLabelRenderStrategy : public vtkObject
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkLabelRenderStrategy, vtkObject);

  /**
   * Whether the text rendering strategy supports rotation.
   * The superclass returns true. Subclasses should override this to
   * return the appropriate value.
   */
  virtual bool SupportsRotation() { return true; }

  /**
   * Whether the text rendering strategy supports bounded size.
   * The superclass returns true. Subclasses should override this to
   * return the appropriate value. Subclasses that return true
   * from this method should implement the version of RenderLabel()
   * that takes a maximum size (see RenderLabel()).
   */
  virtual bool SupportsBoundedSize() { return true; }

  ///@{
  /**
   * Set the renderer associated with this strategy.
   */
  virtual void SetRenderer(vtkRenderer* ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  ///@}

  ///@{
  /**
   * Set the default text property for the strategy.
   */
  virtual void SetDefaultTextProperty(vtkTextProperty* tprop);
  vtkGetObjectMacro(DefaultTextProperty, vtkTextProperty);
  ///@}

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   */
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkStdString label, double bds[4]) = 0;

  /**
   * Render a label at a location in display coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   * The optional final parameter maxWidth specifies a maximum width for the label.
   * Longer labels can be shorted with an ellipsis (...). Only renderer strategies
   * that return true from SupportsBoundedSize must implement this version of th
   * method.
   */
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label) = 0;
  virtual void RenderLabel(
    int x[2], vtkTextProperty* tprop, vtkStdString label, int vtkNotUsed(maxWidth))
  {
    this->RenderLabel(x, tprop, label);
  }

  /**
   * Start a rendering frame. Renderer must be set.
   */
  virtual void StartFrame() {}

  /**
   * End a rendering frame.
   */
  virtual void EndFrame() {}

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow*) {}

protected:
  vtkLabelRenderStrategy();
  ~vtkLabelRenderStrategy() override;

  vtkRenderer* Renderer;
  vtkTextProperty* DefaultTextProperty;

private:
  vtkLabelRenderStrategy(const vtkLabelRenderStrategy&) = delete;
  void operator=(const vtkLabelRenderStrategy&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
