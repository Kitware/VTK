/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFreeTypeLabelRenderStrategy.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingLabelModule.h" // For export macro
#include "vtkLabelRenderStrategy.h"

class vtkActor2D;
class vtkTextRenderer;
class vtkTextMapper;

class VTKRENDERINGLABEL_EXPORT vtkFreeTypeLabelRenderStrategy : public vtkLabelRenderStrategy
{
 public:
  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkFreeTypeLabelRenderStrategy, vtkLabelRenderStrategy);
  static vtkFreeTypeLabelRenderStrategy* New();

  /**
   * The free type render strategy currently does not support rotation.
   */
  virtual bool SupportsRotation()
    { return false; }

  /**
   * The free type render strategy currently does not support bounded size labels.
   */
  virtual bool SupportsBoundedSize()
    { return false; }

  /**
   * Compute the bounds of a label. Must be performed after the renderer is set.
   */
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkStdString label, double bds[4])
    { this->Superclass::ComputeLabelBounds(tprop, label, bds); }
  virtual void ComputeLabelBounds(vtkTextProperty* tprop, vtkUnicodeString label, double bds[4]);

  /**
   * Render a label at a location in world coordinates.
   * Must be performed between StartFrame() and EndFrame() calls.
   */
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label)
    { this->Superclass::RenderLabel(x, tprop, label); }
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop, vtkStdString label, int width)
    { this->Superclass::RenderLabel(x, tprop, label, width); }
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop, vtkUnicodeString label);
  virtual void RenderLabel(int x[2], vtkTextProperty* tprop, vtkUnicodeString label, int width)
    { this->Superclass::RenderLabel(x, tprop, label, width); }

  /**
   * Release any graphics resources that are being consumed by this strategy.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(vtkWindow *window);

protected:
  vtkFreeTypeLabelRenderStrategy();
  ~vtkFreeTypeLabelRenderStrategy();

  vtkTextRenderer *TextRenderer;
  vtkTextMapper* Mapper;
  vtkActor2D* Actor;

private:
  vtkFreeTypeLabelRenderStrategy(const vtkFreeTypeLabelRenderStrategy&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFreeTypeLabelRenderStrategy&) VTK_DELETE_FUNCTION;
};

#endif

