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

// .NAME vtkContextItem - base class for items that are part of a vtkContextScene.
//
// .SECTION Description
// Derive from this class to create custom items that can be added to a
// vtkContextScene.

#ifndef __vtkContextItem_h
#define __vtkContextItem_h

#include "vtkObject.h"

class vtkContext2D;
class vtkTransform2D;
struct vtkContextMouseEvent;

class VTK_CHARTS_EXPORT vtkContextItem : public vtkObject
{
public:
  vtkTypeRevisionMacro(vtkContextItem, vtkObject);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Paint event for the item, called whenever the item needs to be drawn,
  virtual bool Paint(vtkContext2D *painter) = 0;

//BTX
  // Description:
  // Return true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse enter event.
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse leave event.
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);
//ETX

  // Description:
  // Set the transform for the item.
  virtual void SetTransform(vtkTransform2D *transform);

  // Description:
  // Set the transform for the item.
  vtkGetObjectMacro(Transform, vtkTransform2D);

  // Description:
  // Get the opacity of the item.
  vtkGetMacro(Opacity, double);

  // Description:
  // Set the opacity of the item.
  vtkSetMacro(Opacity, double);

  // Description:
  // Translate the item by the given dx, dy.
  void Translate(float dx, float dy);

//BTX
protected:
  vtkContextItem();
  ~vtkContextItem();

  vtkTransform2D *Transform;

  double Opacity;

private:
  vtkContextItem(const vtkContextItem &); // Not implemented.
  void operator=(const vtkContextItem &);   // Not implemented.
//ETX
};

#endif //__vtkContextItem_h
