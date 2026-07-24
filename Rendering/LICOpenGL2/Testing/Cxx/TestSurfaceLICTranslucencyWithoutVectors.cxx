// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This test uses Qt because of a bug that caused red/green color to leak
// into the depth peeling result only when rendered using QVTKOpenGLNativeWidget

#if VTK_MODULE_ENABLE_VTK_GUISupportQt
#define USE_QT 1
#endif

#if USE_QT
#include "vtkRegressionTestImage.h"
#include <QApplication>
#include <QVTKOpenGLNativeWidget.h>
#else
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindowInteractor.h"
#endif

#include "vtkActor.h"
#include "vtkDualDepthPeelingPass.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkSurfaceLICMapper.h"

int TestSurfaceLICTranslucencyWithoutVectors(int argc, char* argv[])
{
#if USE_QT
  QApplication app(argc, argv);
  // Qt widget
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());
  QVTKOpenGLNativeWidget widget;
  widget.resize(960, 540);
#endif
  (void)argc;
  (void)argv;
  // VTK renderer/window/interactor
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(1, 1, 1);
#if USE_QT
  vtkRenderWindow* window = widget.renderWindow();
#else
  vtkNew<vtkRenderWindow> window;
  window->SetSize(960, 540);
#endif
  window->AddRenderer(renderer);

  vtkNew<vtkSphereSource> source;
  vtkNew<vtkSurfaceLICMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetOpacity(0.5);
  renderer->AddActor(actor);

  // Render passes
  vtkNew<vtkRenderStepsPass> renderStepsPass;
  vtkNew<vtkDualDepthPeelingPass> depthPeelingPass;
  depthPeelingPass->SetTranslucentPass(renderStepsPass->GetTranslucentPass());
  renderStepsPass->SetTranslucentPass(depthPeelingPass);
  renderer->SetPass(renderStepsPass);

#if USE_QT
  widget.show();
  int retVal = vtkRegressionTestImage(window);
  if (retVal == vtkTesting::DO_INTERACTOR)
  {
    return app.exec();
  }
  return retVal == vtkTesting::FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
#else
  // Skip image test when not using Qt
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);
  vtkNew<vtkInteractorStyleTrackballCamera> style;
  interactor->SetInteractorStyle(style);
  interactor->Start();
  return 0;
#endif
}
