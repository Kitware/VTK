/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewisePointHandleItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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
#include "vtkWeakPointer.h" // Needed for weak pointer to the PiecewiseFunction.

class vtkContext2D;
class vtkPiecewiseFunction;
class vtkCallbackCommand;
class vtkAbstractContextItem;

class VTKCHARTSCORE_EXPORT vtkPiecewisePointHandleItem : public vtkContextItem
{
public:
  vtkTypeMacro(vtkPiecewisePointHandleItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  static vtkPiecewisePointHandleItem *New();
  static void CallRedraw(vtkObject* sender, unsigned long event, void* receiver, void* params);

  /**
   * Set the parent item, which should be a vtkControlPointItem
   */
  virtual void SetParent(vtkAbstractContextItem *parent);

  /**
   * Paint event for the item.
   */
  virtual bool Paint(vtkContext2D *painter);

  //@{
  /**
   * The current point id in the piecewise function being handled.
   */
  vtkSetMacro(CurrentPointIndex, vtkIdType);
  vtkGetMacro(CurrentPointIndex, vtkIdType);
  //@}

  //@{
  /**
   * Set the PieceWiseFunction the handles will manipulate
   */
  virtual void SetPiecewiseFunction(vtkPiecewiseFunction* piecewiseFunc);
  vtkWeakPointer<vtkPiecewiseFunction> GetPiecewiseFunction();
  //@}

  /**
   * Returns the index of the handle if pos is over any of the handles,
   * otherwise return -1;
   */
  int IsOverHandle(float* pos);

  /**
   * Returns true if the supplied x, y coordinate is inside the item.
   */
  virtual bool Hit(const vtkContextMouseEvent &mouse);

  /**
   * Mouse move event.
   */
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button down event.
   */
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

  /**
   * Mouse button release event.
   */
  virtual bool MouseButtonReleaseEvent(const vtkContextMouseEvent &mouse);

protected:
  vtkPiecewisePointHandleItem();
  ~vtkPiecewisePointHandleItem();

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
  vtkPiecewisePointHandleItem(const vtkPiecewisePointHandleItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPiecewisePointHandleItem &) VTK_DELETE_FUNCTION;

  class InternalPiecewisePointHandleInfo;
  InternalPiecewisePointHandleInfo* Internal;

};

#endif //vtkPiecewisePointHandleItem_h
