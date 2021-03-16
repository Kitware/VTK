/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQQuickVTKRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// Tests QQuickVTKWindowItem

#include "QQuickVTKRenderItem.h"
#include "QQuickVTKRenderWindow.h"
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkPNGWriter.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkWindowToImageFilter.h"

#include <QApplication>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QTimer>
#include <QUrl>

int TestQQuickVTKRenderWindow(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  QApplication app(argc, argv);

  QQmlApplicationEngine engine;
  qDebug() << engine.importPathList();
  engine.load(QUrl("qrc:///TestQQuickVTKRenderWindow.qml"));

  QObject* topLevel = engine.rootObjects().value(0);
  QQuickWindow* window = qobject_cast<QQuickWindow*>(topLevel);

  // Fetch the QQuick window using the standard object name set up in the constructor
  QQuickVTKRenderWindow* qquickvtkWindow =
    topLevel->findChild<QQuickVTKRenderWindow*>("QQuickVTKRenderWindow");

  // Fetch the QQuick window using the standard object name set up in the constructor
  QQuickVTKRenderItem* qquickvtkItem = topLevel->findChild<QQuickVTKRenderItem*>("ConeView");

  // Create a cone pipeline and add it to the view
  vtkNew<vtkActor> actor;
  vtkNew<vtkPolyDataMapper> mapper;
  vtkNew<vtkConeSource> cone;
  mapper->SetInputConnection(cone->GetOutputPort());
  actor->SetMapper(mapper);
  qquickvtkItem->renderer()->AddActor(actor);
  qquickvtkItem->renderer()->ResetCamera();
  qquickvtkItem->renderer()->SetBackground(0.5, 0.5, 0.7);
  qquickvtkItem->renderer()->SetBackground2(0.7, 0.7, 0.7);
  qquickvtkItem->renderer()->SetGradientBackground(true);

  window->show();

  // Wait a little for the application and window to be set up properly
  QEventLoop loop;
  QTimer::singleShot(100, &loop, SLOT(quit()));
  loop.exec();

  // Capture a screenshot of the whole window
  vtkSmartPointer<vtkImageData> im = qquickvtkWindow->captureScreenshot();

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);
  std::string validName = std::string(vtktesting->GetValidImageFileName());
  std::string::size_type slashPos = validName.rfind('/');
  if (slashPos != std::string::npos)
  {
    validName = validName.substr(slashPos + 1);
  }
  std::string tmpDir = vtktesting->GetTempDirectory();
  std::string vImage = tmpDir + "/" + validName;
  vtkNew<vtkPNGWriter> w;
  w->SetInputData(im);
  w->SetFileName(vImage.c_str());
  w->Write();

  int retVal = vtktesting->RegressionTest(vImage.c_str(), 10);

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
