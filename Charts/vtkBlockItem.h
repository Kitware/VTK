/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBlockItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkBlockItem - a vtkContextItem that draws a block (optional label).
//
// .SECTION Description
// This is a vtkContextItem that can be placed into a vtkContextScene. It draws
// a block of the given dimensions, and reacts to mouse events.

#ifndef __vtkBlockItem_h
#define __vtkBlockItem_h

#include "vtkContextItem.h"
#include "vtkStdString.h"    // For vtkStdString ivars

class vtkContext2D;

class VTK_CHARTS_EXPORT vtkBlockItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkBlockItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkBlockItem *New();

  // Description:
  // Paint event for the item.
  virtual bool Paint(vtkContext2D *painter);

//BTX
  // Description:
  // Returns true if the supplied x, y coordinate is inside the item.
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
  // Mouse button down event.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);
//ETX

  // Description:
  // Set the block label.
  virtual void SetLabel(const vtkStdString &label);

  // Description:
  // Get the block label.
  virtual vtkStdString GetLabel();

  // Description:
  // Set the dimensions of the block, elements 0 and 1 are the x and y
  // coordinate of the bottom corner. Elements 2 and 3 are the width and
  // height.
  // Initial value is (0,0,0,0).
  vtkSetVector4Macro(Dimensions, int);

  // Description:
  // Get the dimensions of the block, elements 0 and 1 are the x and y
  // coordinate of the bottom corner. Elements 2 and 3 are the width and
  // height.
  // Initial value is (0,0,0,0)
  vtkGetVector4Macro(Dimensions, int);

//BTX
  void SetScalarFunctor(double (*scalarFunction)(double, double));
//ETX

//BTX
protected:
  vtkBlockItem();
  ~vtkBlockItem();

  int Dimensions[4];

  float LastPosition[2];

  vtkStdString Label;

  bool MouseOver;
  int MouseButtonPressed;

  // Some function pointers to optionally do funky things...
  double (*scalarFunction)(double, double);

private:
  vtkBlockItem(const vtkBlockItem &); // Not implemented.
  void operator=(const vtkBlockItem &);   // Not implemented.
//ETX
};

#endif //__vtkBlockItem_h
