/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkGraphItem
 * @brief   a vtkContextItem that draws a block (optional label).
 *
 *
 * This is a vtkContextItem that can be placed into a vtkContextScene. It draws
 * a block of the given dimensions, and reacts to mouse events.
 */

#ifndef vtkGraphItem_h
#define vtkGraphItem_h

#include "vtkContextItem.h"

class vtkContext2D;
class vtkGraph;

class vtkGraphItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkGraphItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkGraphItem* New();

  vtkGetObjectMacro(Graph, vtkGraph);
  virtual void SetGraph(vtkGraph* g);

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

  void UpdatePositions();

protected:
  vtkGraphItem();
  ~vtkGraphItem() override;

  float LastPosition[2];

  bool MouseOver;
  int MouseButtonPressed;

  vtkGraph* Graph;
  vtkIdType HitVertex;

  class Implementation;
  Implementation* Impl;

private:
  vtkGraphItem(const vtkGraphItem&) = delete;
  void operator=(const vtkGraphItem&) = delete;
};

#endif // vtkGraphItem_h
