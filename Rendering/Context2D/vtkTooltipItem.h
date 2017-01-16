/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTooltipItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkVector.h"     // Needed for vtkVector2f
#include "vtkStdString.h"  // For vtkStdString ivars

class vtkPen;
class vtkBrush;
class vtkTextProperty;

class VTKRENDERINGCONTEXT2D_EXPORT vtkTooltipItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkTooltipItem, vtkContextItem);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a 2D Chart object.
   */
  static vtkTooltipItem *New();

  //@{
  /**
   * Set the position of the tooltip (in pixels).
   */
  vtkSetVector2Macro(Position, float);
  void SetPosition(const vtkVector2f &pos);
  //@}

  //@{
  /**
   * Get position of the axis (in pixels).
   */
  vtkGetVector2Macro(Position, float);
  vtkVector2f GetPositionVector();
  //@}

  //@{
  /**
   * Get/set the text of the item.
   */
  virtual void SetText(const vtkStdString &title);
  virtual vtkStdString GetText();
  //@}

  //@{
  /**
   * Get a pointer to the vtkTextProperty object that controls the way the
   * text is rendered.
   */
  vtkGetObjectMacro(Pen, vtkPen);
  //@}

  //@{
  /**
   * Get a pointer to the vtkPen object.
   */
  vtkGetObjectMacro(Brush, vtkBrush);
  //@}

  //@{
  /**
   * Get the vtkTextProperty that governs how the tooltip text is displayed.
   */
  vtkGetObjectMacro(TextProperties, vtkTextProperty);
  //@}

  /**
   * Update the geometry of the tooltip.
   */
  void Update() VTK_OVERRIDE;

  /**
   * Paint event for the tooltip.
   */
  bool Paint(vtkContext2D *painter) VTK_OVERRIDE;

protected:
  vtkTooltipItem();
  ~vtkTooltipItem() VTK_OVERRIDE;

  vtkVector2f PositionVector;
  float* Position;
  vtkStdString Text;
  vtkTextProperty* TextProperties;
  vtkPen* Pen;
  vtkBrush* Brush;

private:
  vtkTooltipItem(const vtkTooltipItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkTooltipItem &) VTK_DELETE_FUNCTION;

};

#endif //vtkTooltipItem_h
