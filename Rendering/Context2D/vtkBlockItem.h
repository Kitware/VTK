// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkBlockItem
 * @brief   a vtkContextItem that draws a block (optional label).
 *
 * This is a vtkContextItem that can be placed into a vtkContextScene. It draws
 * a block of the given dimensions, and reacts to mouse events.
 *
 * vtkBlockItem can also be used to render label in the scene. The label
 * properties can be set using `vtkTextProperty` accessed via
 * `GetLabelProperties`.
 *
 */

#ifndef vtkBlockItem_h
#define vtkBlockItem_h

#include "vtkContextItem.h"
#include "vtkNew.h"                      // For vtkNew
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkStdString.h"                // For vtkStdString ivars
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkTextProperty;
class vtkBrush;
class vtkPen;

class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkBlockItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkBlockItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkBlockItem* New();

  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse enter event.
   */
  bool MouseEnterEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse leave event.
   */
  bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Set the block label.
   */
  virtual void SetLabel(const vtkStdString& label);

  /**
   * Get the block label.
   */
  virtual vtkStdString GetLabel();

  ///@{
  /**
   * Set the dimensions of the block, elements 0 and 1 are the x and y
   * coordinate of the bottom corner. Elements 2 and 3 are the width and
   * height.
   * Initial value is (0,0,0,0).
   */
  vtkSetVector4Macro(Dimensions, float);
  ///@}

  ///@{
  /**
   * Get the dimensions of the block, elements 0 and 1 are the x and y
   * coordinate of the bottom corner. Elements 2 and 3 are the width and
   * height.
   * Initial value is (0,0,0,0)
   */
  vtkGetVector4Macro(Dimensions, float);
  ///@}

  ///@{
  /**
   * When set to true, the dimensions for the block are computed automatically
   * using the anchor point, alignment at the size of the label.
   * Otherwise the `Dimensions` are used.
   *
   * Default is false i.e `Dimensions` will be used.
   */
  vtkSetMacro(AutoComputeDimensions, bool);
  vtkGetMacro(AutoComputeDimensions, bool);
  vtkBooleanMacro(AutoComputeDimensions, bool);
  ///@}

  enum
  {
    LEFT = 0,
    CENTER,
    RIGHT,
    TOP,
    BOTTOM,
    CUSTOM
  };

  ///@{
  /**
   * Set/Get the horizontal alignment of the legend to the point specified.
   * Valid values are LEFT, CENTER and RIGHT.
   */
  vtkSetMacro(HorizontalAlignment, int);
  vtkGetMacro(HorizontalAlignment, int);
  ///@}

  ///@{
  /**
   * Set/Get the vertical alignment of the legend to the point specified.
   * Valid values are TOP, CENTER and BOTTOM.
   */
  vtkSetMacro(VerticalAlignment, int);
  vtkGetMacro(VerticalAlignment, int);
  ///@}

  ///@{
  /**
   * When AutoComputeDimensions is true, these are the padding for the label
   * within the block.
   *
   * Default is (5, 5).
   */
  vtkSetVector2Macro(Padding, int);
  vtkGetVector2Macro(Padding, int);
  ///@}

  ///@{
  /**
   * When AutoComputeDimensions is true, these are the margins from the edge of
   * the viewport to use when placing the block based on HorizontalAlignment and
   * VerticalAlignment preferences.
   */
  vtkSetVector2Macro(Margins, int);
  vtkGetVector2Macro(Margins, int);
  ///@}

  ///@{
  /**
   * Get pen used to draw the block item outline.
   */
  vtkGetObjectMacro(Pen, vtkPen);
  ///@}

  ///@{
  /**
   * Get the brush used to draw the block item background.
   */
  vtkGetObjectMacro(Brush, vtkBrush);
  ///@}

  ///@{
  /**
   * Get the brush used to draw the block item background when the
   * item is "hit" i.e. interaction is enabled and the mouse is over the block.
   */
  vtkGetObjectMacro(MouseOverBrush, vtkBrush);
  ///@}

  ///@{
  /**
   * Provides access to the vtkTextProperty object that controls the way the
   * label is rendered.
   */
  void SetLabelProperties(vtkTextProperty*);
  vtkGetObjectMacro(LabelProperties, vtkTextProperty);
  ///@}

  void SetScalarFunctor(double (*scalarFunction)(double, double));

protected:
  vtkBlockItem();
  ~vtkBlockItem() override;

  float Dimensions[4];

  vtkStdString Label;

  bool MouseOver;

  // Some function pointers to optionally do funky things...
  double (*scalarFunction)(double, double);

private:
  vtkBlockItem(const vtkBlockItem&) = delete;
  void operator=(const vtkBlockItem&) = delete;

  vtkTextProperty* LabelProperties;
  vtkNew<vtkTextProperty> CachedTextProp;

  vtkNew<vtkPen> Pen;
  vtkNew<vtkPen> CachedPen;

  vtkNew<vtkBrush> Brush;
  vtkNew<vtkBrush> MouseOverBrush;
  vtkNew<vtkBrush> CachedBrush;

  int HorizontalAlignment;
  int VerticalAlignment;
  bool AutoComputeDimensions;
  int Padding[2];
  int Margins[2];
};

VTK_ABI_NAMESPACE_END
#endif // vtkBlockItem_h
