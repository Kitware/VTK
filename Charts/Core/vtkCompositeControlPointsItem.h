/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkCompositeControlPointsItem
 * @brief   Control points for
 * vtkCompositeFunction.
 *
 * vtkCompositeControlPointsItem draws the control points of a vtkPiecewiseFunction
 * and a vtkColorTransferFunction.
 * @sa
 * vtkControlPointsItem
 * vtkColorTransferControlPointsItem
 * vtkCompositeTransferFunctionItem
 * vtkPiecewisePointHandleItem
*/

#ifndef vtkCompositeControlPointsItem_h
#define vtkCompositeControlPointsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkColorTransferControlPointsItem.h"

class vtkPiecewiseFunction;
class vtkPiecewisePointHandleItem;

class VTKCHARTSCORE_EXPORT vtkCompositeControlPointsItem:
  public vtkColorTransferControlPointsItem
{
public:
  vtkTypeMacro(vtkCompositeControlPointsItem, vtkColorTransferControlPointsItem);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Creates a piecewise control points object
   */
  static vtkCompositeControlPointsItem* New();

  /**
   * Set the color transfer function to draw its points
   */
  virtual void SetColorTransferFunction(vtkColorTransferFunction* function);

  //@{
  /**
   * Utility function that calls SetPiecewiseFunction()
   */
  void SetOpacityFunction(vtkPiecewiseFunction* opacity);
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);
  //@}

  enum PointsFunctionType{
    ColorPointsFunction = 1,
    OpacityPointsFunction = 2,
    ColorAndOpacityPointsFunction = 3
  };
  //@{
  /**
   * PointsFunction controls wether the points represent the
   * ColorTransferFunction, OpacityTransferFunction or both.
   * If ColorPointsFunction, only the points of the ColorTransfer function are
   * used.
   * If OpacityPointsFunction, only the points of the Opacity function are used
   * If ColorAndOpacityPointsFunction, the points of both functions are shared
   * by both functions.
   * ColorAndOpacityPointsFunction by default.
   * Note: Set the mode before the functions are set. ColorPointsFunction is
   * not fully supported.
   */
  vtkSetMacro(PointsFunction, int);
  vtkGetMacro(PointsFunction, int);
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
   * If UseOpacityPointHandles is true, when the current point is
   * double clicked, a vtkPiecewisePointHandleItem will show up so
   * that the sharpness and mid point can be adjusted in the scene
   * with those handles
   * False by default.
   */
  vtkSetMacro(UseOpacityPointHandles, bool);
  vtkGetMacro(UseOpacityPointHandles, bool);
  //@}

  //@{
  /**
   * Mouse move event. To take care of some special Key stroke
   */
  bool MouseMoveEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;
  bool MouseDoubleClickEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;
  bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse) VTK_OVERRIDE;
  //@}

protected:
  vtkCompositeControlPointsItem();
  ~vtkCompositeControlPointsItem() VTK_OVERRIDE;

  /**
   * Returns true if control points are to be rendered in log-space. This is
   * true when vtkScalarsToColors is using log-scale, for example.
   */
  bool UsingLogScale() VTK_OVERRIDE;

  void emitEvent(unsigned long event, void* params) VTK_OVERRIDE;

  vtkMTimeType GetControlPointsMTime() VTK_OVERRIDE;

  vtkIdType GetNumberOfPoints()const VTK_OVERRIDE;
  void DrawPoint(vtkContext2D* painter, vtkIdType index) VTK_OVERRIDE;
  void GetControlPoint(vtkIdType index, double* pos)const VTK_OVERRIDE;
  void SetControlPoint(vtkIdType index, double *point) VTK_OVERRIDE;
  void EditPoint(float tX, float tY) VTK_OVERRIDE;
  virtual void EditPointCurve(vtkIdType idx);

  void MergeTransferFunctions();
  void SilentMergeTransferFunctions();

  int                   PointsFunction;
  vtkPiecewiseFunction* OpacityFunction;
  vtkPiecewisePointHandleItem* OpacityPointHandle;
  bool UseOpacityPointHandles;

private:
  vtkCompositeControlPointsItem(const vtkCompositeControlPointsItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeControlPointsItem &) VTK_DELETE_FUNCTION;
};

#endif
