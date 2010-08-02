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

//#include "vtkContextItem.h"
#include "vtkPlot.h"

class vtkCallbackCommand;
class vtkImageData;
class vtkPoints2D;

class VTK_CHARTS_EXPORT vtkScalarsToColorsItem: public vtkPlot
{
public:
  vtkTypeMacro(vtkScalarsToColorsItem, vtkPlot);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void GetBounds(double bounds[4]);
  virtual bool Paint(vtkContext2D *painter);
protected:
  vtkScalarsToColorsItem();
  virtual ~vtkScalarsToColorsItem();

  virtual void ComputeTexture() = 0;
  static void OnScalarsToColorsModified(vtkObject* caller, unsigned long eid, void *clientdata, void* calldata);
  virtual void ScalarsToColorsModified(vtkObject* caller, unsigned long eid, void* calldata);

  vtkImageData*       Texture;
  bool                Interpolate;
  vtkPoints2D*        Shape;
  vtkCallbackCommand* Callback;
private:
  vtkScalarsToColorsItem(const vtkScalarsToColorsItem &); // Not implemented.
  void operator=(const vtkScalarsToColorsItem &);   // Not implemented.
};

#endif
