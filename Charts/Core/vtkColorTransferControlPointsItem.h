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
  void PrintSelf(ostream &os, vtkIndent indent) override;

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
  vtkIdType GetNumberOfPoints()const override;

  /**
   * Returns the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * Note: The y (point[1]) is always 0.5
   */
  void GetControlPoint(vtkIdType index, double *point)const override;

  /**
   * Sets the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * Changing the y has no effect, it will always be 0.5
   */
  void SetControlPoint(vtkIdType index, double *point) override;

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType AddPoint(double* newPos) override;

  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType RemovePoint(double* pos) override;

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
  ~vtkColorTransferControlPointsItem() override;

  void emitEvent(unsigned long event, void* params) override;

  vtkMTimeType GetControlPointsMTime() override;

  void DrawPoint(vtkContext2D* painter, vtkIdType index) override;
  void EditPoint(float tX, float tY) override;

  /**
   * Compute the bounds for this item. Overridden to use the
   * vtkColorTransferFunction range.
   */
  void ComputeBounds(double* bounds) override;

  vtkColorTransferFunction* ColorTransferFunction;

  bool ColorFill;
private:
  vtkColorTransferControlPointsItem(const vtkColorTransferControlPointsItem &) = delete;
  void operator=(const vtkColorTransferControlPointsItem &) = delete;
};

#endif
