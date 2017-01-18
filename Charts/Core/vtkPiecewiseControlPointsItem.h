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

/**
 * @class   vtkPiecewiseControlPointsItem
 * @brief   Control points for
 * vtkPiecewiseFunction.
 *
 * vtkPiecewiseControlPointsItem draws the control points of a vtkPiecewiseFunction.
 * @sa
 * vtkControlPointsItem
 * vtkPiecewiseFunctionItem
 * vtkCompositeTransferFunctionItem
*/

#ifndef vtkPiecewiseControlPointsItem_h
#define vtkPiecewiseControlPointsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkControlPointsItem.h"

class vtkPiecewiseFunction;

class VTKCHARTSCORE_EXPORT vtkPiecewiseControlPointsItem: public vtkControlPointsItem
{
public:
  vtkTypeMacro(vtkPiecewiseControlPointsItem, vtkControlPointsItem);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a piecewise control points object
   */
  static vtkPiecewiseControlPointsItem* New();

  /**
   * Set the piecewise function to draw its points
   */
  virtual void SetPiecewiseFunction(vtkPiecewiseFunction* function);
  //@{
  /**
   * Get the piecewise function
   */
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);
  //@}

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType AddPoint(double* newPos) VTK_OVERRIDE;

  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType RemovePoint(double* pos) VTK_OVERRIDE;

  //@{
  /**
   * Controls whether or not control points are drawn (true) or clicked and
   * moved (false).
   * False by default.
   */
  vtkSetMacro(StrokeMode, bool);
  //@}

protected:
  vtkPiecewiseControlPointsItem();
  ~vtkPiecewiseControlPointsItem() VTK_OVERRIDE;

  void emitEvent(unsigned long event, void* params = 0) VTK_OVERRIDE;

  vtkMTimeType GetControlPointsMTime() VTK_OVERRIDE;

  vtkIdType GetNumberOfPoints()const VTK_OVERRIDE;
  void GetControlPoint(vtkIdType index, double *point)const VTK_OVERRIDE;
  void SetControlPoint(vtkIdType index, double *point) VTK_OVERRIDE;
  void EditPoint(float tX, float tY) VTK_OVERRIDE;

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseControlPointsItem(const vtkPiecewiseControlPointsItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPiecewiseControlPointsItem &) VTK_DELETE_FUNCTION;
};

#endif
