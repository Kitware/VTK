/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImageProbeFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkImageProbeFilter.h"

#include "vtkNew.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetMapper.h"
#include "vtkImageCast.h"
#include "vtkImageData.h"
#include "vtkImageInterpolator.h"
#include "vtkImageMapToColors.h"
#include "vtkImageProperty.h"
#include "vtkImageReader2.h"
#include "vtkInteractorStyleImage.h"
#include "vtkLookupTable.h"
#include "vtkMath.h"
#include "vtkPlaneSource.h"
#include "vtkPoints.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTransform.h"

#include <string>

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestImageProbeFilter(int argc, char* argv[])
{
  // render window and interactor
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);
  renWin->SetSize(512, 512);

  // image file information (because the file is raw)
  int extent[6] = { 0, 63, 0, 63, 1, 93 };
  double origin[3] = { 0.0, 0.0, 0.0 };
  double spacing[3] = { 3.2, 3.2, 1.5 };
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");
  std::string filename = fname;
  delete[] fname;

  // read a CT image
  vtkNew<vtkImageReader2> reader;
  reader->SetDataByteOrderToLittleEndian();
  reader->SetDataExtent(extent);
  reader->SetDataOrigin(origin);
  reader->SetDataSpacing(spacing);
  reader->SetFilePrefix(filename.c_str());

  // grayscale lookup table
  vtkNew<vtkLookupTable> table;
  table->SetRampToLinear();
  table->SetRange(0.0, 4095.0);
  table->SetValueRange(0.0, 1.0);
  table->SetSaturationRange(0.0, 0.0);
  table->Build();

  // create RGBA data for rendering
  vtkNew<vtkImageMapToColors> colors;
  colors->SetOutputFormatToRGBA();
  colors->SetInputConnection(reader->GetOutputPort());
  colors->SetLookupTable(table);

  { // probe RGBA data onto a plane, default probing

    // a plane for probing with
    vtkNew<vtkPlaneSource> plane;
    plane->SetOrigin(0.0, 0.0, 69.75);
    plane->SetPoint1(201.6, 0.0, 69.75);
    plane->SetPoint2(0.0, 201.6, 69.75);
    plane->SetXResolution(63);
    plane->SetYResolution(63);

    // the probe filter
    vtkNew<vtkImageProbeFilter> probe;
    probe->SetSourceConnection(colors->GetOutputPort());
    probe->SetInputConnection(plane->GetOutputPort());

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(probe->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> ren;
    ren->AddViewProp(actor);
    ren->ResetCamera();
    ren->GetActiveCamera()->ParallelProjectionOn();
    ren->GetActiveCamera()->SetParallelScale(102.4);
    ren->SetViewport(0.0, 0.5, 0.5, 1.0);
    renWin->AddRenderer(ren);
  }

  { // probe RGBA data onto a plane, via cubic interpolation

    // a plane for probing with
    vtkNew<vtkPlaneSource> plane;
    plane->SetOrigin(0.0, 0.0, 69.75);
    plane->SetPoint1(201.6, 0.0, 69.75);
    plane->SetPoint2(0.0, 201.6, 69.75);
    plane->SetXResolution(255);
    plane->SetYResolution(255);

    // an interpolator (cubic interpolation)
    vtkNew<vtkImageInterpolator> interpolator;
    interpolator->SetInterpolationModeToCubic();

    // the probe filter
    vtkNew<vtkImageProbeFilter> probe;
    probe->SetInterpolator(interpolator);
    probe->SetSourceConnection(colors->GetOutputPort());
    probe->SetInputConnection(plane->GetOutputPort());

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(probe->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> ren;
    ren->AddViewProp(actor);
    ren->ResetCamera();
    ren->GetActiveCamera()->ParallelProjectionOn();
    ren->GetActiveCamera()->SetParallelScale(102.4);
    ren->SetViewport(0.5, 0.5, 1.0, 1.0);
    renWin->AddRenderer(ren);
  }

  { // probe int data onto oblique plane, via cubic interpolation

    // plane information
    double center[3] = { 100.8, 100.8, 69.75 };
    double p0[3] = { 0.0, 0.0, 69.75 };
    double p1[3] = { 201.6, 0.0, 69.75 };
    double p2[3] = { 0.0, 201.6, 69.75 };

    // a transform for going oblique
    vtkNew<vtkTransform> transform;
    transform->PostMultiply();
    vtkMath::MultiplyScalar(center, -1.0);
    transform->Translate(center);
    vtkMath::MultiplyScalar(center, -1.0);
    transform->RotateWXYZ(-20.0, 0.99388, 0.0, 0.11043);
    transform->Translate(center);
    transform->TransformPoint(p0, p0);
    transform->TransformPoint(p1, p1);
    transform->TransformPoint(p2, p2);

    // a plane for probing with
    vtkNew<vtkPlaneSource> plane;
    plane->SetOrigin(p0);
    plane->SetPoint1(p1);
    plane->SetPoint2(p2);
    plane->SetXResolution(255);
    plane->SetYResolution(255);

    // an interpolator (cubic interpolation)
    vtkNew<vtkImageInterpolator> interpolator;
    interpolator->SetInterpolationModeToCubic();

    // the probe filter
    vtkNew<vtkImageProbeFilter> probe;
    probe->SetInterpolator(interpolator);
    probe->SetSourceConnection(reader->GetOutputPort());
    probe->SetInputConnection(plane->GetOutputPort());

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(probe->GetOutputPort());
    mapper->SetLookupTable(table);
    mapper->UseLookupTableScalarRangeOn();

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> ren;
    ren->AddViewProp(actor);
    ren->ResetCamera();
    ren->GetActiveCamera()->ParallelProjectionOn();
    ren->GetActiveCamera()->SetParallelScale(102.4);
    ren->SetViewport(0.0, 0.0, 0.5, 0.5);
    renWin->AddRenderer(ren);
  }

  { // probe float data onto sphere, via linear interpolation

    // a sphere for probing with
    vtkNew<vtkSphereSource> surface;
    surface->SetCenter(100.8, 100.8, 69.75);
    surface->SetRadius(60.0);
    surface->SetPhiResolution(200);
    surface->SetThetaResolution(200);

    // use floating-point here for coverage
    vtkNew<vtkImageCast> cast;
    cast->SetInputConnection(reader->GetOutputPort());
    cast->SetOutputScalarTypeToFloat();

    // an interpolator (linear interpolation)
    vtkNew<vtkImageInterpolator> interpolator;

    // the probe filter
    vtkNew<vtkImageProbeFilter> probe;
    probe->SetInterpolator(interpolator);
    probe->SetSourceConnection(cast->GetOutputPort());
    probe->SetInputConnection(surface->GetOutputPort());

    vtkNew<vtkDataSetMapper> mapper;
    mapper->SetInputConnection(probe->GetOutputPort());
    mapper->SetLookupTable(table);
    mapper->UseLookupTableScalarRangeOn();

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);

    vtkNew<vtkRenderer> ren;
    ren->AddViewProp(actor);
    ren->ResetCamera();
    ren->GetActiveCamera()->ParallelProjectionOn();
    ren->GetActiveCamera()->SetParallelScale(102.4);
    ren->SetViewport(0.5, 0.0, 1.0, 0.5);
    renWin->AddRenderer(ren);
  }

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
