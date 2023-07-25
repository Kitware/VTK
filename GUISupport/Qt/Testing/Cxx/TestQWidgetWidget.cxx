// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestQtCommon.h"

#include <QApplication>
#include <QPushButton>

#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPlaneSource.h"
#include "vtkQWidgetRepresentation.h"
#include "vtkQWidgetWidget.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

int TestQWidgetWidget(int argc, char* argv[])
{
  // disable multisampling.
  vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);
  QPushButton hello("Hello world!", nullptr);

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  auto widgetOrWindow = detail::create_widget_or_window(type, nullptr);
  vtkNew<vtkGenericOpenGLRenderWindow> window0;
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.2, 0.3, 0.4);
  window0->AddRenderer(renderer);
  detail::set_render_window(widgetOrWindow, window0);
  detail::show(widgetOrWindow, QSize(300, 300));
  detail::process_events_and_wait(100);
  window0->Render();

  vtkNew<vtkQWidgetWidget> widget;
  widget->CreateDefaultRepresentation();
  widget->GetQWidgetRepresentation()->GetPlaneSource()->SetPoint2(-0.5, 0.5, -0.5);
  widget->SetWidget(&hello);
  widget->SetCurrentRenderer(renderer);
  widget->SetInteractor(window0->GetInteractor());

  widget->SetEnabled(1);
  renderer->ResetCamera();
  renderer->ResetCameraClippingRange();
  window0->Render();
  detail::process_events_and_wait(100);
  vtktesting->SetRenderWindow(window0);
  renderer->ResetCamera();
  window0->Render();
  detail::process_events_and_wait(100);
  window0->Render();

  // return app.exec();

  // clear the widget first, to avoid using it
  // after it may have been freed.
  widget->SetWidget(nullptr);

  return EXIT_SUCCESS;
}
