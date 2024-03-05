// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "TestAxisActor2DInternal.h"

#include "vtkLogger.h"
#include "vtkPoints.h"

//------------------------------------------------------------------------------
bool TestDefaultLabels()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);

  std::vector<std::string> expectedLabels = { "0.00  ", "0.200 ", "0.400 ", "0.600 ", "0.800 ",
    "1.00  " };
  return axis->CompareLabelMapperString(expectedLabels);
}

//------------------------------------------------------------------------------
bool TestLabelsNotation()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);

  axis->SetNotation(vtkNumberToString::Scientific);
  axis->SetPrecision(3);
  window->Render();

  std::vector<std::string> expectedLabels = {
    "0.000e+0",
    "2.000e-1",
    "4.000e-1",
    "6.000e-1",
    "8.000e-1",
    "1.000e+0",
  };
  bool status = axis->CompareLabelMapperString(expectedLabels);

  axis->SetNotation(vtkNumberToString::Fixed);
  axis->SetPrecision(2);
  window->Render();
  expectedLabels = { "0.00", "0.20", "0.40", "0.60", "0.80", "1.00" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  return status;
}

//------------------------------------------------------------------------------
bool TestRangeLabels()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);
  axis->SetRange(42, 43);

  axis->AdjustLabelsOff();
  axis->SetNotation(vtkNumberToString::Fixed);
  axis->SetPrecision(2);
  window->Render();

  std::vector<std::string> expectedLabels = { "42.00", "42.25", "42.50", "42.75", "43.00" };
  return axis->CompareLabelMapperString(expectedLabels);
}

//------------------------------------------------------------------------------
bool TestNumberOfLabels()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);

  // adjust labels modifies number of labels. Disable it.
  axis->AdjustLabelsOff();
  int nbOfLabels = 6;
  axis->SetNumberOfLabels(nbOfLabels);

  vtkNew<vtkPoints> expectedPoints;
  double spacing = 48;
  for (int i = 1; i < nbOfLabels - 1; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  bool status = CompareTicksPosition(axis, window, expectedPoints);

  axis->SetNumberOfLabels(2);
  expectedPoints->Initialize();
  status = CompareTicksPosition(axis, window, expectedPoints);

  return status;
}

//------------------------------------------------------------------------------
bool TestAdjustLabels()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);
  axis->SetNotation(vtkNumberToString::Fixed);
  axis->SetPrecision(2);
  axis->SetRange(0, 1.);
  window->Render();

  vtkNew<vtkPoints> expectedPoints;
  double spacing = 48;
  int nbOfLabels = 6;
  for (int i = 1; i < nbOfLabels - 1; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  bool status = CompareTicksPosition(axis, window, expectedPoints);

  std::vector<std::string> expectedLabels = { "0.00", "0.20", "0.40", "0.60", "0.80", "1.00" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  axis->SetRange(0.05, 1.05);
  window->Render();
  // this does not changes anything !
  status = CompareTicksPosition(axis, window, expectedPoints) && status;
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  axis->SetRange(0.05, 1.1);
  // this changes the number of ticks !
  expectedPoints->Initialize();
  spacing = 60;
  nbOfLabels = 5;
  for (int i = 1; i < nbOfLabels - 1; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  status = CompareTicksPosition(axis, window, expectedPoints) && status;
  // this changes the bounds of the range !
  expectedLabels = { "0.00", "0.30", "0.60", "0.90", "1.20" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  return status;
}

//------------------------------------------------------------------------------
bool TestNumberOfMinorTicks()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);
  axis->AdjustLabelsOff();
  axis->SetNumberOfMinorTicks(1);
  axis->SetMinorTickLength(8);
  window->Render();

  vtkNew<vtkPoints> expectedPoints;
  double spacing = 30;
  int nbOfLabels = 9;
  for (int i = 1; i < nbOfLabels - 1; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  return CompareTicksPosition(axis, window, expectedPoints);
}

//------------------------------------------------------------------------------
bool TestRulerMode()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);
  axis->AdjustLabelsOff();
  axis->RulerModeOn();
  axis->SetNotation(vtkNumberToString::Fixed);
  axis->SetPrecision(2);

  vtkNew<vtkPoints> expectedPoints;
  expectedPoints->InsertNextPoint(242.132, 242.132, 0);
  bool status = CompareTicksPosition(axis, window, expectedPoints);
  // we have more labels than drawn ticks ...
  std::vector<std::string> expectedLabels = { "0.00", "0.88", "1.00" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  axis->SetRange(42, 43);
  axis->SetRulerDistance(0.4);
  axis->SetNumberOfMinorTicks(1);

  expectedPoints->Initialize();
  double spacing = 42.4264;
  int nbOfLabels = 7;
  for (int i = 1; i < nbOfLabels - 1; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  status = CompareTicksPosition(axis, window, expectedPoints);

  // we have less labels than drawn ticks ...
  expectedLabels = { "42.00", "42.35", "42.71", "43.00" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  return status;
}

//------------------------------------------------------------------------------
int TestAxisActor2D(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int status = EXIT_SUCCESS;
  if (!TestNumberOfLabels())
  {
    vtkLog(ERROR, "TestNumberOfLabels failed");
    status = EXIT_FAILURE;
  }
  if (!TestDefaultLabels())
  {
    vtkLog(ERROR, "TestDefaultLabels failed");
    status = EXIT_FAILURE;
  }
  if (!TestLabelsNotation())
  {
    vtkLog(ERROR, "TestLabelsNotation failed");
    status = EXIT_FAILURE;
  }
  if (!TestRangeLabels())
  {
    vtkLog(ERROR, "TestRangeLabels failed");
    status = EXIT_FAILURE;
  }
  if (!TestAdjustLabels())
  {
    vtkLog(ERROR, "TestAdjustLabels failed");
    status = EXIT_FAILURE;
  }
  if (!TestNumberOfMinorTicks())
  {
    vtkLog(ERROR, "TestNumberOfMinorTicks failed");
    status = EXIT_FAILURE;
  }
  if (!TestRulerMode())
  {
    vtkLog(ERROR, "TestRulerMode failed");
    status = EXIT_FAILURE;
  }

  // final screenshot with default parameters
  vtkNew<vtkAxisActor2D> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);

  // change property rendering option to make test more robust
  vtkNew<vtkTextProperty> textProp;
  textProp->SetColor(1, 0.5, 0);
  textProp->SetFontSize(18);
  textProp->BoldOn();
  axis->SetUseFontSizeFromProperty(true);
  axis->SetLabelTextProperty(textProp);
  axis->GetProperty()->SetColor(1, 0, 0);
  axis->GetProperty()->SetLineWidth(4);

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);
  interactor->Initialize();
  window->Render();
  interactor->Start();

  return status;
};
