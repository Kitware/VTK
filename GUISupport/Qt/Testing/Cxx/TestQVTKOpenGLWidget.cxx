/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQVTKOpenGLWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests QVTKOpenGLWidget

#include "QVTKOpenGLWidget.h"

#include "vtkActor.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

#include <QApplication>
#include <QSurfaceFormat>
#include <qmainwindow.h>

int TestQVTKOpenGLWidget(int argc, char* argv[])
{
  // disable multisampling.
  vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());

  QApplication app(argc, argv);

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  QVTKOpenGLWidget widget;
  {
    vtkNew<vtkGenericOpenGLRenderWindow> window0;
    widget.SetRenderWindow(window0);
    app.processEvents();
  }

  // make sure rendering works correctly after switching to a new render window
  vtkNew<vtkGenericOpenGLRenderWindow> window;
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

  int* windowSize = window->GetSize();
  int* screenSize = window->GetScreenSize();
  if (screenSize[0] < windowSize[0] || screenSize[1] < windowSize[1])
  {
    std::cout << "Expected vtkGenericOpenGLRenderWindow::GetScreenSize() "
      "dimensions to be larger than the render window size"
      << std::endl;
    return EXIT_FAILURE;
  }

  widget.show();

  // Make sure that the widget context is valid before making OpenGL calls.
  // This should only take up to 4 calls to processEvents(). If this test keeps
  // timing out, consider that the widget initialization is broken.
  while (!widget.isValid())
  {
    app.processEvents();
  }

  vtktesting->SetRenderWindow(window);

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
