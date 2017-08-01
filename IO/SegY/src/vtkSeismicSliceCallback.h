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

#ifndef SEGYVISUALIZER2D_VTKSEISMICSLICECALLBACK_H
#define SEGYVISUALIZER2D_VTKSEISMICSLICECALLBACK_H

#include <vtkCommand.h>
#include <vtkPlane.h>
#include "vtkSeismicSliceWidget.h"
class vtkSeismicSliceCallback : public vtkCommand
{
public:
    static vtkSeismicSliceCallback* New() { return new vtkSeismicSliceCallback(); }
    void Execute(vtkObject* caller, unsigned long event, void* calldata);
    void SetClippingPlane(vtkPlane* plane){clippingPlane = plane;}
private:
    vtkSeismicSliceCallback();
    vtkPlane* clippingPlane;
    bool bUp;
};


#endif //SEGYVISUALIZER2D_VTKSEISMICSLICECALLBACK_H
