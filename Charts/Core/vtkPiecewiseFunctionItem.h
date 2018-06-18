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

/// vtkPiecewiseFunctionItem internal uses vtkPlot::Color, white by default
class VTKCHARTSCORE_EXPORT vtkPiecewiseFunctionItem: public vtkScalarsToColorsItem
{
public:
  static vtkPiecewiseFunctionItem* New();
  vtkTypeMacro(vtkPiecewiseFunctionItem, vtkScalarsToColorsItem);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  void SetPiecewiseFunction(vtkPiecewiseFunction* t);
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

protected:
  vtkPiecewiseFunctionItem();
  ~vtkPiecewiseFunctionItem() override;

  // Description:
  // Reimplemented to return the range of the piecewise function
  void ComputeBounds(double bounds[4]) override;

  // Description
  // Compute the texture from the PiecewiseFunction
  void ComputeTexture() override;

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseFunctionItem(const vtkPiecewiseFunctionItem &) = delete;
  void operator=(const vtkPiecewiseFunctionItem &) = delete;
};

#endif
