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
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void SetPiecewiseFunction(vtkPiecewiseFunction* t);
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

protected:
  vtkPiecewiseFunctionItem();
  virtual ~vtkPiecewiseFunctionItem();

  // Description:
  // Reimplemented to return the range of the piecewise function
  virtual void ComputeBounds(double bounds[4]);

  // Description
  // Compute the texture from the PiecewiseFunction
  virtual void ComputeTexture();

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseFunctionItem(const vtkPiecewiseFunctionItem &); // Not implemented.
  void operator=(const vtkPiecewiseFunctionItem &); // Not implemented.
};

#endif
