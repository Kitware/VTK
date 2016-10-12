/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkColorTransferControlPointsItem
 * @brief   Control points for
 * vtkColorTransferFunction.
 *
 * vtkColorTransferControlPointsItem draws the control points of a vtkColorTransferFunction.
 * @sa
 * vtkControlPointsItem
 * vtkColorTransferFunctionItem
 * vtkCompositeTransferFunctionItem
*/

#ifndef vtkColorTransferControlPointsItem_h
#define vtkColorTransferControlPointsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkControlPointsItem.h"

class vtkColorTransferFunction;

class VTKCHARTSCORE_EXPORT vtkColorTransferControlPointsItem: public vtkControlPointsItem
{
public:
  vtkTypeMacro(vtkColorTransferControlPointsItem, vtkControlPointsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  /**
   * Creates a piecewise control points object
   */
  static vtkColorTransferControlPointsItem* New();

  /**
   * Set the piecewise function to draw its points
   */
  void SetColorTransferFunction(vtkColorTransferFunction* function);
  //@{
  /**
   * Get the piecewise function
   */
  vtkGetObjectMacro(ColorTransferFunction, vtkColorTransferFunction);
  //@}

  /**
   * Return the number of points in the color transfer function.
   */
  virtual vtkIdType GetNumberOfPoints()const;

  /**
   * Returns the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * Note: The y (point[1]) is always 0.5
   */
  virtual void GetControlPoint(vtkIdType index, double *point)const;

  /**
   * Sets the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * Changing the y has no effect, it will always be 0.5
   */
  virtual void SetControlPoint(vtkIdType index, double *point);

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  virtual vtkIdType AddPoint(double* newPos);

  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  virtual vtkIdType RemovePoint(double* pos);

  //@{
  /**
   * If ColorFill is true, the control point brush color is set with the
   * matching color in the color transfer function.
   * False by default.
   */
  vtkSetMacro(ColorFill, bool);
  vtkGetMacro(ColorFill, bool);
  //@}

protected:
  vtkColorTransferControlPointsItem();
  virtual ~vtkColorTransferControlPointsItem();

  /**
   * Returns true if control points are to be rendered in log-space. This is
   * true when vtkScalarsToColors is using log-scale, for example. Default
   * implementation always return false.
   */
  virtual bool UsingLogScale();

  virtual void emitEvent(unsigned long event, void* params);

  virtual vtkMTimeType GetControlPointsMTime();

  virtual void DrawPoint(vtkContext2D* painter, vtkIdType index);
  virtual void EditPoint(float tX, float tY);

  /**
   * Compute the bounds for this item. Overridden to use the
   * vtkColorTransferFunction range.
   */
  virtual void ComputeBounds(double* bounds);

  vtkColorTransferFunction* ColorTransferFunction;

  bool ColorFill;
private:
  vtkColorTransferControlPointsItem(const vtkColorTransferControlPointsItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkColorTransferControlPointsItem &) VTK_DELETE_FUNCTION;
};

#endif
