// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .NAME Test of vtkMPASReader
// .SECTION Description
// Tests the vtkMPASReader.

#include "vtkMPASReader.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkExecutive.h"
#include "vtkExtractGeometry.h"
#include "vtkGeometryFilter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"

#include "vtkDataArray.h"
#include "vtkPointData.h"

#include <iostream>

int TestMPASReader(int argc, char* argv[])
{
  // Basic visualisation.
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  renWin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Read file names.
  char* fName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/NetCDF/MPASReader.nc");
  std::string fileName(fName);
  delete[] fName;
  fName = nullptr;

  // make loop for multiple actors for the multiple modes of the reader
  for (int i = 0; i < 6; i++)
  {
    bool primaryGrid = ((i & 0x01) != 0);
    bool projectLatLon = ((i & 0x02) != 0);
    bool multilayer = ((i & 0x04) != 0);

    // Create the reader.
    vtkNew<vtkMPASReader> reader;
    reader->SetFileName(fileName.c_str());

    // Crinkle clip if creating layers
    vtkNew<vtkExtractGeometry> extract;
    if (multilayer)
    {
      vtkNew<vtkPlane> plane;
      plane->SetOrigin(0.0, 0.0, 0.0);
      plane->SetNormal(-0.866, 0.0, 0.5);

      extract->SetInputConnection(reader->GetOutputPort());
      extract->SetImplicitFunction(plane);
    }

    // Convert to PolyData.
    vtkNew<vtkGeometryFilter> geometryFilter;
    geometryFilter->SetInputConnection(
      multilayer ? extract->GetOutputPort() : reader->GetOutputPort());

    geometryFilter->UpdateInformation();
    vtkExecutive* executive = geometryFilter->GetExecutive();
    vtkInformationVector* inputVector = executive->GetInputInformation(0);
    double timeReq = 0;
    inputVector->GetInformationObject(0)->Set(
      vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    reader->Update();
    reader->EnableAllCellArrays();
    reader->EnableAllPointArrays();
    reader->SetProjectLatLon(projectLatLon);
    reader->SetUsePrimaryGrid(primaryGrid);
    reader->SetShowMultilayerView(multilayer);
    reader->SetLayerThickness(1000000);
    reader->SetVerticalLevel(i);
    reader->Update();

    int* values = reader->GetVerticalLevelRange();
    if (values[0] != 0 || values[1] != 3)
    {
      vtkGenericWarningMacro("Vertical level range is incorrect.");
      return 1;
    }
    values = reader->GetLayerThicknessRange();
    if (values[0] != 0 || values[1] != 200000)
    {
      vtkGenericWarningMacro("Layer thickness range is incorrect.");
      return 1;
    }
    values = reader->GetCenterLonRange();
    if (values[0] != 0 || values[1] != 360)
    {
      vtkGenericWarningMacro("Center lon range is incorrect.");
      return 1;
    }

    // Create a mapper and LUT.
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(geometryFilter->GetOutputPort());
    mapper->ScalarVisibilityOn();
    mapper->SetColorModeToMapScalars();
    mapper->SetScalarRange(0.0116, 199.9);
    if (primaryGrid)
    {
      mapper->SetScalarModeToUseCellFieldData();
    }
    else
    {
      mapper->SetScalarModeToUsePointFieldData();
    }
    mapper->SelectColorArray("ke");

    // Create the actor.
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    if (projectLatLon)
    {
      actor->SetScale(30000);
      actor->AddPosition(4370000, 0, 0);
    }
    if (primaryGrid)
    {
      actor->AddPosition(0, 1.0e7, 0);
    }
    if (multilayer)
    {
      actor->AddPosition(-10000000, 0, 0);
    }
    ren->AddActor(actor);
  }

  ren->ResetCamera(-14000000, 12370000, -6370000, 16370000, -6370000, 6370000);
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
