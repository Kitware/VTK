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

// .NAME vtkCompositeControlPointsItem - Control points for
// vtkCompositeFunction.
// .SECTION Description
// vtkCompositeControlPointsItem draws the control points of a vtkPiecewiseFunction
// and a vtkColorTransferFunction.
// .SECTION See Also
// vtkControlPointsItem
// vtkColorTransferControlPointsItem
// vtkCompositeTransferFunctionItem
// vtkPiecewisePointHandleItem

#ifndef __vtkCompositeControlPointsItem_h
#define __vtkCompositeControlPointsItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkColorTransferControlPointsItem.h"

class vtkPiecewiseFunction;
class vtkPiecewisePointHandleItem;

class VTKCHARTSCORE_EXPORT vtkCompositeControlPointsItem:
  public vtkColorTransferControlPointsItem
{
public:
  vtkTypeMacro(vtkCompositeControlPointsItem, vtkColorTransferControlPointsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Creates a piecewise control points object
  static vtkCompositeControlPointsItem* New();

  // Description:
  // Set the color transfer function to draw its points
  virtual void SetColorTransferFunction(vtkColorTransferFunction* function);

  // Description
  // Utility function that calls SetPiecewiseFunction()
  void SetOpacityFunction(vtkPiecewiseFunction* opacity);
  vtkGetObjectMacro(OpacityFunction, vtkPiecewiseFunction);

  enum PointsFunctionType{
    ColorPointsFunction = 1,
    OpacityPointsFunction = 2,
    ColorAndOpacityPointsFunction = 3
  };
  // Description:
  // PointsFunction controls wether the points represent the
  // ColorTransferFunction, OpacityTransferFunction or both.
  // If ColorPointsFunction, only the points of the ColorTransfer function are
  // used.
  // If OpacityPointsFunction, only the points of the Opacity function are used
  // If ColorAndOpacityPointsFunction, the points of both functions are shared
  // by both functions.
  // ColorAndOpacityPointsFunction by default.
  // Note: Set the mode before the functions are set. ColorPointsFunction is
  // not fully supported.
  vtkSetMacro(PointsFunction, int);
  vtkGetMacro(PointsFunction, int);

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
  // If UseOpacityPointHandles is true, when the current point is
  // double clicked, a vtkPiecewisePointHandleItem will show up so
  // that the sharpness and mid point can be adjusted in the scene
  // with those handles
  // False by default.
  vtkSetMacro(UseOpacityPointHandles, bool);
  vtkGetMacro(UseOpacityPointHandles, bool);

  // Description:
  // Mouse move event. To take care of some special Key stroke
  virtual bool MouseMoveEvent(const vtkContextMouseEvent &mouse);
  virtual bool MouseDoubleClickEvent(const vtkContextMouseEvent &mouse);
  virtual bool MouseButtonPressEvent(const vtkContextMouseEvent &mouse);

protected:
  vtkCompositeControlPointsItem();
  virtual ~vtkCompositeControlPointsItem();

  virtual void emitEvent(unsigned long event, void* params);

  virtual unsigned long int GetControlPointsMTime();

  virtual vtkIdType GetNumberOfPoints()const;
  virtual void DrawPoint(vtkContext2D* painter, vtkIdType index);
  virtual void GetControlPoint(vtkIdType index, double* pos)const;
  virtual void SetControlPoint(vtkIdType index, double *point);
  virtual void EditPoint(float tX, float tY);
  virtual void EditPointCurve(vtkIdType idx);

  void MergeTransferFunctions();
  void SilentMergeTransferFunctions();

  int                   PointsFunction;
  vtkPiecewiseFunction* OpacityFunction;
  vtkPiecewisePointHandleItem* OpacityPointHandle;
  bool UseOpacityPointHandles;

private:
  vtkCompositeControlPointsItem(const vtkCompositeControlPointsItem &); // Not implemented.
  void operator=(const vtkCompositeControlPointsItem &);   // Not implemented.
};

#endif
