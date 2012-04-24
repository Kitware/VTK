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

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkControlPointsItem.h"

class vtkPiecewiseFunction;

class VTKCHARTSCORE_EXPORT vtkPiecewiseControlPointsItem: public vtkControlPointsItem
{
public:
  vtkTypeMacro(vtkPiecewiseControlPointsItem, vtkControlPointsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a piecewise control points object
  static vtkPiecewiseControlPointsItem* New();

  // Description:
  // Set the piecewise function to draw its points
  virtual void SetPiecewiseFunction(vtkPiecewiseFunction* function);
  // Description
  // Get the piecewise function
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

  // Description:
  // Add a point to the function. Returns the index of the point (0 based),
  // or -1 on error.
  // Subclasses should reimplement this function to do the actual work.
  virtual vtkIdType AddPoint(double* newPos);

  // Description:
  // Remove a point of the function. Returns the index of the point (0 based),
  // or -1 on error.
  // Subclasses should reimplement this function to do the actual work.
  virtual vtkIdType RemovePoint(double* pos);

  // Description:
  // Controls whether or not control points are drawn (true) or clicked and
  // moved (false).
  // False by default.
  vtkSetMacro(StrokeMode, bool);

protected:
  vtkPiecewiseControlPointsItem();
  virtual ~vtkPiecewiseControlPointsItem();

  virtual void emitEvent(unsigned long event, void* params = 0);

  virtual unsigned long int GetControlPointsMTime();

  virtual vtkIdType GetNumberOfPoints()const;
  virtual void GetControlPoint(vtkIdType index, double *point)const;
  virtual void SetControlPoint(vtkIdType index, double *point);
  virtual void EditPoint(float tX, float tY);

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseControlPointsItem(const vtkPiecewiseControlPointsItem &); // Not implemented.
  void operator=(const vtkPiecewiseControlPointsItem &);   // Not implemented.
};

#endif
