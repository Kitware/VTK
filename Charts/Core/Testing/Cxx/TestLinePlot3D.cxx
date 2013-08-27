/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLinePlot3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderWindow.h"
#include "vtkSmartPointer.h"
#include "vtkChartXYZ.h"
#include "vtkPen.h"
#include "vtkPlotLine3D.h"
#include "vtkTable.h"
#include "vtkFloatArray.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"

// Plot the solution to the Lorenz attractor.
// http://en.wikipedia.org/wiki/Lorenz_system
namespace
{
void lorenz(const float * varX, float * varXDerivative)
{
  const float sigma = 10.f;
  const float rho = 28.f;
  const float beta = 2.66666666666f;

  varXDerivative[0] = sigma * (varX[1] - varX[0]);
  varXDerivative[1] = varX[0] * (rho - varX[2]) - varX[1];
  varXDerivative[2] = varX[0] * varX[1] - beta * varX[2];
}
} // end anonymous namespace

//----------------------------------------------------------------------------
int TestLinePlot3D(int, char * [])
{
  // Create the data.
  vtkNew<vtkTable> varXSolution;
  vtkNew<vtkFloatArray> arrX0;
  arrX0->SetName("X");
  varXSolution->AddColumn(arrX0.GetPointer());
  vtkNew<vtkFloatArray> arrX1;
  arrX1->SetName("Y");
  varXSolution->AddColumn(arrX1.GetPointer());
  vtkNew<vtkFloatArray> arrX2;
  arrX2->SetName("Z");
  varXSolution->AddColumn(arrX2.GetPointer());
  const unsigned int numberOfTimePoints = 1000;
  varXSolution->SetNumberOfRows(numberOfTimePoints);
  float varX[3];
  varX[0] = 0.0f;
  varX[1] = 1.0f;
  varX[2] = 1.05f;
  float varXDerivative[3];
  const float deltaT = 0.01f;
  for (unsigned int ii = 0; ii < numberOfTimePoints; ++ii)
    {
    varXSolution->SetValue(ii, 0, varX[0]);
    varXSolution->SetValue(ii, 1, varX[1]);
    varXSolution->SetValue(ii, 2, varX[2]);
    lorenz(varX, varXDerivative);
    varX[0] += varXDerivative[0] * deltaT;
    varX[1] += varXDerivative[1] * deltaT;
    varX[2] += varXDerivative[2] * deltaT;
    }

  // Set up a 3D scene and add an XYZ chart to it.
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXYZ> chart;
  chart->SetGeometry(vtkRectf(75.0, 20.0, 250, 260));
  view->GetScene()->AddItem(chart.GetPointer());

  // Add a line plot.
  vtkNew<vtkPlotLine3D> plot;
  plot->SetInputData(varXSolution.GetPointer());
  plot->GetPen()->SetColorF(0.1, 0.2, 0.8, 1.0);
  chart->AddPlot(plot.GetPointer());

  // Finally render the scene and compare the image to a reference image.
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
