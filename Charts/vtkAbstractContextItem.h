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
