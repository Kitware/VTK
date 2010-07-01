/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColorsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkScalarsToColorsItem_h
#define __vtkScalarsToColorsItem_h

#include "vtkContextItem.h"

class vtkImageData;

class VTK_CHARTS_EXPORT vtkScalarsToColorsItem: public vtkContextItem
{
public:
  vtkTypeMacro(vtkScalarsToColorsItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual bool Paint(vtkContext2D *painter);
protected:
  vtkScalarsToColorsItem();
  virtual ~vtkScalarsToColorsItem();

  virtual void ComputeTexture() = 0;

  vtkImageData*   Texture;
  bool            Interpolate;
private:
  vtkScalarsToColorsItem(const vtkScalarsToColorsItem &); // Not implemented.
  void operator=(const vtkScalarsToColorsItem &);   // Not implemented.
};

#endif
