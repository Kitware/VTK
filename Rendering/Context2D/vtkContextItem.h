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

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkAbstractContextItem.h"

class VTKRENDERINGCONTEXT2D_EXPORT vtkContextItem : public vtkAbstractContextItem
{
public:
  vtkTypeMacro(vtkContextItem, vtkAbstractContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get the opacity of the item.
  vtkGetMacro(Opacity, double);

  // Description:
  // Set the opacity of the item.
  // 1.0 by default.
  vtkSetMacro(Opacity, double);

//BTX
protected:
  vtkContextItem();
  ~vtkContextItem();

  double Opacity;

private:
  vtkContextItem(const vtkContextItem &); // Not implemented.
  void operator=(const vtkContextItem &);   // Not implemented.
//ETX
};

#endif //__vtkContextItem_h
