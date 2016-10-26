//
// Created by jia chen on 8/14/15.
//

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
