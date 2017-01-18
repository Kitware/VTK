/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPiecewiseFunctionItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkPiecewiseFunctionItem_h
#define vtkPiecewiseFunctionItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkScalarsToColorsItem.h"

class vtkPiecewiseFunction;
class vtkImageData;

/// vtkPiecewiseFunctionItem internall uses vtkPlot::Color, white by default
class VTKCHARTSCORE_EXPORT vtkPiecewiseFunctionItem: public vtkScalarsToColorsItem
{
public:
  static vtkPiecewiseFunctionItem* New();
  vtkTypeMacro(vtkPiecewiseFunctionItem, vtkScalarsToColorsItem);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  void SetPiecewiseFunction(vtkPiecewiseFunction* t);
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

protected:
  vtkPiecewiseFunctionItem();
  ~vtkPiecewiseFunctionItem() VTK_OVERRIDE;

  // Description:
  // Reimplemented to return the range of the piecewise function
  void ComputeBounds(double bounds[4]) VTK_OVERRIDE;

  // Description
  // Compute the texture from the PiecewiseFunction
  void ComputeTexture() VTK_OVERRIDE;

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseFunctionItem(const vtkPiecewiseFunctionItem &) VTK_DELETE_FUNCTION;
  void operator=(const vtkPiecewiseFunctionItem &) VTK_DELETE_FUNCTION;
};

#endif
