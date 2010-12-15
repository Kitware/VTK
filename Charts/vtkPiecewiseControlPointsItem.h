/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkPiecewiseControlPointsItem - Control points for
// vtkPiecewiseFunction.
// .SECTION Description
// vtkPiecewiseControlPointsItem draws the control points of a vtkPiecewiseFunction.
// .SECTION See Also
// vtkControlPointsItem
// vtkPiecewiseFunctionItem
// vtkCompositeTransferFunctionItem

#ifndef __vtkPiecewiseControlPointsItem_h
#define __vtkPiecewiseControlPointsItem_h

#include "vtkControlPointsItem.h"

class vtkPiecewiseFunction;

class VTK_CHARTS_EXPORT vtkPiecewiseControlPointsItem: public vtkControlPointsItem
{
public:
  vtkTypeMacro(vtkPiecewiseControlPointsItem, vtkControlPointsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a piecewise control points object
  static vtkPiecewiseControlPointsItem* New();

  // Description:
  // Set the piecewise function to draw its points
  void SetPiecewiseFunction(vtkPiecewiseFunction* function);
  // Description
  // Get the piecewise function
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

protected:
  vtkPiecewiseControlPointsItem();
  virtual ~vtkPiecewiseControlPointsItem();

  // Decription:
  // Reimplemented to extract the control points from the piecewise function
  virtual void ComputePoints();

  // Description:
  // Returns true if the supplied x, y coordinate is inside the item.
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse move event.
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button down event.
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  // Description:
  // Mouse button release event.
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

  vtkPiecewiseFunction* PiecewiseFunction;

  float     ButtonPressPosition[2];
  vtkIdType MouseOver;

private:
  vtkPiecewiseControlPointsItem(const vtkPiecewiseControlPointsItem &); // Not implemented.
  void operator=(const vtkPiecewiseControlPointsItem &);   // Not implemented.
};

#endif
