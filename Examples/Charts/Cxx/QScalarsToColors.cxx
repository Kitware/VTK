/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFloatArray.h"
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkSmartPointer.h"

#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkCompositeTransferFunctionItem.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"

#include "vtkTimerLog.h"

#include <QApplication>
#include <QWidget>
#include <QMainWindow>
#include <QHBoxLayout>

#include "QVTKWidget.h"
#include "vtkQtTableView.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
int main( int argc, char * argv [] )
{
  // Qt initialization
  QApplication app(argc, argv);

  // QVTK set up and initialization
  QVTKWidget qvtkWidget(0);

  // Set up my 2D world...
  VTK_CREATE(vtkContextView, view); // This contains a chart object
  view->SetInteractor(qvtkWidget.GetInteractor());
  qvtkWidget.SetRenderWindow(view->GetRenderWindow());

  vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
  chart->SetTitle("Chart");
  view->GetScene()->AddItem(chart);

  vtkSmartPointer<vtkColorTransferFunction> colorTransferFunction =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  colorTransferFunction->AddHSVSegment(0.,0.,1.,1.,0.3333,0.3333,1.,1.);
  colorTransferFunction->AddHSVSegment(0.3333,0.3333,1.,1.,0.6666,0.6666,1.,1.);
  colorTransferFunction->AddHSVSegment(0.6666,0.6666,1.,1.,1.,0.,1.,1.);

  colorTransferFunction->Build();

  vtkSmartPointer<vtkPiecewiseFunction> opacityFunction =
    vtkSmartPointer<vtkPiecewiseFunction>::New();
  opacityFunction->AddPoint(0.,0.);
  opacityFunction->AddPoint(0.5,0.5);
  opacityFunction->AddPoint(1.,1.);

  vtkSmartPointer<vtkCompositeTransferFunctionItem> item3 =
    vtkSmartPointer<vtkCompositeTransferFunctionItem>::New();
  item3->SetColorTransferFunction(colorTransferFunction);
  item3->SetOpacityFunction(opacityFunction);
  item3->SetOpacity(0.2);
  item3->SetMaskAboveCurve(true);
  chart->AddPlot(item3);

  // Now lets try to add a table view
  //QWidget *widget = new QWidget(mainWindow);
  //QHBoxLayout *layout = new QHBoxLayout(widget);
  //layout->addWidget(qvtkWidget);
  //mainWindow->setCentralWidget(widget);

  // Now show the application and start the event loop
  qvtkWidget.show();
  //view->GetRenderWindow()->SetMultiSamples(0);

  return app.exec();
}
