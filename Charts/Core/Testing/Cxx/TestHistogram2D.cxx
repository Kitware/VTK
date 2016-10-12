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

#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkChartHistogram2D.h"
#include "vtkPlotHistogram2D.h"
#include "vtkImageData.h"
#include "vtkColorTransferFunction.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkMath.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

//----------------------------------------------------------------------------
int TestHistogram2D(int, char * [])
{
  // Set up a 2D scene, add an XY chart to it
  int size = 400;
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(size, size);
  vtkNew<vtkChartHistogram2D> chart;

  view->GetScene()->AddItem(chart.GetPointer());

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

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
