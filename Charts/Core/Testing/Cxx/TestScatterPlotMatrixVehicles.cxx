/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestScatterPlotMatrixVehicles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkScatterPlotMatrix.h"
#include "vtkRenderWindow.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkNew.h"
#include "vtkTestUtilities.h"
#include "vtkDelimitedTextReader.h"
#include "vtkTable.h"

//----------------------------------------------------------------------------
int TestScatterPlotMatrixVehicles(int argc, char *argv[])
{
  // Get the file name, and read the CSV file.
  char *fname = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                     "Data/vehicle_data.csv");
  vtkNew<vtkDelimitedTextReader> reader;
  reader->SetFileName(fname);
  reader->SetHaveHeaders(true);
  reader->SetDetectNumericColumns(true);
  delete [] fname;
  reader->Update();

  // Set up a 2D scene, add a chart to it
  vtkNew<vtkContextView> view;
  view->GetRenderWindow()->SetSize(800, 600);
  vtkNew<vtkScatterPlotMatrix> matrix;
  view->GetScene()->AddItem(matrix.GetPointer());

  // Set the scatter plot matrix up to analyze all columns in the table.
  matrix->SetInput(reader->GetOutput());

  //Finally render the scene and compare the image to a reference image
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->Start();
  return EXIT_SUCCESS;
}
