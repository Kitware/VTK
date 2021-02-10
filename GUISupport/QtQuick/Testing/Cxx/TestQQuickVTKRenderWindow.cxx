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

#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QUrl>

int TestQQuickVTKRenderWindow(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  QApplication app(argc, argv);

  QQmlApplicationEngine engine;
  engine.load(QUrl("qrc:///TestQQuickVTKRenderWindow.qml"));

  QObject* topLevel = engine.rootObjects().value(0);
  QQuickWindow* window = qobject_cast<QQuickWindow*>(topLevel);

  window->show();

  vtkNew<vtkTesting> vtktesting;
  vtktesting->AddArguments(argc, argv);

  //  vtkNew<vtkGenericOpenGLRenderWindow> renWin;
  //  renWin->Render();
  //  vtktesting->SetRenderWindow(renWin);

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
