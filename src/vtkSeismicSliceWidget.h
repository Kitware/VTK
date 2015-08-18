//
// Created by jia chen on 8/14/15.
//

#ifndef SEGYVISUALIZER2D_VTKSEISMICSLICEINTERACTOR_H
#define SEGYVISUALIZER2D_VTKSEISMICSLICEINTERACTOR_H
#include <vtkCommand.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkPlane.h>
#include <vtk3DWidget.h>
#include <vtkImagePlaneWidget.h>
#include <vtkContextMouseEvent.h>

class vtkSeismicSliceWidget : public vtkImagePlaneWidget
{
public:
    static vtkSeismicSliceWidget * New()
    {
        return new vtkSeismicSliceWidget();
    }
    vtkPlane* GetPlane() {return plane;}
    void SetPlaneOrientationToXAxes();
    void SetPlaneOrientationToYAxes();
    void SetPlaneOrientationToZAxes();

    void SetSlicePosition(double position);

private:
    vtkSeismicSliceWidget();
    vtkPlane* plane;
};


#endif //SEGYVISUALIZER2D_VTKSEISMICSLICEINTERACTOR_H
