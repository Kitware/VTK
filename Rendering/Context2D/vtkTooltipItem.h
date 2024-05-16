// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkTooltipItem
 * @brief   takes care of drawing 2D axes
 *
 *
 * The vtkTooltipItem is drawn in screen coordinates. It is used to display a
 * tooltip on a scene, giving additional information about an element on the
 * scene, such as in vtkChartXY. It takes care of ensuring that it draws itself
 * within the bounds of the screen.
 */

#ifndef vtkTooltipItem_h
#define vtkTooltipItem_h

#include "vtkContextItem.h"
#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkStdString.h"                // For vtkStdString ivars
#include "vtkVector.h"                   // Needed for vtkVector2f
#include "vtkWrappingHints.h"            // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPen;
class vtkBrush;
class vtkTextProperty;

class VTKRENDERINGCONTEXT2D_EXPORT VTK_MARSHALAUTO vtkTooltipItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkTooltipItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a 2D Chart object.
   */
  static vtkTooltipItem* New();

  ///@{
  /**
   * Set the position of the tooltip (in pixels).
   */
  vtkSetVector2Macro(Position, float);
  void SetPosition(const vtkVector2f& pos);
  ///@}

  ///@{
  /**
   * Get position of the axis (in pixels).
   */
  vtkGetVector2Macro(Position, float);
  vtkVector2f GetPositionVector();
  ///@}

  ///@{
  /**
   * Get/set the text of the item.
   */
  virtual void SetText(const vtkStdString& text);
  virtual vtkStdString GetText();
  ///@}

  ///@{
  /**
   * Get a pointer to the vtkTextProperty object that controls the way the
   * text is rendered.
   */
  vtkGetObjectMacro(Pen, vtkPen);
  ///@}

  ///@{
  /**
   * Get a pointer to the vtkPen object.
   */
  vtkGetObjectMacro(Brush, vtkBrush);
  ///@}

  ///@{
  /**
   * Get the vtkTextProperty that governs how the tooltip text is displayed.
   */
  vtkGetObjectMacro(TextProperties, vtkTextProperty);
  ///@}

  /**
   * Update the geometry of the tooltip.
   */
  void Update() override;

  /**
   * Paint event for the tooltip.
   */
  bool Paint(vtkContext2D* painter) override;

protected:
  vtkTooltipItem();
  ~vtkTooltipItem() override;

  vtkVector2f PositionVector;
  float* Position;
  vtkStdString Text;
  vtkTextProperty* TextProperties;
  vtkPen* Pen;
  vtkBrush* Brush;

private:
  vtkTooltipItem(const vtkTooltipItem&) = delete;
  void operator=(const vtkTooltipItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkTooltipItem_h
