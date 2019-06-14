/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScatterPlotMatrixVehiclesDensity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextScene.h"
#include "vtkContextView.h"
#include "vtkDelimitedTextReader.h"
#include "vtkNew.h"
#include "vtkOTScatterPlotMatrix.h"
#include "vtkPlotPoints.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTable.h"
#include "vtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestScatterPlotMatrixVehiclesDensity(int argc, char* argv[])
{
  // Get the file name, and read the CSV file.
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/vehicle_data.csv");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(fname);
  reader->SetHaveHeaders(true);
  reader->SetDetectNumericColumns(true);
  delete[] fname;
  reader->Update();

  // Set up a 2D scene, add a chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkOTScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix);

  // Set the scatter plot matrix up to analyze all columns in the table.
  matrix->SetInput(reader->GetOutput());
  matrix->SetPlotMarkerStyle(vtkOTScatterPlotMatrix::ACTIVEPLOT, vtkPlotPoints::NONE);
  matrix->SetDensityMapVisibility(vtkOTScatterPlotMatrix::ACTIVEPLOT, true);
  matrix->SetDensityMapVisibility(vtkOTScatterPlotMatrix::SCATTERPLOT, true);

  // Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();

  view->GetRenderWindow()->Render();
  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Start();
  }

  return !retVal;
}
