/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlane.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSeismicSliceCallback.h"
#include <vtkDataSet.h>

void vtkSeismicSliceCallback::Execute(vtkObject *caller, unsigned long event, void *calldata)
{
    auto widget = reinterpret_cast<vtkSeismicSliceWidget*>(caller);

    if(widget == NULL)
        return;

    double pos0 = widget->GetSlicePosition();


    double newPos = bUp ? pos0 - 1 : pos0 + 1;
    widget->SetSlicePosition( newPos);

    if(widget->GetSlicePosition() == pos0)
        bUp = !bUp;
}

vtkSeismicSliceCallback::vtkSeismicSliceCallback()
{
    bUp = true;
}
