//
// Created by jia chen on 8/14/15.
//

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