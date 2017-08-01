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

protected:
    vtkSeismicSliceWidget();
    ~vtkSeismicSliceWidget();

private:
    vtkPlane* plane;
};


#endif //SEGYVISUALIZER2D_VTKSEISMICSLICEINTERACTOR_H
