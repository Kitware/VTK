// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkPiecewisePointHandleItem
 * @brief   a vtkContextItem that draws handles
 *       around a point of a piecewise function
 *
 *
 * This is a vtkContextItem that can be placed into a vtkContextScene. It draws
 * handles around a given point of a piecewise function so that the curve can
 * be adjusted using these handles.
 */

#ifndef vtkPiecewisePointHandleItem_h
#define vtkPiecewisePointHandleItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkContextItem.h"
#include "vtkWeakPointer.h"   // Needed for weak pointer to the PiecewiseFunction.
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkContext2D;
class vtkPiecewiseFunction;
class vtkCallbackCommand;
class vtkAbstractContextItem;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkPiecewisePointHandleItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkPiecewisePointHandleItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPiecewisePointHandleItem* New();
  static void CallRedraw(vtkObject* sender, unsigned long event, void* receiver, void* params);

  /**
   * Set the parent item, which should be a vtkControlPointItem
   */
  void SetParent(vtkAbstractContextItem* parent) override;

  /**
   * Paint event for the item.
   */
  bool Paint(vtkContext2D* painter) override;

  ///@{
  /**
   * The current point id in the piecewise function being handled.
   */
  vtkSetMacro(CurrentPointIndex, vtkIdType);
  vtkGetMacro(CurrentPointIndex, vtkIdType);
  ///@}

  ///@{
  /**
   * Set the PieceWiseFunction the handles will manipulate
   */
  virtual void SetPiecewiseFunction(vtkPiecewiseFunction* piecewiseFunc);
  vtkWeakPointer<vtkPiecewiseFunction> GetPiecewiseFunction();
  ///@}

  /**
   * Returns the index of the handle if pos is over any of the handles,
   * otherwise return -1;
   */
  int IsOverHandle(float* pos);

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

protected:
  vtkPiecewisePointHandleItem();
  ~vtkPiecewisePointHandleItem() override;

  /**
   * Redraw all the handles
   */
  virtual void Redraw();

  int MouseOverHandleIndex;
  vtkIdType CurrentPointIndex;
  float HandleRadius;

  vtkWeakPointer<vtkPiecewiseFunction> PiecewiseFunction;
  vtkCallbackCommand* Callback;

private:
  vtkPiecewisePointHandleItem(const vtkPiecewisePointHandleItem&) = delete;
  void operator=(const vtkPiecewisePointHandleItem&) = delete;

  class InternalPiecewisePointHandleInfo;
  InternalPiecewisePointHandleInfo* Internal;
};

VTK_ABI_NAMESPACE_END
#endif // vtkPiecewisePointHandleItem_h
