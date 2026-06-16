// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAMRContourFilter.h"
#include "vtkAMREnzoReader.h"
#include "vtkAMRGaussianPulseSource.h"
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
  ProgressChecker(vtkAMRContourFilter* contour)
    : Contour(contour){};

  void ProgressCheck()
  {
    this->ProgressCount++;
    double currentProgress = this->Contour->GetProgress();
    if (currentProgress < this->PreviousProgress)
    {
      this->ProgressStatus = false;
    }
  }

  double PreviousProgress = 0;
  int ProgressCount = 0;
  bool ProgressStatus = true;
  vtkAMRContourFilter* Contour;
};

void ProgressCheck(vtkObject*, unsigned long, void* clientdata, void*)
{
  ProgressChecker* progressChecker = reinterpret_cast<ProgressChecker*>(clientdata);
  progressChecker->ProgressCheck();
}

bool TestAMRContourProgress()
{

  vtkNew<vtkAMRGaussianPulseSource> source;

  // Convert to point data to test interpolation
  vtkNew<vtkCellDataToPointData> cd2pd;
  cd2pd->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkAMRContourFilter> contour;
  contour->SetInputConnection(cd2pd->GetOutputPort());
  contour->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Gaussian-Pulse");
  std::vector<double> values = { 1.2953579971568757e-06 };
  contour->SetContourValues(values);
  contour->ComputeNormalsOff();
  contour->ComputeScalarsOff();
  contour->GenerateTrianglesOff();

  ProgressChecker progressChecker(contour);
  vtkNew<vtkCallbackCommand> progressObserver;
  progressObserver->SetCallback(::ProgressCheck);
  progressObserver->SetClientData(&progressChecker);
  contour->AddObserver(vtkCommand::ProgressEvent, progressObserver);

  contour->Update();

  return progressChecker.ProgressStatus && progressChecker.ProgressCount == 47;
}

bool TestAMRContourOptions(const std::string& dataRoot)
{

  vtkNew<vtkAMRGaussianPulseSource> source;

  // Convert to point data to test interpolation
  vtkNew<vtkCellDataToPointData> cd2pd;
  cd2pd->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkAMRContourFilter> contour;
  contour->SetInputConnection(cd2pd->GetOutputPort());
  contour->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Gaussian-Pulse");
  std::vector<double> values = { 1.2953579971568757e-06 };
  contour->SetContourValues(values);
  contour->ComputeNormalsOff();
  contour->ComputeScalarsOff();
  contour->GenerateTrianglesOff();
  contour->Update();

  // XXX: use vtkHDFReader: https://gitlab.kitware.com/vtk/vtk/-/issues/19971
  std::string fileName = dataRoot + "/Data/amr_contour_options_expected.vtpd";
  vtkNew<vtkXMLPartitionedDataSetReader> expectedReader;
  expectedReader->SetFileName(fileName.c_str());
  expectedReader->Update();

  return !vtkTestUtilities::CompareDataObjects(
    contour->GetOutput(), expectedReader->GetOutput(), true);
}

bool TestAMRContourDefault(const std::string& dataRoot)
{

  vtkNew<vtkAMRGaussianPulseSource> source;

  // Convert to point data to test interpolation
  vtkNew<vtkCellDataToPointData> cd2pd;
  cd2pd->SetInputConnection(source->GetOutputPort());

  vtkNew<vtkAMRContourFilter> contour;
  contour->SetInputConnection(cd2pd->GetOutputPort());
  contour->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Gaussian-Pulse");
  std::vector<double> values = { 1.2953579971568757e-06 };
  contour->SetContourValues(values);

  // Do not generate triangles because of
  // https://gitlab.kitware.com/vtk/vtk/-/issues/19973 and
  // https://gitlab.kitware.com/vtk/vtk/-/issues/19977
  contour->GenerateTrianglesOff();

  contour->Update();

  // XXX: use vtkHDFReader: https://gitlab.kitware.com/vtk/vtk/-/issues/19971
  std::string fileName = dataRoot + "/Data/amr_contour_expected.vtpd";
  vtkNew<vtkXMLPartitionedDataSetReader> expectedReader;
  expectedReader->SetFileName(fileName.c_str());
  expectedReader->Update();

  return !vtkTestUtilities::CompareDataObjects(
    contour->GetOutput(), expectedReader->GetOutput(), true);
}

bool TestAMRContourMultiGrid(const std::string& dataRoot)
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

  vtkNew<vtkAMRContourFilter> contour;
  contour->SetInputConnection(cd2pd->GetOutputPort());
  contour->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Density");
  std::vector<double> values = { 4.228202438914248e-27 };
  contour->SetContourValues(values);
  contour->ComputeNormalsOff();

  // Do not generate triangles because of
  // https://gitlab.kitware.com/vtk/vtk/-/issues/19973 and
  // https://gitlab.kitware.com/vtk/vtk/-/issues/19977
  contour->GenerateTrianglesOff();

  // This tests generates warnings but this is expected
  contour->Update();

  // XXX: use vtkHDFReader: https://gitlab.kitware.com/vtk/vtk/-/issues/19971
  fileName = dataRoot + "/Data/amr_contour_multigrid_expected.vtpd";
  vtkNew<vtkXMLPartitionedDataSetReader> expectedReader;
  expectedReader->SetFileName(fileName.c_str());
  expectedReader->Update();

  return !vtkTestUtilities::CompareDataObjects(
    contour->GetOutput(), expectedReader->GetOutput(), true);
}
}

//------------------------------------------------------------------------------
int TestAMRContour(int argc, char* argv[])
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
  ret |= ::TestAMRContourDefault(dataRoot);
  ret |= ::TestAMRContourOptions(dataRoot);
  ret |= ::TestAMRContourProgress();
  ret |= ::TestAMRContourMultiGrid(dataRoot);
  return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
