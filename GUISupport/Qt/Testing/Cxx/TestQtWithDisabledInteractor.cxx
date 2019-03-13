/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQVTKOpenGLNativeWidgetWithDisabledInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests QVTKOpenGLNativeWidget with a vtkRenderWindowInteractor that has its
// EnableRender flag disabled.
#include "TestQtCommon.h"
#include "vtkActor.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

#include <QImage>

int TestQtWithDisabledInteractor(int argc, char* argv[])
{
  // Disable multisampling
  vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  auto vtktesting = vtkSmartPointer<vtkTesting>::New();
  vtktesting->AddArguments(argc, argv);

  auto widgetOrWindow = detail::create_widget_or_window(type, nullptr);

  auto renWin = detail::get_render_window(widgetOrWindow);
  auto ren = vtkSmartPointer<vtkRenderer>::New();
  ren->GradientBackgroundOn();
  ren->SetBackground2(0.7, 0.7, 0.7);
  renWin->AddRenderer(ren);
  renWin->Render();

  detail::show(widgetOrWindow, QSize(100, 100));

  // Set interactor to not call Render() on the vtkRenderWindow. Clients might
  // set this to enforce a specified framerate by rendering only when a timer
  // fires, for example.
  renWin->GetInteractor()->EnableRenderOff();

  auto source = vtkSmartPointer<vtkSphereSource>::New();
  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputConnection(source->GetOutputPort());
  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  ren->AddActor(actor);
  ren->ResetCamera();
  renWin->Render(); // this will render a sphere at 100x100.

  // Resize widget. this should to retrigger a VTK render since
  // the interactor is disabled. We should still see the rendering result from
  // earlier.
  detail::show(widgetOrWindow, QSize(300, 300));

  // Get output image filename
  const std::string tempDir(vtktesting->GetTempDirectory());
  std::string fileName(vtktesting->GetValidImageFileName());
  auto slashPos = fileName.rfind('/');
  if (slashPos != std::string::npos)
  {
    fileName = fileName.substr(slashPos + 1);
  }
  fileName = tempDir + '/' + fileName;

  // Capture widget using Qt. Don't use vtkTesting to capture the image, because
  // this should test what the widget displays, not what VTK renders.
  const QImage image = detail::grab_framebuffer(widgetOrWindow);
  if (!image.save(QString::fromStdString(fileName)))
  {
    std::cout << "ERROR: Saving image failed" << std::endl;
    return EXIT_FAILURE;
  }

  int retVal = vtktesting->RegressionTest(fileName, 0);
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
