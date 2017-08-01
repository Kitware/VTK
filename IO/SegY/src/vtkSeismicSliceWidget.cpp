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

#include "vtkSeismicSliceWidget.h"

vtkSeismicSliceWidget::vtkSeismicSliceWidget()
{
    plane = vtkPlane::New();
}

vtkSeismicSliceWidget::~vtkSeismicSliceWidget()
{
    if (plane)
    {
        plane->Delete();
        plane = NULL;
    }
}

void vtkSeismicSliceWidget::SetPlaneOrientationToXAxes()
{
    this->vtkImagePlaneWidget::SetPlaneOrientationToXAxes();

    plane->SetOrigin(GetOrigin());
    plane->SetNormal(GetNormal());
}

void vtkSeismicSliceWidget::SetPlaneOrientationToYAxes()
{
    this->vtkImagePlaneWidget::SetPlaneOrientationToYAxes();

    plane->SetOrigin(GetOrigin());
    plane->SetNormal(GetNormal());
}

void vtkSeismicSliceWidget::SetPlaneOrientationToZAxes()
{
    this->vtkImagePlaneWidget::SetPlaneOrientationToXAxes();

    plane->SetOrigin(GetOrigin());
    plane->SetNormal(GetNormal());
}

void vtkSeismicSliceWidget::SetSlicePosition(double position)
{
    this->vtkImagePlaneWidget::SetSlicePosition(position);

    plane->SetOrigin(GetOrigin());
    plane->SetNormal(GetNormal());
}
