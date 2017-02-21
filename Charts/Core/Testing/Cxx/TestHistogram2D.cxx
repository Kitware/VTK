/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkChartHistogram2D.h"
#include "vtkColorTransferFunction.h"
#include "vtkContextMouseEvent.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlotHistogram2D.h"
#include "vtkPlotLine.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkVector.h"

//----------------------------------------------------------------------------
int TestHistogram2D(int, char * [])
{
  // Set up a 2D scene, add an XY chart to it
  int size = 400;
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(size, size);

  // Define a chart
  vtkNew<vtkChartHistogram2D> chart;
  view->GetScene()->AddItem(chart.GetPointer());

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetRenderWindow()->Render();

  // Add only a plot without an image data
  vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
  vtkSmartPointer<vtkDoubleArray> X = vtkSmartPointer<vtkDoubleArray>::New();
  X->SetName("X");
  X->SetNumberOfComponents(1);
  X->SetNumberOfTuples(size);
  vtkSmartPointer<vtkDoubleArray> Y = vtkSmartPointer<vtkDoubleArray>::New();
  Y->SetName("Y");
  Y->SetNumberOfComponents(1);
  Y->SetNumberOfTuples(size);

  for (int i = 0; i < size; i++)
  {
    X->SetTuple1(i, i);
    Y->SetTuple1(i, i);
  }
  table->AddColumn(X);
  table->AddColumn(Y);

  vtkPlotLine* plot = vtkPlotLine::SafeDownCast(chart->AddPlot((int)vtkChart::LINE));
  plot->SetInputData(table, 0, 1);
  plot->SetColor(1.0, 0.0, 0.0);
  plot->SetWidth(5);

  vtkContextMouseEvent mouseEvent;
  mouseEvent.SetInteractor(view->GetInteractor());
  vtkVector2i mousePosition;

  // Test interactions when there is only a plot and no image data
  mouseEvent.SetButton(vtkContextMouseEvent::LEFT_BUTTON);
  int x = chart->GetPoint1()[0] + 4;
  int y = chart->GetPoint1()[1] + 10;
  mousePosition.Set(x, y);
  mouseEvent.SetScreenPos(mousePosition);
  mouseEvent.SetPos(vtkVector2f(0.0,0.0));
  chart->MouseButtonPressEvent(mouseEvent);
  chart->MouseButtonReleaseEvent(mouseEvent);

  // Remove the plot and add an image data
  vtkIdType id = chart->GetPlotIndex(plot);
  chart->RemovePlot(id);

  vtkNew<vtkImageData> data;
  data->SetExtent(0, size-1, 0, size-1, 0, 0);
  data->AllocateScalars(VTK_DOUBLE, 1);

  data->SetOrigin(100.0, 0.0, 0.0);
  data->SetSpacing(2.0, 1.0, 1.0);

  double *dPtr = static_cast<double *>(data->GetScalarPointer(0, 0, 0));
  for (int i = 0; i < size; ++i)
  {
    for (int j = 0; j < size; ++j)
    {
      dPtr[i * size + j] = sin(vtkMath::RadiansFromDegrees(double(2*i))) *
          cos(vtkMath::RadiansFromDegrees(double(j)));
    }
  }
  chart->SetInputData(data.GetPointer());

  vtkNew<vtkColorTransferFunction> transferFunction;
  transferFunction->AddHSVSegment(0.0, 0.0, 1.0, 1.0,
                                  0.3333, 0.3333, 1.0, 1.0);
  transferFunction->AddHSVSegment(0.3333, 0.3333, 1.0, 1.0,
                                  0.6666, 0.6666, 1.0, 1.0);
  transferFunction->AddHSVSegment(0.6666, 0.6666, 1.0, 1.0,
                                  1.0, 0.2, 1.0, 0.3);
  transferFunction->Build();
  chart->SetTransferFunction(transferFunction.GetPointer());

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
