/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkColorTransferFunctionItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkColorTransferFunctionItem_h
#define __vtkColorTransferFunctionItem_h

#include "vtkChartsCoreModule.h" // For export macro
#include "vtkScalarsToColorsItem.h"

class vtkColorTransferFunction;
class vtkImageData;

// Description:
// vtkPlot::Color, vtkPlot::Brush, vtkScalarsToColors::DrawPolyLine,
// vtkScalarsToColors::MaskAboveCurve have no effect here.
class VTKCHARTSCORE_EXPORT vtkColorTransferFunctionItem: public vtkScalarsToColorsItem
{
public:
  static vtkColorTransferFunctionItem* New();
  vtkTypeMacro(vtkColorTransferFunctionItem, vtkScalarsToColorsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void SetColorTransferFunction(vtkColorTransferFunction* t);
  vtkGetObjectMacro(ColorTransferFunction, vtkColorTransferFunction);

protected:
  vtkColorTransferFunctionItem();
  virtual ~vtkColorTransferFunctionItem();

  // Description:
  // Reimplemented to return the range of the lookup table
  virtual void ComputeBounds(double bounds[4]);

  virtual void ComputeTexture();
  vtkColorTransferFunction* ColorTransferFunction;
private:
  vtkColorTransferFunctionItem(const vtkColorTransferFunctionItem&); // Not implemented
  void operator=(const vtkColorTransferFunctionItem&); // Not implemented
};

#endif
