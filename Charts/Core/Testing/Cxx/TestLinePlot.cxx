// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkChartXY.h"
#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkPlot.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"
#include "vtkTableAlgorithm.h"

#include <string>

static const int NPOINTS = 65;
static const float INCX = 7.5;

class vtkTestSineTableSource : public vtkTableAlgorithm
{
public:
  static vtkTestSineTableSource* New();
  vtkTypeMacro(vtkTestSineTableSource, vtkTableAlgorithm);
  static std::string GetXName() { return "X Axis"; }
  static std::string GetYName() { return "Sine2"; }

protected:
  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector) override
  {
    vtkTable* output = vtkTable::GetData(outputVector, 0);

    vtkNew<vtkFloatArray> x_arr;
    x_arr->SetNumberOfComponents(1);
    x_arr->SetName(vtkTestSineTableSource::GetXName().c_str());

    vtkNew<vtkFloatArray> y_arr;
    y_arr->SetNumberOfComponents(1);
    y_arr->SetName(vtkTestSineTableSource::GetYName().c_str());

    output->AddColumn(x_arr);
    output->AddColumn(y_arr);

    x_arr->SetNumberOfTuples(NPOINTS);
    y_arr->SetNumberOfTuples(NPOINTS);

    float inc = INCX / (NPOINTS - 1);
    for (int i = 0; i < NPOINTS; i++)
    {
      float x = i * inc;
      float y = sin(x) + 0.5;
      x_arr->SetTuple1(i, x);
      y_arr->SetTuple1(i, y);
    }

    return 1;
  }

private:
  vtkTestSineTableSource() { this->SetNumberOfInputPorts(0); }
};

vtkStandardNewMacro(vtkTestSineTableSource);

//------------------------------------------------------------------------------
int TestLinePlot(int, char*[])
{
  int status = EXIT_SUCCESS;
  // Set up a 2D scene, add an XY chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(400, 300);
  vtkNew<vtkChartXY> chart;
  view->GetScene()->AddItem(chart);

  // Create a table with some points in it...
  vtkNew<vtkTable> table;
  vtkNew<vtkFloatArray> arrX;
  arrX->SetName("X Axis");
  table->AddColumn(arrX);
  vtkNew<vtkFloatArray> arrC;
  arrC->SetName("Cosine");
  table->AddColumn(arrC);
  vtkNew<vtkFloatArray> arrS;
  arrS->SetName("Sine");
  table->AddColumn(arrS);
  vtkNew<vtkFloatArray> arr1;
  arr1->SetName("One");
  table->AddColumn(arr1);
  // Test charting with a few more points...
  int numPoints = NPOINTS;
  float inc = INCX / (numPoints - 1);
  table->SetNumberOfRows(numPoints);
  for (int i = 0; i < numPoints; ++i)
  {
    table->SetValue(i, 0, i * inc);
    table->SetValue(i, 1, cos(i * inc) + 0.0);
    table->SetValue(i, 2, sin(i * inc) + 0.0);
    table->SetValue(i, 3, 1.0);
  }

  // Add multiple line plots, setting the colors etc
  vtkPlot* line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 1);
  line->SetColor(0, 255, 0, 255);
  line->SetWidth(1.0);
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 2);
  line->SetColor(255, 0, 0, 255);
  line->SetWidth(5.0);

  // Create a test table algorithm and add it as a plot
  vtkNew<vtkTestSineTableSource> sin2;
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputConnection(sin2->GetOutputPort());
  line->SetInputArray(0, sin2->GetXName());
  line->SetInputArray(1, sin2->GetYName());
  line->SetColor(0, 0, 255, 255);
  line->SetWidth(4.0);

  // Render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();

  // Verify that log-scaling is improper for both x & y axes
  double bds[4];
  line->GetUnscaledInputBounds(bds);
  if (bds[0] * bds[1] > 0. || bds[2] * bds[3] > 0.)
  {
    cerr << "ERROR: Data on both X and Y axes expected to cross origin.\n";
    status = EXIT_FAILURE;
  }

  // Verify that log-scaling is proper for arr1 y axis (which
  // is not plotted so as to avoid changing baseline images).
  line = chart->AddPlot(vtkChart::LINE);
  line->SetInputData(table, 0, 3);
  line->Update();
  line->GetUnscaledInputBounds(bds);
  if (bds[0] * bds[1] > 0. || bds[2] * bds[3] <= 0.)
  {
    cerr << "ERROR: Data on X axis expected to cross origin.\n";
    status = EXIT_FAILURE;
  }

  return status;
}
