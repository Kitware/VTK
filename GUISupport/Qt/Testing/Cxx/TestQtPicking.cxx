/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQtPicking.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests picking actors with
// QVTKOpenGLStereoWidget/QVTKOpenGLWindow/QVTKOpenGLNativeWidget and vtkPropPicker.
#include "TestQtCommon.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCoordinate.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkMath.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPropPicker.h"
#include "vtkProperty2D.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include <QApplication>
#include <QSurfaceFormat>

#include <algorithm>
#include <cmath>
#include <vector>

int TestQtPicking(int argc, char* argv[])
{
  // Disable multisampling
  vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
  auto widgetOrWindow = detail::create_widget_or_window(type, renWin);
  auto interactor = renWin->GetInteractor();

  auto ren = vtkSmartPointer<vtkRenderer>::New();
  ren->GradientBackgroundOn();
  ren->SetBackground2(0.7, 0.7, 0.7);
  renWin->AddRenderer(ren);

  interactor->Render();

  const int NumSpheres = 5;
  const double SphereRadius = 0.5;

  // Add spheres arranged in a circle
  std::vector<vtkSmartPointer<vtkActor2D> > actors;
  const double Pi2 = 2.0 * vtkMath::Pi();
  const double step = Pi2 / NumSpheres;
  for (double theta = 0.0; theta < Pi2; theta += step)
  {
    auto source = vtkSmartPointer<vtkSphereSource>::New();
    const double x = sin(theta);
    const double y = cos(theta);
    const double z = 0.0;
    source->SetRadius(SphereRadius);
    source->SetCenter(x, y, z);

    auto coordinate = vtkSmartPointer<vtkCoordinate>::New();
    coordinate->SetCoordinateSystemToWorld();

    auto mapper = vtkSmartPointer<vtkPolyDataMapper2D>::New();
    mapper->SetInputConnection(source->GetOutputPort());
    mapper->SetTransformCoordinate(coordinate);

    auto actor = vtkSmartPointer<vtkActor2D>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.62, 0.81, 0.62);
    ren->AddActor(actor);
    actors.push_back(actor);
  }

  ren->GetActiveCamera()->SetPosition(0.0, 0.0, 9.0);

  detail::show(widgetOrWindow, QSize(300, 300));

  auto picker = vtkSmartPointer<vtkPropPicker>::New();

  auto coordinate = vtkSmartPointer<vtkCoordinate>::New();
  coordinate->SetCoordinateSystemToWorld();

  // Pick at sphere centers
  std::vector<vtkSmartPointer<vtkActor2D> > hits;
  for (double theta = 0.0; theta < Pi2; theta += step)
  {
    const double x = sin(theta);
    const double y = cos(theta);
    const double z = 0.0;
    coordinate->SetValue(x, y, z);

    const int* display = coordinate->GetComputedDisplayValue(ren);
    picker->Pick(display[0], display[1], 0.0, ren);
    auto actor = picker->GetActor2D();
    if (actor)
    {
      actor->GetProperty()->SetColor(0.89, 0.81, 0.67);
    }
    hits.push_back(actor);

    interactor->Render();
    app.processEvents();
  }

  // Pick outside of spheres
  std::vector<vtkSmartPointer<vtkActor2D> > misses;
  for (double theta = 0.0; theta < Pi2; theta += (0.5 * step))
  {
    const double x = 2.0 * sin(theta);
    const double y = 2.0 * cos(theta);
    const double z = 0.0;
    coordinate->SetValue(x, y, z);

    const int* display = coordinate->GetComputedDisplayValue(ren);
    picker->Pick(display[0], display[1], 0.0, ren);
    auto actor = picker->GetActor2D();
    if (actor)
    {
      actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    }
    misses.push_back(actor);

    interactor->Render();
    app.processEvents();
  }

  // Pick in center of window
  {
    coordinate->SetValue(0.0, 0.0, 0.0);
    const int* display = coordinate->GetComputedDisplayValue(ren);

    picker->Pick(display[0], display[1], 0.0, ren);
    auto actor = picker->GetActor2D();
    if (actor)
    {
      actor->GetProperty()->SetColor(1.0, 0.0, 0.0);
    }
    misses.push_back(actor);

    interactor->Render();
    app.processEvents();
  }

  // Check that picks on spheres hit correct actors
  bool hitsOk = (hits == actors);
  if (!hitsOk)
  {
    std::cout << "ERROR: Picking actors failed" << std::endl;
    return EXIT_FAILURE;
  }

  // Check that picks outside of spheres hit no actors
  bool missesOk = std::all_of(misses.begin(), misses.end(),
    [](const vtkSmartPointer<vtkActor2D>& actor) { return (actor == nullptr); });
  if (!missesOk)
  {
    std::cout << "ERROR: Picking outside of actors failed" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
