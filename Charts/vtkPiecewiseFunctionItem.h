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

class VTK_CHARTS_EXPORT vtkPiecewiseFunctionItem: public vtkScalarsToColorsItem
{
public:
  static vtkPiecewiseFunctionItem* New();
  vtkTypeMacro(vtkPiecewiseFunctionItem, vtkScalarsToColorsItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  void SetPiecewiseFunction(vtkPiecewiseFunction* t);
  vtkGetObjectMacro(PiecewiseFunction, vtkPiecewiseFunction);

  vtkSetVector3Macro(Color, unsigned char);
  vtkGetVector3Macro(Color, unsigned char);

  void SetMaskAboveCurve(bool mask);
  vtkGetMacro(MaskAboveCurve, bool);
protected:
  vtkPiecewiseFunctionItem();
  virtual ~vtkPiecewiseFunctionItem();

  // Description
  // Compute the texture from the PiecewiseFunction
  virtual void ComputeTexture();
  virtual void ScalarsToColorsModified(vtkObject* object,
                                       unsigned long eid,
                                       void* calldata);

  vtkPiecewiseFunction* PiecewiseFunction;
  unsigned char         Color[3];
  bool                  MaskAboveCurve;

private:
  vtkPiecewiseFunctionItem(const vtkPiecewiseFunctionItem &); // Not implemented.
  void operator=(const vtkPiecewiseFunctionItem &); // Not implemented.
};

#endif
