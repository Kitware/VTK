//
// Created by jia chen on 8/14/15.
//

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
