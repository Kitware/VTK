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

#include "vtkScalarsToColorsItem.h"

class vtkColorTransferFunction;
class vtkImageData;

class VTK_CHARTS_EXPORT vtkColorTransferFunctionItem: public vtkScalarsToColorsItem
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

  virtual void ComputeTexture();

  vtkColorTransferFunction* ColorTransferFunction;
};

#endif
