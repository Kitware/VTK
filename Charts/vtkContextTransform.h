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

// .NAME vtkContextTransform - all children of this item are transformed
// by the vtkTransform2D of this item.
//
// .SECTION Description
// This class can be used to transform all child items of this class. The
// default transform is the identity.

#ifndef __vtkContextTransform_h
#define __vtkContextTransform_h

#include "vtkAbstractContextItem.h"
#include "vtkSmartPointer.h" // Needed for SP ivars.

class vtkTransform2D;

class VTK_CHARTS_EXPORT vtkContextTransform : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextTransform, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a vtkContextTransform object.
  static vtkContextTransform *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the item, called whenever the item needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Translate the item by the specified amounts dx and dy in the x and y
  // directions.
  virtual void Translate(float dx, float dy);

  // Description:
  // Scale the item by the specified amounts dx and dy in the x and y
  // directions.
  virtual void Scale(float dx, float dy);

  // Description:
  // Rotate the item by the specified angle.
  virtual void Rotate(float angle);

  // Description:
  // Access the vtkTransform2D that controls object transformation.
  virtual vtkTransform2D* GetTransform();

  // Description:
  // Transforms a point to the parent coordinate system.
  virtual vtkVector2f MapToParent(const vtkVector2f& point);

  // Description:
  // Transforms a point from the parent coordinate system.
  virtual vtkVector2f MapFromParent(const vtkVector2f& point);

//BTX
protected:
  vtkContextTransform();
  ~vtkContextTransform();

  vtkSmartPointer<vtkTransform2D> Transform;

private:
  vtkContextTransform(const vtkContextTransform &); // Not implemented.
  void operator=(const vtkContextTransform &);   // Not implemented.
//ETX
};

#endif //__vtkContextTransform_h
