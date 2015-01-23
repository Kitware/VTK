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

// .NAME vtkContextClip - all children of this item are clipped
// by the specified area.
//
// .SECTION Description
// This class can be used to clip the rendering of an item inside a rectangular
// area.

#ifndef vtkContextClip_h
#define vtkContextClip_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkAbstractContextItem.h"
#include "vtkSmartPointer.h" // Needed for SP ivars.

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextClip : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextClip, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a vtkContextClip object.
  static vtkContextClip *New();

  // Description:
  // Perform any updates to the item that may be necessary before rendering.
  // The scene should take care of calling this on all items before their
  // Paint function is invoked.
  virtual void Update();

  // Description:
  // Paint event for the item, called whenever the item needs to be drawn.
  virtual bool Paint(vtkContext2D *painter);

  // Description:
  // Set the origin, width and height of the clipping rectangle. These are in
  // pixel coordinates.
  virtual void SetClip(float x, float y, float width, float height);

//BTX
protected:
  vtkContextClip();
  ~vtkContextClip();

  float Dims[4];

private:
  vtkContextClip(const vtkContextClip &); // Not implemented.
  void operator=(const vtkContextClip &);   // Not implemented.
//ETX
};

#endif //vtkContextClip_h
