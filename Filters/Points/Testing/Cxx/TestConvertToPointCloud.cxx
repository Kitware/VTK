// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test covers the vtkConvertToPointCloud

#include "vtkActor.h"
#include "vtkConvertToPointCloud.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPolyDataReader.h"

#include <iostream>

//------------------------------------------------------------------------------
// Check for a regression of a bug where the filter would crash if given an
// empty vtkPointSet.
static bool TestEmptyInput()
{
  vtkNew<vtkUnstructuredGrid> emptyInput;

  vtkNew<vtkConvertToPointCloud> convPointCloud;
  convPointCloud->SetInputData(emptyInput);
  convPointCloud->SetCellGenerationMode(vtkConvertToPointCloud::VERTEX_CELLS);
  convPointCloud->Update();

  vtkPolyData* output = vtkPolyData::SafeDownCast(convPointCloud->GetOutput());
  if (!output)
  {
    std::cerr << "TestConvertToPointCloud did not create output data from empty input" << std::endl;
    return false;
  }
  if ((output->GetNumberOfCells() != 0) || (output->GetNumberOfPoints()))
  {
    std::cerr << "TestConvertToPointCloud did not give empty output for empty input" << std::endl;
    return false;
  }
  return true;
}

//------------------------------------------------------------------------------
static bool TestGeneral(int argc, char* argv[])
{
  vtkNew<vtkXMLPolyDataReader> reader;
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cow.vtp");
  reader->SetFileName(fname);
  delete[] fname;

  vtkNew<vtkConvertToPointCloud> convPointCloud;
  convPointCloud->SetInputConnection(reader->GetOutputPort());

  // Check different modes first
  convPointCloud->SetCellGenerationMode(vtkConvertToPointCloud::NO_CELLS);
  convPointCloud->Update();
  vtkPolyData* output = vtkPolyData::SafeDownCast(convPointCloud->GetOutput());
  if (!output || output->GetNumberOfCells() != 0)
  {
    std::cerr << "TestConvertToPointCloud failed with NO_CELLS mode" << std::endl;
    return false;
  }

  convPointCloud->SetCellGenerationMode(vtkConvertToPointCloud::VERTEX_CELLS);
  convPointCloud->Update();
  output = vtkPolyData::SafeDownCast(convPointCloud->GetOutput());
  if (!output || output->GetNumberOfCells() != 2903)
  {
    std::cerr << "TestConvertToPointCloud failed with VERTEX_CELLS mode" << std::endl;
    return false;
  }

  convPointCloud->SetCellGenerationMode(vtkConvertToPointCloud::POLYVERTEX_CELL);
  convPointCloud->Update();
  output = vtkPolyData::SafeDownCast(convPointCloud->GetOutput());
  if (!output || output->GetNumberOfCells() != 1)
  {
    std::cerr << "TestConvertToPointCloud failed with POLYVERTEX_CELL mode" << std::endl;
    return false;
  }

  // Check a render
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(convPointCloud->GetOutputPort());

  vtkNew<vtkRenderer> renderer;

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 600);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  renderer->AddActor(actor);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return (retVal != vtkRegressionTester::FAILED);
}

//------------------------------------------------------------------------------
int TestConvertToPointCloud(int argc, char* argv[])
{
  bool success = TestEmptyInput() && TestGeneral(argc, argv);

  return (success ? 0 : 1);
}
