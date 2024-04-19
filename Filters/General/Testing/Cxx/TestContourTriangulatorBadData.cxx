// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This example gives bad data to vtkContourTriangulator to check whether
// the algorithm will terminate vs. go into infinite recursion
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkContourTriangulator.h"
#include "vtkDataSetMapper.h"
#include "vtkDataSetReader.h"
#include "vtkNew.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

#include <string>

int TestContourTriangulatorBadData(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();
  std::string tempDir = testHelper->GetTempDirectory();
  std::string inputFileName = dataRoot + "/Data/TriangulatorBadData.vtk";
  std::string tempBaseline = tempDir + "/TestContourTriangulatorBadData.png";

  vtkNew<vtkDataSetReader> reader;
  reader->SetFileName(inputFileName.c_str());
  reader->Update();

  // To see the code in vtkCountourTriangulator that guards against infinite
  // loops, search  vtkCountourTriangulator.cxx for "infinite loop" comments
  vtkNew<vtkContourTriangulator> triangulator;
  triangulator->SetInputConnection(reader->GetOutputPort());
  triangulator->Update();

  // Display the contour, not the triangulation, since the triangulation
  // is guaranteed to be bad for this data (we just want to check that
  // triangulation code does not segfault or infinitely loop)
  vtkNew<vtkDataSetMapper> contourMapper;
  contourMapper->SetInputConnection(triangulator->GetOutputPort());
  contourMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> contourActor;
  contourActor->SetMapper(contourMapper);
  contourActor->GetProperty()->SetColor(1.0, 1.0, 1.0);

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(contourActor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  vtkCamera* camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Elevation(-90);

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
