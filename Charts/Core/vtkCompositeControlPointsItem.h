// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkPiecewiseFunction;
class vtkPiecewisePointHandleItem;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkCompositeControlPointsItem
  : public vtkColorTransferControlPointsItem
{
public:
  vtkTypeMacro(vtkCompositeControlPointsItem, vtkColorTransferControlPointsItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a piecewise control points object
   */
  static vtkCompositeControlPointsItem* New();

  /**
   * Set the color transfer function to draw its points
   */
  virtual void SetColorTransferFunction(vtkColorTransferFunction* function);

  ///@{
  /**
   * Utility function that calls SetPiecewiseFunction()
   */
  void SetOpacityFunction(vtkPiecewiseFunction* opacity);
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);
  ///@}

  enum PointsFunctionType
  {
    ColorPointsFunction = 1,
    OpacityPointsFunction = 2,
    ColorAndOpacityPointsFunction = 3
  };

  ///@{
  /**
   * PointsFunction controls whether the points represent the
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
  ///@}

  /**
   * Add a point to the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType AddPoint(double* newPos) override;

  using Superclass::RemovePoint;
  /**
   * Remove a point of the function. Returns the index of the point (0 based),
   * or -1 on error.
   * Subclasses should reimplement this function to do the actual work.
   */
  vtkIdType RemovePoint(double* pos) override;

  ///@{
  /**
   * If UseOpacityPointHandles is true, when the current point is
   * double clicked, a vtkPiecewisePointHandleItem will show up so
   * that the sharpness and mid point can be adjusted in the scene
   * with those handles
   * False by default.
   */
  vtkSetMacro(UseOpacityPointHandles, bool);
  vtkGetMacro(UseOpacityPointHandles, bool);
  ///@}

  ///@{
  /**
   * Mouse move event. To take care of some special Key stroke
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseDoubleClickEvent(const vtkContextMouseEvent& mouse) override;
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;
  ///@}

  /**
   * Returns the total number of points, either from
   * using the superclass implementation or the opacity function
   * when available
   */
  vtkIdType GetNumberOfPoints() const override;

  /**
   * Returns the x and y coordinates as well as the midpoint and sharpness
   * of the control point corresponding to the index.
   * point must be a double array of size 4.
   * The values will be recovered from the opacity function when available.
   */
  void GetControlPoint(vtkIdType index, double point[4]) const override;

  /**
   * Sets the x and y coordinates as well as the midpoint and sharpness,
   * of the control point corresponding to the index, either using the superclass
   * implementation or the opacity function when available.
   * The provided point should be a double array of size 4.
   */
  void SetControlPoint(vtkIdType index, double* point) override;

protected:
  vtkCompositeControlPointsItem();
  ~vtkCompositeControlPointsItem() override;

  void emitEvent(unsigned long event, void* params) override;

  vtkMTimeType GetControlPointsMTime() override;

  void DrawPoint(vtkContext2D* painter, vtkIdType index) override;
  void EditPoint(float tX, float tY) override;
  virtual void EditPointCurve(vtkIdType idx);

  void MergeTransferFunctions();
  void SilentMergeTransferFunctions();

  int PointsFunction = vtkCompositeControlPointsItem::ColorAndOpacityPointsFunction;
  vtkPiecewiseFunction* OpacityFunction = nullptr;
  vtkPiecewisePointHandleItem* OpacityPointHandle = nullptr;
  bool UseOpacityPointHandles = false;

private:
  vtkCompositeControlPointsItem(const vtkCompositeControlPointsItem&) = delete;
  void operator=(const vtkCompositeControlPointsItem&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
