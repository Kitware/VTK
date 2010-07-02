/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkControlPointsItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef __vtkControlPointsItem_h
#define __vtkControlPointsItem_h

#include "vtkContextItem.h"

class vtkCallbackCommand;
class vtkContext2D;
class vtkPoints2D;

class VTK_CHARTS_EXPORT vtkControlPointsItem: public vtkContextItem
{
public:
  vtkTypeMacro(vtkControlPointsItem, vtkContextItem);
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual bool Paint(vtkContext2D *painter);
protected:
  vtkControlPointsItem();
  virtual ~vtkControlPointsItem();

  static void CallComputePoints(vtkObject* sender, unsigned long event, void* receiver, void* params);
  virtual void ComputePoints();
  void DrawPoints(vtkContext2D* painter, vtkPoints2D* points);

  vtkPoints2D*    Points;
  vtkPoints2D*    HighlightPoints;
  vtkCallbackCommand* Callback;

private:
  vtkControlPointsItem(const vtkControlPointsItem &); // Not implemented.
  void operator=(const vtkControlPointsItem &);   // Not implemented.
};

#endif
