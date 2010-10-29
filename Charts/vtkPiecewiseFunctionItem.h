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

#ifndef __vtkPiecewiseFunctionItem_h
#define __vtkPiecewiseFunctionItem_h

#include "vtkScalarsToColorsItem.h"

class vtkPiecewiseFunction;
class vtkImageData;

/// vtkPiecewiseFunctionItem internall uses vtkPlot::Color, white by default
class VTK_CHARTS_EXPORT vtkPiecewiseFunctionItem: public vtkScalarsToColorsItem
{
public:
  static vtkPiecewiseFunctionItem* New();
  vtkTypeMacro(vtkPiecewiseFunctionItem, vtkScalarsToColorsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Reimplemented to return the range of the piecewise function
  virtual void GetBounds(double bounds[4]);

  void SetPiecewiseFunction(vtkPiecewiseFunction* t);
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

protected:
  vtkPiecewiseFunctionItem();
  virtual ~vtkPiecewiseFunctionItem();

  // Description
  // Compute the texture from the PiecewiseFunction
  virtual void ComputeTexture();

  vtkPiecewiseFunction* PiecewiseFunction;

private:
  vtkPiecewiseFunctionItem(const vtkPiecewiseFunctionItem &); // Not implemented.
  void operator=(const vtkPiecewiseFunctionItem &); // Not implemented.
};

#endif
