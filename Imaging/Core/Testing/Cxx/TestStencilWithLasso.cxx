/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStencilWithLasso.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageData.h"
#include "vtkImageReader2.h"
#include "vtkPoints.h"
#include "vtkLassoStencilSource.h"
#include "vtkTransform.h"
#include "vtkImageShiftScale.h"
#include "vtkImageStencil.h"
#include "vtkCamera.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkInteractorStyleImage.h"
#include "vtkImageSlice.h"
#include "vtkImageSliceMapper.h"
#include "vtkImageProperty.h"
#include "vtkTestUtilities.h"
#include "vtkSmartPointer.h"

#include <string>

int TestStencilWithLasso(int argc, char *argv[])
{
  // a simple concave closed contour
  static double lassoPoints[7][3] = {
    { 30, 50 },
    { 50, 90 },
    { 150, 50 },
    { 180, 100 },
    { 100, 170 },
    { 60, 170 },
    { 30, 50 },
  };

  int extent[6] = { 0, 63, 0, 63, 1, 93 };
  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 3.2, 3.2, 1.5 };
  double center[3] = { 0.5*3.2*63, 0.5*3.2*63, 0.5*1.5*94 };

  char* fname = vtkTestUtilities::ExpandDataFileName(
    argc, argv, "Data/headsq/quarter");
  std::string filename = fname;
  delete [] fname;

  vtkSmartPointer<vtkImageReader2> reader =
    vtkSmartPointer<vtkImageReader2>::New();
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(extent);
  reader->SetDataOrigin(origin);
  reader->SetDataSpacing(spacing);
  reader->SetFilePrefix(filename.c_str());

  vtkSmartPointer<vtkImageShiftScale> shiftScale =
    vtkSmartPointer<vtkImageShiftScale>::New();
  shiftScale->SetInputConnection(reader->GetOutputPort());
  shiftScale->SetScale(0.5);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetSize(256, 256);

  vtkSmartPointer<vtkInteractorStyleImage> style =
    vtkSmartPointer<vtkInteractorStyleImage>::New();

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  for (int j = 0; j < 4; j++)
  {
    int orientation = 2 - (j%3);

    vtkSmartPointer<vtkPoints> points =
      vtkSmartPointer<vtkPoints>::New();
    // exercise both open and closed contours
    vtkIdType npoints = (j < 2 ? 7 : 6);
    points->SetNumberOfPoints(npoints);
    for (vtkIdType i = 0; i < npoints; i++)
    {
      double point[3] = { 0.0, 0.0, 0.0 };
      point[(orientation + 1)%3] = lassoPoints[i][0];
      point[(orientation + 2)%3] = lassoPoints[i][1];
      points->SetPoint(i, point);
    }

    vtkSmartPointer<vtkLassoStencilSource> stencilSource =
      vtkSmartPointer<vtkLassoStencilSource>::New();
    stencilSource->SetOutputOrigin(origin);
    stencilSource->SetOutputSpacing(spacing);
    stencilSource->SetOutputWholeExtent(extent);
    stencilSource->SetPoints(points);
    stencilSource->SetShapeToSpline();
    stencilSource->SetSliceOrientation(orientation);
    if (j == 3)
    {
      // exercise the polygon code, too
      stencilSource->SetShapeToPolygon();
    }

    vtkSmartPointer<vtkImageStencil> stencil =
      vtkSmartPointer<vtkImageStencil>::New();
    stencil->SetInputConnection(0, shiftScale->GetOutputPort());
    stencil->SetInputConnection(1, reader->GetOutputPort());
    stencil->SetStencilConnection(stencilSource->GetOutputPort());
    stencil->Update();

    vtkSmartPointer<vtkImageSliceMapper> mapper =
      vtkSmartPointer<vtkImageSliceMapper>::New();
    mapper->BorderOn();
    mapper->SetInputConnection(stencil->GetOutputPort());
    mapper->SliceAtFocalPointOn();
    mapper->SetOrientation(orientation);

    vtkSmartPointer<vtkImageSlice> actor =
      vtkSmartPointer<vtkImageSlice>::New();
    actor->GetProperty()->SetColorWindow(2000.0);
    actor->GetProperty()->SetColorLevel(1000.0);
    actor->SetMapper(mapper);

    vtkSmartPointer<vtkRenderer> renderer =
      vtkSmartPointer<vtkRenderer>::New();
    renderer->SetViewport(0.5*(j%2), 0.5*(1 - j/2),
                          0.5*((j%2) + 1), 0.5*(2 - j/2));
    renderer->AddViewProp(actor);
    renWin->AddRenderer(renderer);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->ParallelProjectionOn();
    camera->SetParallelScale(0.25*100.8*spacing[1]);
    double position[3] = { center[0], center[1], center[2] };
    camera->SetFocalPoint(position);
    position[orientation] += 10.0;
    camera->SetPosition(position);
    if (orientation == 2)
    {
      camera->SetViewUp(0.0, 1.0, 0.0);
    }
    else
    {
      camera->SetViewUp(0.0, 0.0, -1.0);
    }
    camera->SetClippingRange(5.0, 15.0);
  }

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
