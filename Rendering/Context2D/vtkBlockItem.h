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

/**
 * @class   vtkBlockItem
 * @brief   a vtkContextItem that draws a block (optional label).
 *
 *
 * This is a vtkContextItem that can be placed into a vtkContextScene. It draws
 * a block of the given dimensions, and reacts to mouse events.
*/

#ifndef vtkBlockItem_h
#define vtkBlockItem_h

#include "vtkRenderingContext2DModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkStdString.h"    // For vtkStdString ivars

class vtkContext2D;

class VTKRENDERINGCONTEXT2D_EXPORT vtkBlockItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkBlockItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkBlockItem *New();

  /**
   * Paint event for the item.
   */
  virtual bool Paint(vtkContext2D *painter);

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Mouse enter event.
   */
  virtual bool MouseEnterEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse move event.
   */
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse leave event.
   */
  virtual bool MouseLeaveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button down event.
   */
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button release event.
   */
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  /**
   * Set the block label.
   */
  virtual void SetLabel(const vtkStdString &label);

  /**
   * Get the block label.
   */
  virtual vtkStdString GetLabel();

  //@{
  /**
   * Set the dimensions of the block, elements 0 and 1 are the x and y
   * coordinate of the bottom corner. Elements 2 and 3 are the width and
   * height.
   * Initial value is (0,0,0,0).
   */
  vtkSetVector4Macro(Dimensions, float);
  //@}

  //@{
  /**
   * Get the dimensions of the block, elements 0 and 1 are the x and y
   * coordinate of the bottom corner. Elements 2 and 3 are the width and
   * height.
   * Initial value is (0,0,0,0)
   */
  vtkGetVector4Macro(Dimensions, float);
  //@}

  void SetScalarFunctor(double (*scalarFunction)(double, double));

protected:
  vtkBlockItem();
  ~vtkBlockItem();

  float Dimensions[4];

  vtkStdString Label;

  bool MouseOver;

  // Some function pointers to optionally do funky things...
  double (*scalarFunction)(double, double);

private:
  vtkBlockItem(const vtkBlockItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkBlockItem &) VTK_DELETE_FUNCTION;

};

#endif //vtkBlockItem_h
