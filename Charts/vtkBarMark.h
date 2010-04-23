/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBarMark.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkBarMark - base class for items that are part of a vtkContextScene.
//
// .SECTION Description
// Derive from this class to create custom items that can be added to a
// vtkContextScene.

#ifndef __vtkBarMark_h
#define __vtkBarMark_h

#include "vtkMark.h"

class vtkBrush;
class vtkDataObject;
class vtkPen;
class vtkAbstractContextBufferId;

class VTK_CHARTS_EXPORT vtkBarMark : public vtkMark
{
public:
  vtkTypeMacro(vtkBarMark, vtkMark);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkBarMark* New();

  // Description:
  // Paint event for the item, called whenever the item needs to be drawn,
  virtual bool Paint(vtkContext2D *painter);

//BTX
  // Description:
  // Paint the mark elements in a special mode to build a cache for picking.
  // Use internally.
  void PaintIds();
  
  // Description:
  // Make sure the buffer id for the children items is up-to-date.
  void UpdateBufferId();
  
  // Description:
  // Return the item under mouse cursor at x,y.
  vtkIdType GetPickedItem(int x, int y);
  
  // Description:
  // Mouse enter event. As Panel is container, it propagates the event to
  // its children.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event. As Panel is container, it propagates the event to
  // its children.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event. As Panel is container, it propagates the event to
  // its children.
  // Return true if the item holds the event, false if the event can be
  // propagated to other items.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);
  
  // Description:
  // Mouse enter event on a specific item of the bar.
  void MouseEnterEventOnItem(int item);
  
  // Description:
  // Mouse leave event on a specific item sector of the bar.
  void MouseLeaveEventOnItem(int item);
  
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);
//ETX

  virtual int GetType() { return BAR; }
  
  // Description:
  // Release graphics resources hold by the item.
  virtual void ReleaseGraphicsResources();
  
//BTX
protected:
  vtkBarMark();
  ~vtkBarMark();
  
  bool MouseOver; // tell if the mouse cursor entered the bar
  
  vtkAbstractContextBufferId *BufferId;
  
  int ActiveItem;
//  bool PaintIdMode;
  
private:
  vtkBarMark(const vtkBarMark &); // Not implemented.
  void operator=(const vtkBarMark &);   // Not implemented.
//ETX
};

#endif //__vtkBarMark_h
