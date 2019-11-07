/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQVTKWidget2.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "QTestMainWindow.h"
#include "QVTKWidget2.h"
#include "vtkActor.h"
#include "vtkConeSource.h"
#include "vtkDataSetMapper.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include <QApplication>
#include <QTimer>

int TestQVTKWidget2(int argc, char* argv[])
{
  QApplication app(argc, argv);

  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkConeSource> cone;

  vtkNew<vtkDataSetMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  vtkNew<vtkDataSetMapper> coneMapper;
  coneMapper->SetInputConnection(cone->GetOutputPort());
  vtkNew<vtkActor> coneActor;
  coneActor->SetMapper(coneMapper);

  sphereActor->GetProperty()->SetOpacity(0.3);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(sphereActor);
  renderer->AddActor(coneActor);
  renderer->ResetCamera();

  vtkNew<vtkGenericOpenGLRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);

  QTestMainWindow* qwindow = new QTestMainWindow(renWin, argc, argv);
  QVTKWidget2* widget = new QVTKWidget2(renWin);
  widget->setMinimumSize(QSize(300, 300));
  widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  qwindow->setCentralWidget(widget);

  qwindow->show();

  QTimer::singleShot(500, qwindow, SLOT(captureImage()));
  int appVal = app.exec();
  int retVal = qwindow->regressionImageResult();

  delete qwindow;

  return !retVal + appVal;
}
