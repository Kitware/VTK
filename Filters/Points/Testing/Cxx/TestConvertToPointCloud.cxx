/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestConvertToPointCloud.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
#include "vtkXMLPolyDataReader.h"

//------------------------------------------------------------------------------
int TestConvertToPointCloud(int argc, char* argv[])
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
    return 0;
  }

  convPointCloud->SetCellGenerationMode(vtkConvertToPointCloud::VERTEX_CELLS);
  convPointCloud->Update();
  output = vtkPolyData::SafeDownCast(convPointCloud->GetOutput());
  if (!output || output->GetNumberOfCells() != 2903)
  {
    std::cerr << "TestConvertToPointCloud failed with VERTEX_CELLS mode" << std::endl;
    return 0;
  }

  convPointCloud->SetCellGenerationMode(vtkConvertToPointCloud::POLYVERTEX_CELL);
  convPointCloud->Update();
  output = vtkPolyData::SafeDownCast(convPointCloud->GetOutput());
  if (!output || output->GetNumberOfCells() != 1)
  {
    std::cerr << "TestConvertToPointCloud failed with POLYVERTEX_CELL mode" << std::endl;
    return 0;
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

  return !retVal;
}
