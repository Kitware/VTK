// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Tests QVTKOpenGLStereoWidget/QVTKOpenGLNativeWidget/QVTKOpenGLWindow with vtkChartHistogram2D
#include "TestQtCommon.h"
#include "vtkChartHistogram2D.h"
#include "vtkColorTransferFunction.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlotHistogram2D.h"
#include "vtkPlotLine.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTable.h"
#include "vtkTesting.h"

#include <QApplication>
#include <QSurfaceFormat>

int TestQtWithChartHistogram2D(int argc, char* argv[])
{
  // disable multisampling globally.
  vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);

  auto type = detail::select_widget(argc, argv);
  // setup default format, if needed.
  detail::set_default_format(type);

  QApplication app(argc, argv);

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  vtkNew<vtkGenericOpenGLRenderWindow> window;
  window->SetMultiSamples(0); // disable multisampling

  auto widgetOrWindow = detail::create_widget_or_window(type, window);

  vtkNew<vtkRenderer> ren;
  ren->SetGradientBackground(true);
  ren->SetBackground2(0.7, 0.7, 0.7);
  window->AddRenderer(ren);

  int size = 300;

  vtkNew<vtkContextView> view;
  view->SetRenderWindow(window);
  // Define a chart
  vtkNew<vtkChartHistogram2D> chart;
  chart->SetAutoAxes(true);
  chart->SetRenderEmpty(true);
  view->GetScene()->AddItem(chart);

  vtkNew<vtkImageData> data;
  data->SetExtent(0, size - 1, 0, size - 1, 0, 0);
  data->AllocateScalars(VTK_DOUBLE, 1);

  data->SetOrigin(100.0, 0.0, 0.0);
  data->SetSpacing(2.0, 1.0, 1.0);

  double* dPtr = static_cast<double*>(data->GetScalarPointer(0, 0, 0));
  for (int i = 0; i < size; ++i)
  {
    for (int j = 0; j < size; ++j)
    {
      dPtr[i * size + j] = sin(vtkMath::RadiansFromDegrees(double(2 * i))) *
        cos(vtkMath::RadiansFromDegrees(double(j)));
    }
  }
  chart->SetInputData(data);

  vtkNew<vtkColorTransferFunction> transferFunction;
  transferFunction->AddHSVSegment(0.0, 0.0, 1.0, 1.0, 0.3333, 0.3333, 1.0, 1.0);
  transferFunction->AddHSVSegment(0.3333, 0.3333, 1.0, 1.0, 0.6666, 0.6666, 1.0, 1.0);
  transferFunction->AddHSVSegment(0.6666, 0.6666, 1.0, 1.0, 1.0, 0.2, 1.0, 0.3);
  transferFunction->Build();
  chart->SetTransferFunction(transferFunction);

  detail::show(widgetOrWindow, QSize(size, size));
  vtktesting->SetRenderWindow(window);

  int retVal = vtktesting->RegressionTest(10);
  switch (retVal)
  {
    case vtkTesting::DO_INTERACTOR:
      return QApplication::exec();
    case vtkTesting::FAILED:
    case vtkTesting::NOT_RUN:
      return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
