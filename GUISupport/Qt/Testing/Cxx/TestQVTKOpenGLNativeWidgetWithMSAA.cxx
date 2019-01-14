/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQVTKOpenGLNativeWidgetWithMSAA.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests QVTKOpenGLNativeWidget with MSAA (based on TestQVTKOpenGLNativeWidget)
#include "QVTKOpenGLNativeWidget.h"
#include "vtkActor.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

#include <QApplication>
#include <QSurfaceFormat>

int TestQVTKOpenGLNativeWidgetWithMSAA(int argc, char* argv[])
{
  // enable multisampling.
  vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(8);
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

  QApplication app(argc, argv);

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  vtkNew<vtkGenericOpenGLRenderWindow> window;

  QVTKOpenGLNativeWidget widget;
  widget.SetRenderWindow(window);

  vtkNew<vtkRenderer> ren;
  ren->SetGradientBackground(1);
  ren->SetBackground2(0.7, 0.7, 0.7);
  window->AddRenderer(ren);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  ren->AddActor(actor);

  vtktesting->SetRenderWindow(window);
  widget.show();
  app.processEvents();

  int retVal = vtktesting->RegressionTest(10);
  switch (retVal)
  {
    case vtkTesting::DO_INTERACTOR:
      return app.exec();
    case vtkTesting::FAILED:
    case vtkTesting::NOT_RUN:
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
