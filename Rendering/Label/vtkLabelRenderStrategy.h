/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLabelRenderStrategy
 * @brief   Superclass for label rendering implementations.
 *
 *
 * These methods should only be called within a mapper.
*/

#ifndef vtkLabelRenderStrategy_h
#define vtkLabelRenderStrategy_h

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkObject.h"

#include "vtkStdString.h" // For string support
#include "vtkUnicodeString.h" // For unicode string support

class vtkRenderer;
class vtkWindow;
class vtkTextProperty;

class VTKRENDERINGLABEL_EXPORT vtkLabelRenderStrategy : public vtkObject
{
 public:
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  vtkTypeMacro(vtkLabelRenderStrategy, vtkObject);

  /**
   * Whether the text rendering strategy supports rotation.
   * The superclass returns true. Subclasses should override this to
   * return the appropriate value.
   */
  virtual bool SupportsRotation()
    { return true; }

  /**
   * Whether the text rendering strategy supports bounded size.
   * The superclass returns true. Subclasses should override this to
   * return the appropriate value. Subclasses that return true
   * from this method should implement the version of RenderLabel()
   * that takes a maximum size (see RenderLabel()).
   */
  virtual bool SupportsBoundedSize()
    { return true; }

  //@{
  /**
   * Set the renderer associated with this strategy.
   */
  virtual void SetRenderer(vtkRenderer* ren);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  //@}

  //@{
  /**
   * Set the default text property for the strategy.
   */
  virtual void SetDefaultTextProperty(vtkTextProperty* tprop);
  vtkGetObjectMacro(DefaultTextProperty, vtkTextProperty);
  //@}

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   * Only the unicode string version must be implemented in subclasses.
   */
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkStdString label,
                                  double bds[4])
    { this->ComputeLabelBounds(tprop, vtkUnicodeString::from_utf8(label.c_str()),
                               bds); }
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkUnicodeString label,
                                  double bds[4]) = 0;

  /**
   * Render a label at a location in display coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   * Only the unicode string version must be implemented in subclasses.
   * The optional final parameter maxWidth specifies a maximum width for the label.
   * Longer labels can be shorted with an ellipsis (...). Only renderer strategies
   * that return true from SupportsBoundedSize must implement this version of th
   * method.
   */
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label)
    { this->RenderLabel(x, tprop, vtkUnicodeString::from_utf8(label)); }
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label,
                           int maxWidth)
    { this->RenderLabel(x, tprop, vtkUnicodeString::from_utf8(label), maxWidth); }
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop,
                           vtkUnicodeString label) = 0;
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop,
                           vtkUnicodeString label, int vtkNotUsed(maxWidth))
    { this->RenderLabel(x, tprop, label); }

  /**
   * Start a rendering frame. Renderer must be set.
   */
  virtual void StartFrame() { }

  /**
   * End a rendering frame.
   */
  virtual void EndFrame() { }

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *) { }

protected:
  vtkLabelRenderStrategy();
  ~vtkLabelRenderStrategy() VTK_OVERRIDE;

  vtkRenderer* Renderer;
  vtkTextProperty* DefaultTextProperty;

private:
  vtkLabelRenderStrategy(const vtkLabelRenderStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLabelRenderStrategy&) VTK_DELETE_FUNCTION;
};

#endif

