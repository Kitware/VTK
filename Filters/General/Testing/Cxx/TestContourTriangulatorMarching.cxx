// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This example demonstrates how to use vtkContourTriangulator
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkContourTriangulator.h"
#include "vtkDataSetMapper.h"
#include "vtkMarchingSquares.h"
#include "vtkPNGReader.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTesting.h"

int TestContourTriangulatorMarching(int argc, char* argv[])
{
  vtkSmartPointer<vtkTesting> testHelper = vtkSmartPointer<vtkTesting>::New();
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string inputFileName = dataRoot + "/Data/fullhead15.png";

  vtkSmartPointer<vtkPNGReader> reader = vtkSmartPointer<vtkPNGReader>::New();
  if (!reader->CanReadFile(inputFileName.c_str()))
  {
    std::cerr << "Error: Could not read " << inputFileName << ".\n";
    return EXIT_FAILURE;
  }
  reader->SetFileName(inputFileName.c_str());
  reader->Update();

  vtkSmartPointer<vtkMarchingSquares> iso = vtkSmartPointer<vtkMarchingSquares>::New();
  iso->SetInputConnection(reader->GetOutputPort());
  iso->SetValue(0, 500);

  vtkSmartPointer<vtkDataSetMapper> isoMapper = vtkSmartPointer<vtkDataSetMapper>::New();
  isoMapper->SetInputConnection(iso->GetOutputPort());
  isoMapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> isoActor = vtkSmartPointer<vtkActor>::New();
  isoActor->SetMapper(isoMapper);
  isoActor->GetProperty()->SetColor(0, 0, 0);

  vtkSmartPointer<vtkContourTriangulator> poly = vtkSmartPointer<vtkContourTriangulator>::New();
  poly->SetInputConnection(iso->GetOutputPort());

  vtkSmartPointer<vtkDataSetMapper> polyMapper = vtkSmartPointer<vtkDataSetMapper>::New();
  polyMapper->SetInputConnection(poly->GetOutputPort());
  polyMapper->ScalarVisibilityOff();

  vtkSmartPointer<vtkActor> polyActor = vtkSmartPointer<vtkActor>::New();
  polyActor->SetMapper(polyMapper);
  polyActor->GetProperty()->SetColor(1.0, 1.0, 1.0);

  // Standard rendering classes
  vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renderer->AddActor(polyActor);
  renderer->AddActor(isoActor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  vtkCamera* camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Azimuth(180);

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
