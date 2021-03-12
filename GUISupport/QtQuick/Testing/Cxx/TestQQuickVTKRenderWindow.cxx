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
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

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
  return app.exec();

  //  QApplication::sendPostedEvents();
  //  QApplication::processEvents();
  //
  //  QEventLoop loop;
  //  QTimer::singleShot(1000, &loop, SLOT(quit()));
  //  loop.exec();
  //
  //  QApplication::sendPostedEvents();
  //  QApplication::processEvents();
  //  QApplication::sendPostedEvents();
  //  QApplication::processEvents();

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  vtktesting->SetRenderWindow(qquickvtkWindow->renderWindow());

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
