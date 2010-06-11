/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkAbstractContextItem - base class for items that are part of a
// vtkContextScene.
//
// .SECTION Description
// This class is the common base for all context scene items. You should
// generally derive from vtkContextItem, rather than this class, as it provides
// most of the commonly used API.

#ifndef __vtkAbstractContextItem_h
#define __vtkAbstractContextItem_h

#include "vtkObject.h"

class vtkContext2D;
class vtkContextMouseEvent;

class VTK_CHARTS_EXPORT vtkAbstractContextItem : public vtkObject
{
public:
  vtkTypeMacro(vtkAbstractContextItem, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the item, called whenever the item needs to be drawn.
  virtual bool Paint(vtkContext2D *painter) = 0;

  // Description:
  // Release graphics resources hold by the item. The default implementation
  // is empty.
  virtual void ReleaseGraphicsResources();

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse enter event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse wheel event, positive delta indicates forward movement of the wheel.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseWheelEvent(const vtkContextMouseEvent &mouse, int delta);
//ETX

//BTX
protected:
  vtkAbstractContextItem();
  ~vtkAbstractContextItem();

private:
  vtkAbstractContextItem(const vtkAbstractContextItem &); // Not implemented.
  void operator=(const vtkAbstractContextItem &);   // Not implemented.
//ETX
};

#endif //__vtkContextItem_h
