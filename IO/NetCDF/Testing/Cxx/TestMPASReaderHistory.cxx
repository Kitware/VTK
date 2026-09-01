// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkMPASReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include <cstdlib>
#include <iostream>
#include <string>

int TestMPASReaderHistory(int argc, char* argv[])
{
  auto expandDataFile = [&](const char* relativePath)
  {
    char* fullPath = vtkTestUtilities::ExpandDataFileName(argc, argv, relativePath);
    std::string stringPath(fullPath);
    delete[] fullPath;
    return stringPath;
  };
  std::string gridPath = expandDataFile("Data/NetCDF/MPASReaderGrid.nc");
  std::string historyPath = expandDataFile("Data/NetCDF/MPASReaderHistory.nc");

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // make loop for multiple actors for the multiple modes of the reader
  for (int i = 0; i < 2; ++i)
  {
    bool primaryGrid = ((i & 0x01) != 0);

    vtkNew<vtkMPASReader> reader;
    reader->SetFileName(gridPath.c_str());
    reader->SetHistoryFileName(historyPath.c_str());

    reader->UpdateInformation();
    if (reader->GetNumberOfPointArrays() != 1 || std::string(reader->GetPointArrayName(0)) != "ke")
    {
      std::cerr << "The history file field was not exposed as point data.\n";
      return EXIT_FAILURE;
    }

    vtkInformation* information = reader->GetOutputInformation(0);
    if (information->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS()) != 11)
    {
      std::cerr << "The history file time steps were not exposed.\n";
      return EXIT_FAILURE;
    }
    int* verticalRange = reader->GetVerticalLevelRange();
    if (verticalRange[0] != 0 || verticalRange[1] != 3)
    {
      std::cerr << "The history file vertical levels were not exposed.\n";
      return EXIT_FAILURE;
    }

    reader->EnableAllCellArrays();
    reader->EnableAllPointArrays();
    reader->SetProjectLatLon(false);
    reader->SetUsePrimaryGrid(primaryGrid);
    reader->SetShowMultilayerView(false);
    reader->SetLayerThickness(1);
    reader->SetVerticalLevel(0);
    reader->UpdateTimeStep(11.0 * i);

    vtkNew<vtkGeometryFilter> geometryFilter;
    geometryFilter->SetInputConnection(reader->GetOutputPort());

    geometryFilter->UpdateInformation();
    vtkExecutive* executive = geometryFilter->GetExecutive();
    vtkInformationVector* inputVector = executive->GetInputInformation(0);
    double timeReq = 11.0 * i;
    inputVector->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);

    geometryFilter->Update();

    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(geometryFilter->GetOutputPort());
    mapper->ScalarVisibilityOn();
    mapper->SetColorModeToMapScalars();
    mapper->SetScalarRange(0.0, 220.0);
    if (primaryGrid)
    {
      mapper->SetScalarModeToUseCellFieldData();
    }
    else
    {
      mapper->SetScalarModeToUsePointFieldData();
    }
    mapper->SelectColorArray("ke");

    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    actor->RotateX(90.0);
    if (primaryGrid)
    {
      actor->AddPosition(1.0e7, 0.0, 0);
    }
    ren->AddActor(actor);
  }

  ren->ResetCamera(-6.e6, 1.6e7, -6.e6, 6.e6, -6.e6, 6.e6);
  ren->GetActiveCamera()->Zoom(2);

  ren->SetBackground(0, 0, 0);
  renWin->SetSize(350, 300);

  // interact with data
  renWin->Render();

  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);

  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  std::cerr << !retVal << " is the return val\n";
  return !retVal;
}
