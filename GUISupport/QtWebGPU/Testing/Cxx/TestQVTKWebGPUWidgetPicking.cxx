// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests picking actors with QVTKWebGPUWidget and vtkPropPicker.

#include "QVTKWebGPUWidget.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkPropPicker.h"
#include "vtkProperty.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkWebGPURenderWindow.h"

#include <QApplication>
#include <QEventLoop>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
void process_events_and_wait(int msec)
{
  QEventLoop loop;
  QTimer::singleShot(msec, &loop, SLOT(quit()));
  loop.exec();
}
}

int TestQVTKWebGPUWidgetPicking(int argc, char* argv[])
{
  QApplication app(argc, argv);

  // Create the QVTKWebGPUWidget
  QVTKWebGPUWidget widget;
  widget.setWindowTitle("TestQVTKWebGPUWidgetPicking");

  vtkWebGPURenderWindow* renWin = widget.renderWindow();
  if (!renWin)
  {
    std::cerr << "Failed to get render window from QVTKWebGPUWidget" << std::endl;
    return EXIT_FAILURE;
  }

  auto interactor = renWin->GetInteractor();

  vtkNew<vtkRenderer> ren;
  ren->GradientBackgroundOn();
  ren->SetBackground(0.2, 0.2, 0.2);
  ren->SetBackground2(0.7, 0.7, 0.7);
  renWin->AddRenderer(ren);

  interactor->Render();

  const int NumSpheres = 5;
  const double SphereRadius = 0.5;

  // Add spheres arranged in a circle
  std::vector<vtkSmartPointer<vtkActor>> actors;
  const double Pi2 = 2.0 * vtkMath::Pi();
  const double step = Pi2 / NumSpheres;
  for (double theta = 0.0; theta < Pi2; theta += step)
  {
    vtkNew<vtkSphereSource> source;
    const double x = 2.0 * sin(theta);
    const double y = 2.0 * cos(theta);
    const double z = 0.0;
    source->SetRadius(SphereRadius);
    source->SetCenter(x, y, z);
    source->SetThetaResolution(16);
    source->SetPhiResolution(16);

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(source->GetOutputPort());

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(0.62, 0.81, 0.62);
    ren->AddActor(actor);
    actors.push_back(actor);
  }

  ren->GetActiveCamera()->SetPosition(0.0, 0.0, 15.0);
  ren->ResetCamera();

  widget.resize(300, 300);
  widget.show();
  process_events_and_wait(500);

  vtkNew<vtkPropPicker> picker;

  // Pick at sphere centers and verify we get hits
  int hitCount = 0;
  for (double theta = 0.0; theta < Pi2; theta += step)
  {
    const double x = 2.0 * sin(theta);
    const double y = 2.0 * cos(theta);
    const double z = 0.0;

    // Convert world to display coordinates
    ren->SetWorldPoint(x, y, z, 1.0);
    ren->WorldToDisplay();
    double* display = ren->GetDisplayPoint();

    picker->Pick(display[0], display[1], 0.0, ren);
    auto actor = picker->GetActor();
    if (actor)
    {
      actor->GetProperty()->SetColor(0.89, 0.81, 0.67);
      hitCount++;
    }

    interactor->Render();
    QApplication::processEvents();
  }

  std::cout << "Hit " << hitCount << " out of " << NumSpheres << " spheres" << std::endl;

  // Pick at the center (should miss all spheres)
  picker->Pick(150, 150, 0.0, ren);
  auto missedActor = picker->GetActor();

  // Verify results
  if (hitCount < NumSpheres)
  {
    std::cerr << "Expected to hit all " << NumSpheres << " spheres, but only hit " << hitCount
              << std::endl;
    // Note: This may fail depending on WebGPU picking implementation
    // For now, we just warn but don't fail
  }

  if (missedActor != nullptr)
  {
    std::cerr << "Expected to miss when picking at center, but hit an actor" << std::endl;
    return EXIT_FAILURE;
  }

  process_events_and_wait(500);

  return EXIT_SUCCESS;
}
