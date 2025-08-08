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
  bool status = axis->CompareLabelMapperString(expectedLabels);

  axis->SetRange(42, 42);
  expectedLabels = { "42.00", "42.00", "42.00", "42.00", "42.00" };
  window->Render();
  status = axis->CompareLabelMapperString(expectedLabels);

  axis->SetRange(-42, -43);
  expectedLabels = { "-42.00", "-42.25", "-42.50", "-42.75", "-43.00" };
  window->Render();
  status = axis->CompareLabelMapperString(expectedLabels);

  return status;
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
  for (int i = 0; i < nbOfLabels; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  bool status = CompareTicksPosition(axis, window, expectedPoints);

  axis->SetNumberOfLabels(2);
  expectedPoints->Initialize();
  expectedPoints->InsertNextPoint(START_POINT, START_POINT, 0);
  expectedPoints->InsertNextPoint(END_POINT, END_POINT, 0);
  status = CompareTicksPosition(axis, window, expectedPoints);

  return status;
}

//------------------------------------------------------------------------------
bool TestSnapLabels()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);
  axis->SnapLabelsToGridOn();
  axis->SetNotation(vtkNumberToString::Fixed);
  axis->SetPrecision(2);
  window->Render();

  vtkNew<vtkPoints> expectedPoints;
  double spacing = 48;
  int nbOfLabels = 6;
  for (int i = 0; i < nbOfLabels; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  bool status = CompareTicksPosition(axis, window, expectedPoints);
  std::vector<std::string> expectedLabels = { "0.00", "0.20", "0.40", "0.60", "0.80", "1.00" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  axis->SetRange(0.05, 1.05);
  window->Render();
  expectedPoints->Initialize();
  double shiftedStart = 66;
  nbOfLabels = 5;
  for (int i = 0; i < nbOfLabels; i++)
  {
    expectedPoints->InsertNextPoint(shiftedStart + i * spacing, shiftedStart + i * spacing, 0);
  }
  status = CompareTicksPosition(axis, window, expectedPoints) && status;
  // 0 is now out of bounds
  expectedLabels = { "0.20", "0.40", "0.60", "0.80", "1.00" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  axis->SetRange(-1, -2);
  expectedPoints->Initialize();
  spacing = 48;
  nbOfLabels = 6;
  for (int i = 0; i < nbOfLabels; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  status = CompareTicksPosition(axis, window, expectedPoints) && status;
  expectedLabels = { "-1.00", "-1.20", "-1.40", "-1.60", "-1.80", "-2.00" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  window->Render();

  return status;
}

//------------------------------------------------------------------------------
bool TestNumberOfMinorTicks()
{
  vtkNew<vtkAxisActor2DMock> axis;
  vtkNew<vtkRenderWindow> window;
  SetupPipeline(axis, window);
  axis->AdjustLabelsOff();
  int nbOfMinorTicks = 1;
  const int nbOfMajorTicks = 5;
  // one label per tick.
  // nbMinorTicks is per major tick interval interval.
  int nbOfLabels = (nbOfMajorTicks - 1) * (nbOfMinorTicks) + nbOfMajorTicks;
  axis->SetNumberOfMinorTicks(nbOfMinorTicks);
  axis->SetMinorTickLength(8);
  window->Render();

  vtkNew<vtkPoints> expectedPoints;
  const double majorSpacing = 240;
  double spacing = majorSpacing / (nbOfLabels - 1);
  for (int i = 0; i < nbOfLabels; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }

  if (!CompareTicksPosition(axis, window, expectedPoints))
  {
    return false;
  }

  nbOfMinorTicks = 3;
  nbOfLabels = (nbOfMajorTicks - 1) * (nbOfMinorTicks) + nbOfMajorTicks;
  spacing = majorSpacing / (nbOfLabels - 1);

  axis->SetNumberOfMinorTicks(nbOfMinorTicks);
  window->Render();
  expectedPoints->Initialize();
  for (int i = 0; i < nbOfLabels; i++)
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
  double spacing = 212.132;
  int nbOfTicks = 2;
  for (int i = 0; i < nbOfTicks; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  bool status = CompareTicksPosition(axis, window, expectedPoints);
  std::vector<std::string> expectedLabels = { "0.00", "0.88" };
  status = axis->CompareLabelMapperString(expectedLabels) && status;

  axis->SetRange(42, 43);
  axis->SetRulerDistance(0.42);
  axis->SetNumberOfMinorTicks(1);

  expectedPoints->Initialize();
  spacing = 44.5477;
  nbOfTicks = 6;
  for (int i = 0; i < nbOfTicks; i++)
  {
    expectedPoints->InsertNextPoint(START_POINT + i * spacing, START_POINT + i * spacing, 0);
  }
  status = CompareTicksPosition(axis, window, expectedPoints);

  expectedLabels = { "42.00", "42.37", "42.74" };
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
  if (!TestSnapLabels())
  {
    vtkLog(ERROR, "TestSnapLabels failed");
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
