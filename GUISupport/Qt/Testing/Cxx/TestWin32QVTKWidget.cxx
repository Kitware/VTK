/*=========================================================================

Program:   Visualization Toolkit
Module:    TestWin32QVTKWidget.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests behavior of vtkWin32OpenGLRenderWindow under a QT Context.
// @Note
// opengl startup behaves differently when running
// on a build server. This test tries to exercise areas that have
// been known to fail but only in a local build. I.e. one cannot
// trust the dashboard entry for this test unfortunately.


#include "vtkSmartPointer.h"
#include "QVTKWidget.h"
#include "vtkWin32OpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkRenderer.h"

#include <QPointer>
#include <QDockWidget>
#include <QTabWidget>
#include <QEvent>
#include <QApplication>
#include <QMainWindow>

#define TOKEN_TO_STRING(TOK) # TOK
#define STRINGIZE_TOKEN(TOK) TOKEN_TO_STRING(TOK)
#define PRINT_AND_EVAL(X) {std::string fnc=__FUNCTION__;std::cout<<fnc<<": "<<STRINGIZE_TOKEN(X)<<"="<<X<<std::endl;std::cout.flush();}

#define fail(msg) \
  std::cout << msg << std::endl; \
  return EXIT_FAILURE

void flushQtEvents()
{
  QApplication::sendPostedEvents( );
  QApplication::processEvents(QEventLoop::AllEvents,10);
}

void initializeWidget(QWidget* arg)
{
  arg->setGeometry(20,20,640,480);
  arg->show();
  flushQtEvents();
}

template <typename WidgetType1,typename WidgetType2=WidgetType1>
struct QVTKWidgetInsideQWidgets
{
  QVTKWidgetInsideQWidgets() : widget1(0), widget2(0), mainWindow(0)
  {
    renderer->SetBackground(1.0,0.0,0.8);
    renderer->SetBackground2(0.5,0.5,0.5);
    renderer->SetGradientBackground(true);
    glwin->AddRenderer(renderer.Get());
  }

  ~QVTKWidgetInsideQWidgets()
  {
    widget1.data()->deleteLater();
    widget2.data()->deleteLater();
    if(mainWindow && !mainWindow->parent())
      delete mainWindow;
    //delete widget2;
  }

  QVTKWidget* spawnSubwidget( QTabWidget* tabWidget,QVTKWidget* qvtk=NULL)
  {
    if(!mainWindow)
      mainWindow = new QMainWindow();
    tabWidget->setParent(mainWindow);
    mainWindow->setCentralWidget(tabWidget);
    if(!qvtk)
      qvtk = new QVTKWidget();
    qvtk->setMinimumSize(600,400);
    tabWidget->addTab(qvtk, "qvtk_widget");
    qvtk->setParent(tabWidget);
    return qvtk;
  }

  QVTKWidget* spawnSubwidget(QMainWindow* mainWin,QVTKWidget* qvtk = NULL)
  {
    mainWindow       = mainWin;
    if (!qvtk)
      qvtk = new QVTKWidget(mainWindow);

    qvtk->setMinimumSize(600, 400);
    mainWindow->setCentralWidget(qvtk);
    return qvtk;
  }

  QVTKWidget* spawnSubwidget(QDockWidget* dock,QVTKWidget* qvtk = NULL)
  {
    if (!mainWindow)
      mainWindow = new QMainWindow();
    dock->setParent(mainWindow);
    qvtk = new QVTKWidget(dock);
    qvtk->setMinimumSize(600, 400);
    dock->setWidget(qvtk);
    return qvtk;
  }

  int run()
  {
    //auto window = std::make_unique<QMainWindow>();
    widget1            = new WidgetType1();
    initializeWidget(widget1);
    QVTKWidget* qvtk   = spawnSubwidget( widget1 );

    qvtk->SetRenderWindow(glwin.Get());

    PRINT_AND_EVAL("BEFORE RENDER:" << glwin->ReportCapabilities());
    glwin->Render();
    PRINT_AND_EVAL("AFTER RENDER1:" << glwin->ReportCapabilities());

    flushQtEvents();
    widget2 = new WidgetType2();
    initializeWidget(widget2);
    QVTKWidget* qvtk1_ref = spawnSubwidget( widget2, qvtk );

    qvtk1_ref->GetRenderWindow()->Render();

    PRINT_AND_EVAL("AFTER RENDER2:" << glwin->ReportCapabilities());
    flushQtEvents();
    return EXIT_SUCCESS;
  }

  QPointer<WidgetType1>               widget1;
  QPointer<WidgetType2>               widget2;
  QPointer<QMainWindow>               mainWindow;
  vtkNew<vtkWin32OpenGLRenderWindow>  glwin;
  vtkNew<vtkRenderer>                 renderer;
};

int TestWin32QVTKWidget(int argc, char* argv[])
{

  QApplication app(argc, argv);

  if( EXIT_SUCCESS != QVTKWidgetInsideQWidgets<QMainWindow>().run() )
    return EXIT_FAILURE;

  if (EXIT_SUCCESS != QVTKWidgetInsideQWidgets<QMainWindow,QTabWidget>().run())
    return EXIT_FAILURE;

  if (EXIT_SUCCESS != QVTKWidgetInsideQWidgets<QMainWindow,QDockWidget>().run())
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
