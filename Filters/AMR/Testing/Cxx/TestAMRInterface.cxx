// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAMREnzoReader.h"
#include "vtkAMRGaussianPulseSource.h"
#include "vtkAMRInterfaceFilter.h"
#include "vtkCallbackCommand.h"
#include "vtkCellDataToPointData.h"
#include "vtkPartitionedDataSet.h"
#include "vtkTestUtilities.h"
#include "vtkTesting.h"
#include "vtkXMLPartitionedDataSetReader.h"

#include <iostream>

namespace
{
struct ProgressChecker
{
  ProgressChecker(vtkAMRInterfaceFilter* interface)
    : Interface(interface){};

  void ProgressCheck()
  {
    this->ProgressCount++;
    double currentProgress = this->Interface->GetProgress();
    if (currentProgress < this->PreviousProgress)
    {
      this->ProgressStatus = false;
    }
  }

  double PreviousProgress = 0;
  int ProgressCount = 0;
  bool ProgressStatus = true;
  vtkAMRInterfaceFilter* Interface;
};

void ProgressCheck(vtkObject*, unsigned long, void* clientdata, void*)
{
  ProgressChecker* progressChecker = reinterpret_cast<ProgressChecker*>(clientdata);
  progressChecker->ProgressCheck();
}

bool TestAMRInterfaceProgress()
{

  vtkNew<vtkAMRGaussianPulseSource> source;

  // Convert to point data to test interpolation
  vtkNew<vtkCellDataToPointData> cd2pd;
  cd2pd->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkAMRInterfaceFilter> interface;
  interface->SetInputConnection(cd2pd->GetOutputPort());

  ProgressChecker progressChecker(interface);
  vtkNew<vtkCallbackCommand> progressObserver;
  progressObserver->SetCallback(::ProgressCheck);
  progressObserver->SetClientData(&progressChecker);
  interface->AddObserver(vtkCommand::ProgressEvent, progressObserver);

  interface->Update();

  return progressChecker.ProgressStatus && progressChecker.ProgressCount == 47;
}

bool TestAMRInterfaceDefault(const std::string& dataRoot)
{

  vtkNew<vtkAMRGaussianPulseSource> source;

  // Convert to point data to test interpolation
  vtkNew<vtkCellDataToPointData> cd2pd;
  cd2pd->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkAMRInterfaceFilter> interface;
  interface->SetInputConnection(cd2pd->GetOutputPort());
  interface->Update();

  // XXX: use vtkHDFReader: https://gitlab.kitware.com/vtk/vtk/-/issues/19971
  std::string fileName = dataRoot + "/Data/amr_interface_expected.vtpd";
  vtkNew<vtkXMLPartitionedDataSetReader> expectedReader;
  expectedReader->SetFileName(fileName.c_str());
  expectedReader->Update();

  return !vtkTestUtilities::CompareDataObjects(
    interface->GetOutput(), expectedReader->GetOutput(), true);
}

bool TestAMRInterfaceMultiGrid(const std::string& dataRoot)
{
  vtkNew<vtkAMREnzoReader> reader;
  std::string fileName = dataRoot + "/Data/AMR/Enzo/DD0010/moving7_0010.hierarchy";
  reader->SetFileName(fileName.c_str());
  reader->SetMaxLevel(10);
  reader->SetCellArrayStatus("Density", 1);

  // Convert to point data to test interpolation
  vtkNew<vtkCellDataToPointData> cd2pd;
  cd2pd->SetInputConnection(reader->GetOutputPort());
  cd2pd->Update();

  vtkNew<vtkAMRInterfaceFilter> interface;
  interface->SetInputConnection(cd2pd->GetOutputPort());
  // This tests generates warnings but this is expected
  interface->Update();

  // XXX: use vtkHDFReader: https://gitlab.kitware.com/vtk/vtk/-/issues/19971
  fileName = dataRoot + "/Data/amr_interface_multigrid_expected.vtpd";
  vtkNew<vtkXMLPartitionedDataSetReader> expectedReader;
  expectedReader->SetFileName(fileName.c_str());
  expectedReader->Update();

  return !vtkTestUtilities::CompareDataObjects(
    interface->GetOutput(), expectedReader->GetOutput(), true);
}
}

//------------------------------------------------------------------------------
int TestAMRInterface(int argc, char* argv[])
{
  vtkNew<vtkTesting> testHelper;
  testHelper->AddArguments(argc, argv);
  if (!testHelper->IsFlagSpecified("-D"))
  {
    std::cerr << "Error: -D /path/to/data was not specified.";
    return EXIT_FAILURE;
  }

  std::string dataRoot = testHelper->GetDataRoot();

  bool ret = true;
  ret |= ::TestAMRInterfaceDefault(dataRoot);
  ret |= ::TestAMRInterfaceProgress();
  ret |= ::TestAMRInterfaceMultiGrid(dataRoot);
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
