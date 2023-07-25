// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartHistogram2D.h"
#include "vtkColorTransferFunction.h"
#include "vtkContextView.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlotHistogram2D.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestHistogram2DComponent(int, char*[])
{
  const int EXTENT = 200;
  const int SIZE = 2 * EXTENT + 1;
  const char* ARRAY_NAME = "swirl";

  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(SIZE, SIZE);

  // Define a chart
  vtkNew<vtkChartHistogram2D> chart;
  view->GetScene()->AddItem(chart);

  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetRenderWindow()->Render();

  // Add an image data
  vtkNew<vtkImageData> data;
  data->SetExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, 0, 0);

  vtkIdType nbPoints = data->GetNumberOfPoints();
  int dims[3];
  data->GetDimensions(dims);

  // Compute swirl array
  vtkNew<vtkDoubleArray> array;
  array->SetName(ARRAY_NAME);
  array->SetNumberOfComponents(3);
  array->SetNumberOfTuples(nbPoints);
  for (vtkIdType i = 0; i < nbPoints; ++i)
  {
    int ijk[3];
    vtkStructuredData::ComputePointStructuredCoords(i, dims, ijk);
    array->SetTuple3(i, ijk[0] - EXTENT, ijk[1] - EXTENT, ijk[2]);
  }

  data->GetPointData()->AddArray(array);
  chart->SetInputData(data);

  // Select the multi-dimensional array
  vtkPlotHistogram2D* plot = vtkPlotHistogram2D::SafeDownCast(chart->GetPlot(0));
  plot->SetArrayName(ARRAY_NAME);

  // Set a transfer function for coloring
  vtkNew<vtkColorTransferFunction> transferFunction;
  transferFunction->AddRGBSegment(-EXTENT, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
  transferFunction->AddRGBSegment(0.0, 0.0, 1.0, 0.0, EXTENT, 0.0, 0.0, 1.0);
  transferFunction->Build();

  // Color by component (Y)
  transferFunction->SetVectorModeToComponent();
  transferFunction->SetVectorComponent(1);
  chart->SetTransferFunction(transferFunction);
  chart->RecalculateBounds();

  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
