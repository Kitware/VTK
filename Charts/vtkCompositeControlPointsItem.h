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

#ifndef __vtkCompositeControlPointsItem_h
#define __vtkCompositeControlPointsItem_h

#include "vtkColorTransferControlPointsItem.h"

class vtkPiecewiseFunction;

class VTK_CHARTS_EXPORT vtkCompositeControlPointsItem:
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

  enum PointsFunction{
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

protected:
  vtkCompositeControlPointsItem();
  virtual ~vtkCompositeControlPointsItem();

  virtual unsigned long int GetControlPointsMTime();
  virtual void ComputePoints();

  virtual int GetNumberOfPoints()const;
  virtual void DrawPoint(vtkContext2D* painter, vtkIdType index);
  virtual void GetControlPoint(vtkIdType index, double* pos);
  virtual void SetControlPoint(vtkIdType index, double *point);
  virtual void EditPoint(float tX, float tY);

  void MergeTransferFunctions();
  void SilentMergeTransferFunctions();

  int                   PointsFunction;
  vtkPiecewiseFunction* OpacityFunction;
  bool                  Updating;

private:
  vtkCompositeControlPointsItem(const vtkCompositeControlPointsItem &); // Not implemented.
  void operator=(const vtkCompositeControlPointsItem &);   // Not implemented.
};

#endif
