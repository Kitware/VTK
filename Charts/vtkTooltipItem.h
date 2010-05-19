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

// .NAME vtkTooltipItem - takes care of drawing 2D axes
//
// .SECTION Description
// The vtkTooltipItem is drawn in screen coordinates. It is used to display a
// tooltip on a scene, giving additional information about an element on the
// scene, such as in vtkChartXY. It takes care of ensuring that it draws itself
// within the bounds of the screen.

#ifndef __vtkTooltipItem_h
#define __vtkTooltipItem_h

#include "vtkContextItem.h"
#include "vtkVector.h" // Needed for vtkVector2f

class vtkPen;
class vtkBrush;
class vtkTextProperty;

class VTK_CHARTS_EXPORT vtkTooltipItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkTooltipItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a 2D Chart object.
  static vtkTooltipItem *New();

  // Description:
  // Set the position of the tooltip (in pixels).
  vtkSetVector2Macro(Position, float);

  // Description:
  // Get position of the axis (in pixels).
  vtkGetVector2Macro(Position, float);

  // Description:
  // Get/set the text of the item.
  vtkSetStringMacro(Text);
  vtkGetStringMacro(Text);

  // Description:
  // Get a pointer to the vtkTextProperty object that controls the way the
  // text is rendered.
  vtkGetObjectMacro(Pen, vtkPen);

  // Description:
  // Get a pointer to the vtkPen object.
  vtkGetObjectMacro(Brush, vtkBrush);

  // Description:
  // Update the geometry of the tooltip.
  virtual void Update();

  // Description:
  // Paint event for the tooltip.
  virtual bool Paint(vtkContext2D *painter);

//BTX
protected:
  vtkTooltipItem();
  ~vtkTooltipItem();

  vtkVector2f PositionVector;
  float* Position;
  char* Text;
  vtkTextProperty* TextProperties;
  vtkPen* Pen;
  vtkBrush* Brush;

private:
  vtkTooltipItem(const vtkTooltipItem &); // Not implemented.
  void operator=(const vtkTooltipItem &);   // Not implemented.
//ETX
};

#endif //__vtkTooltipItem_h
