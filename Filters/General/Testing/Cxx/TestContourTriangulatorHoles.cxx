/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContourTriangulatorHoles.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkContourTriangulator with difficult holes
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkContourTriangulator.h"
#include "vtkDataSetMapper.h"
#include "vtkIdList.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

int TestContourTriangulatorHoles(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);

  std::string tempDir = testHelper->GetTempDirectory();
  std::string tempBaseline = tempDir + "/TestContourTriangulatorHoles.png";

  const double polys[6][4][3] = {
    {
      { -100.0, -100.0, 0.0 },
      { +100.0, -100.0, 0.0 },
      { +100.0, +100.0, 0.0 },
      { -100.0, +100.0, 0.0 },
    },
    {
      { -30.0, +30.0, 0.0 },
      { +30.0, +30.0, 0.0 },
      { +30.0, -30.0, 0.0 },
      { -30.0, -30.0, 0.0 },
    },
    {
      { -40.0, +80.0, 0.0 },
      { +40.0, +80.0, 0.0 },
      { +40.0, +50.0, 0.0 },
      { -40.0, +50.0, 0.0 },
    },
    {
      { -40.0, -50.0, 0.0 },
      { +40.0, -50.0, 0.0 },
      { +40.0, -80.0, 0.0 },
      { -40.0, -80.0, 0.0 },
    },
    {
      { -90.0, +90.0, 0.0 },
      { -50.0, +90.0, 0.0 },
      { -50.0, -90.0, 0.0 },
      { -90.0, -90.0, 0.0 },
    },
    {
      { +50.0, +90.0, 0.0 },
      { +90.0, +90.0, 0.0 },
      { +90.0, -90.0, 0.0 },
      { +50.0, -90.0, 0.0 },
    },
  };

  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> lines;
  for (int i = 0; i < 6; i++)
  {
    vtkIdType ids[5];
    for (int j = 0; j < 4; j++)
    {
      ids[j] = points->InsertNextPoint(polys[i][j]);
    }
    // close the contour
    ids[4] = ids[0];
    // add contour to data
    lines->InsertNextCell(5, ids);
  }

  vtkNew<vtkPolyData> data;
  data->SetPoints(points);
  data->SetLines(lines);
  data->BuildLinks();

  vtkNew<vtkContourTriangulator> triangulator;
  triangulator->SetInputData(data);

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetInputConnection(triangulator->GetOutputPort());
  mapper->ScalarVisibilityOff();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1.0, 1.0, 1.0);

  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(actor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);

  vtkCamera* camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Zoom(1.4);

  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
