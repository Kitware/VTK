/*=========================================================================

  Program:   Visualization Toolkit
  Module:    QScalarsToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "QVTKOpenGLWidget.h"
#include "vtkChartXY.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkGenericOpenGLRenderWindow.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPlot.h"
#include "vtkQtTableView.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QSurfaceFormat>
#include <QWidget>

//----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  // needed to ensure appropriate OpenGL context is created for VTK rendering.
  QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());

  // Qt initialization
  QApplication app(argc, argv);

  // QVTK set up and initialization
  QVTKOpenGLWidget qvtkWidget;

  vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
  qvtkWidget.setRenderWindow(renderWindow);

  // Set up my 2D world...
  vtkNew<vtkContextView> view;
  ; // This contains a chart object
  view->SetRenderWindow(qvtkWidget.renderWindow());
  view->SetInteractor(qvtkWidget.interactor());

  vtkNew<vtkChartXY> chart;
  chart->SetTitle("Chart");
  view->GetScene()->AddItem(chart);

  vtkNew<vtkColorTransferFunction> colorTransferFunction;
  colorTransferFunction->AddHSVSegment(0., 0., 1., 1., 0.3333, 0.3333, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.3333, 0.3333, 1., 1., 0.6666, 0.6666, 1., 1.);
  colorTransferFunction->AddHSVSegment(0.6666, 0.6666, 1., 1., 1., 0., 1., 1.);
  colorTransferFunction->Build();

  vtkNew<vtkPiecewiseFunction> opacityFunction;
  opacityFunction->AddPoint(0., 0.);
  opacityFunction->AddPoint(0.5, 0.5);
  opacityFunction->AddPoint(1., 1.);

  vtkNew<vtkCompositeTransferFunctionItem> item3;
  item3->SetColorTransferFunction(colorTransferFunction);
  item3->SetOpacityFunction(opacityFunction);
  item3->SetOpacity(0.2);
  item3->SetMaskAboveCurve(true);
  chart->AddPlot(item3);

  // Now show the application and start the event loop
  qvtkWidget.show();
  return app.exec();
}
